#!/usr/bin/env python3
"""Black-box acceptance tests: actual processes, byte-exact output and actual HTTP transport."""
import http.server
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import threading
import time
import unittest

PROGRAM = str(Path(sys.argv.pop(1)).resolve())


def job(name, *args):
    return {"id": name, "executable": "@self", "arguments": ["--fixture", *args]}


class Integration(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="dispatcher test Кириллица-")
        self.root = Path(self.temp.name)
        self.counter = 0

    def tearDown(self):
        self.temp.cleanup()

    def run_config(self, jobs, mode="async", limit=2, expected=0, **extra):
        self.counter += 1
        directory = self.root / str(self.counter)
        directory.mkdir()
        config = {"mode": mode, "max_processes": limit, "jobs": jobs,
                  "report": "report.json", "logging": {"console": False, "file": "run.jsonl"}}
        config.update(extra)
        path = directory / "config.json"
        path.write_text(json.dumps(config), encoding="utf-8")
        run = subprocess.run([PROGRAM, str(path)], capture_output=True, timeout=20)
        self.assertEqual(run.returncode, expected, run.stderr.decode(errors="replace"))
        report_path = directory / config["report"]
        report = json.loads(report_path.read_text(encoding="utf-8")) if report_path.exists() else None
        return report, directory, run

    def check_stream(self, result, stream, expected):
        capture = result[stream]
        self.assertEqual(capture["error"], "")
        self.assertEqual(capture["bytes"], len(expected))
        self.assertEqual(Path(capture["path"]).read_bytes(), expected)

    def test_reference_and_completion_order(self):
        jobs = [job("slow", "--delay-ms", "600", "--stdout", "slow\n"),
                job("fast", "--delay-ms", "15", "--stdout", "fast\n"), job("empty")]
        for mode in ("sync", "async"):
            r, d, _ = self.run_config(jobs, mode)
            self.assertEqual([x["id"] for x in r["results"]], ["slow", "fast", "empty"])
            expected = ["slow", "fast", "empty"] if mode == "sync" else ["fast", "empty", "slow"]
            self.assertEqual([e["job_id"] for e in r["completions"]], expected)
            self.assertEqual([e["sequence"] for e in r["completions"]], [1, 2, 3])
            self.assertEqual(r["peak_concurrency"], 1 if mode == "sync" else 2)
            for result, text in zip(r["results"], (b"slow\n", b"fast\n", b"")):
                self.check_stream(result, "stdout", text)
                self.check_stream(result, "stderr", b"")
            logs = [json.loads(line) for line in (d / "run.jsonl").read_text(encoding="utf-8").splitlines()]
            self.assertEqual([x["fields"]["job_id"] for x in logs if x["event"] == "process_exited"], expected)
            self.assertEqual(logs[0]["event"], "program_started")
            self.assertEqual(logs[-1]["event"], "program_finished")

    def test_failures_and_literal_arguments(self):
        literal = "quotes ' \" $() ; space\nПривет"
        denied = self.root / "not-executable"
        denied.write_text("text")
        jobs = [{"id": "missing", "executable": "/definitely/missing/program", "arguments": []},
                job("nonzero", "--exit-code", "7", "--stderr", "error\n"),
                {"id": "denied", "executable": str(denied), "arguments": []},
                job("literal", "--stdout", literal, "--stderr", "")]
        for mode in ("sync", "async"):
            r, _, _ = self.run_config(jobs, mode, expected=3)
            self.assertEqual([x["status"] for x in r["results"]],
                             ["launch_failed", "nonzero_exit", "launch_failed", "succeeded"])
            self.assertIsNone(r["results"][0]["exit_code"])
            self.assertEqual(r["results"][1]["exit_code"], 7)
            self.check_stream(r["results"][3], "stdout", literal.encode())
            self.assertEqual(len(r["completions"]), 4)

    def test_large_binary_dual_streams(self):
        count = 2 * 1024 * 1024 + 31
        jobs = [job(f"large{i}", "--bytes", str(count), "--binary") for i in range(3)]
        stdout = (bytes(range(256)) * ((count + 255) // 256))[:count]
        stderr = (bytes(reversed(range(256))) * ((count + 255) // 256))[:count]
        for mode in ("sync", "async"):
            r, _, _ = self.run_config(jobs, mode, block_size=4093)
            for result in r["results"]:
                self.check_stream(result, "stdout", stdout)
                self.check_stream(result, "stderr", stderr)

    def test_early_stream_close_and_signal(self):
        jobs = [{"id": "closed", "executable": sys.executable,
                 "arguments": ["-c", "import os,time;os.close(1);os.write(2,b'error stream');time.sleep(.03)"]},
                {"id": "signal", "executable": sys.executable,
                 "arguments": ["-c", "import os,signal;os.kill(os.getpid(),signal.SIGTERM)"]}, job("after")]
        for mode in ("sync", "async"):
            r, _, _ = self.run_config(jobs, mode, expected=3)
            self.check_stream(r["results"][0], "stdout", b"")
            self.check_stream(r["results"][0], "stderr", b"error stream")
            self.assertEqual(r["results"][1]["status"], "nonzero_exit")
            self.assertNotEqual(r["results"][1]["exit_code"], 0)
            self.assertEqual(r["results"][2]["status"], "succeeded")

    def test_concurrency_limit_and_empty_run(self):
        for limit in (1, 2, 4):
            r, _, _ = self.run_config([job(f"j{i}", "--delay-ms", "40") for i in range(8)], limit=limit)
            self.assertEqual(r["peak_concurrency"], limit)
            events = []
            for result in r["results"]:
                events.extend([(int(result["started_at_ns"]), 1), (int(result["finished_at_ns"]), -1)])
            active = 0
            for _, delta in sorted(events):
                active += delta
                self.assertLessEqual(active, limit)
            self.assertEqual(active, 0)
        for mode in ("sync", "async"):
            r, _, _ = self.run_config([], mode)
            self.assertTrue(r["successful"])
            self.assertEqual(r["results"], [])
            self.assertEqual(r["peak_concurrency"], 0)

    def test_invalid_config(self):
        changes = [{"mode": "other"}, {"max_processes": 0}, {"block_size": -1},
                   {"unknown": 3}, {"jobs": [job("same"), job("same")]},
                   {"jobs": [job("../escape")]}, {"jobs": [job("nul", "--stdout", "\x00")]},
                   {"logging": {"file": "report.json"}}]
        for extra in changes:
            self.run_config(extra.pop("jobs", []), expected=2, **extra)

    def test_repeated_run_preserves_captured_files(self):
        r, d, _ = self.run_config([job("same", "--stdout", "keep")])
        first = Path(r["results"][0]["stdout"]["path"])
        subprocess.run([PROGRAM, str(d / "config.json"), "--mode", "sync"], check=True, capture_output=True, timeout=10)
        second = json.loads((d / "report.json").read_text(encoding="utf-8"))["results"][0]["stdout"]["path"]
        self.assertNotEqual(str(first), second)
        self.assertEqual(first.read_bytes(), b"keep")

    def test_report_failure(self):
        blocker = self.root / "regular-file"
        blocker.write_bytes(b"block")
        self.run_config([job("ok")], expected=2, report=str(blocker / "report.json"))

    def test_loki_http_delivery_and_failures(self):
        for status in (204, 500, 260):
            bodies = []
            class Handler(http.server.BaseHTTPRequestHandler):
                def do_POST(self):
                    bodies.append((self.path, json.loads(self.rfile.read(int(self.headers["Content-Length"])))))
                    self.send_response(status)
                    self.end_headers()
                def log_message(self, *_):
                    pass
            with http.server.ThreadingHTTPServer(("127.0.0.1", 0), Handler) as server:
                thread = threading.Thread(target=server.serve_forever, daemon=True)
                thread.start()
                try:
                    r, d, run = self.run_config([job("ok", "--stdout", "captured")], expected=0 if status == 204 else 4,
                        logging={"console": False, "file": "run.jsonl", "loki": {
                            "url": f"http://127.0.0.1:{server.server_port}/loki/api/v1/push",
                            "labels": {"app": "dispatcher_test"}, "timeout_ms": 1000}})
                    self.assertTrue(r["successful"])
                    self.assertEqual(bodies[0][0], "/loki/api/v1/push")
                    stream = bodies[0][1]["streams"][0]
                    self.assertEqual(stream["stream"], {"app": "dispatcher_test"})
                    received = [json.loads(x[1]) for x in stream["values"]]
                    local = [json.loads(x) for x in (d / "run.jsonl").read_text(encoding="utf-8").splitlines()]
                    self.assertEqual(received, local)
                    self.assertEqual(len(received), 6)
                finally:
                    server.shutdown()
                    thread.join()


if __name__ == "__main__":
    unittest.main()
