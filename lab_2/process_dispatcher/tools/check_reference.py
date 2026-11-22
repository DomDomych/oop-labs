#!/usr/bin/env python3
"""Independent sequential oracle using Python subprocess; compare both C++ dispatchers."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile


def sha(path):
    with path.open("rb") as file:
        return hashlib.file_digest(file, "sha256").hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--program", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()
    program = args.program.resolve()
    with tempfile.TemporaryDirectory(prefix="dispatcher-reference-") as temp:
        root = Path(temp)
        jobs = [
            {"id": "slow", "executable": str(program), "arguments": ["--fixture", "--delay-ms", "250", "--stdout", "A\n", "--stderr", "err\n"]},
            {"id": "fast", "executable": str(program), "arguments": ["--fixture", "--stdout", "Привет ' \" ; $()", "--exit-code", "7"]},
            {"id": "empty", "executable": str(program), "arguments": ["--fixture"]},
            {"id": "missing", "executable": str(root / "missing"), "arguments": []},
            {"id": "binary", "executable": str(program), "arguments": ["--fixture", "--bytes", "1048583", "--binary"]},
        ]
        expected = []
        for job in jobs:
            out, err = root / (job["id"] + ".out"), root / (job["id"] + ".err")
            with out.open("wb") as stdout, err.open("wb") as stderr:
                try:
                    run = subprocess.run([job["executable"], *job["arguments"]], stdin=subprocess.DEVNULL,
                                         stdout=stdout, stderr=stderr, timeout=10)
                    code = abs(run.returncode)
                    status = "succeeded" if code == 0 else "nonzero_exit"
                except OSError:
                    code, status = None, "launch_failed"
            expected.append({"id": job["id"], "status": status, "exit_code": code,
                             "stdout_bytes": out.stat().st_size, "stdout_sha256": sha(out),
                             "stderr_bytes": err.stat().st_size, "stderr_sha256": sha(err)})
        evidence = {"oracle": "Python subprocess.run, serial; independent of Boost and C++ dispatchers",
                    "expected": expected, "modes": {}}
        for mode in ("sync", "async"):
            config = {"mode": mode, "jobs": jobs, "max_processes": 2, "report": mode + ".json",
                      "logging": {"console": False}, "block_size": 4093}
            path = root / "config.json"
            path.write_text(json.dumps(config))
            result = subprocess.run([str(program), str(path)], capture_output=True, timeout=15)
            assert result.returncode == 3, result.stderr
            report = json.loads((root / (mode + ".json")).read_text(encoding="utf-8"))
            for actual, wanted in zip(report["results"], expected, strict=True):
                for field in ("id", "status", "exit_code"):
                    assert actual[field] == wanted[field], (mode, field, actual, wanted)
                for stream in ("stdout", "stderr"):
                    assert not actual[stream]["error"]
                    assert actual[stream]["bytes"] == wanted[stream + "_bytes"]
                    assert sha(Path(actual[stream]["path"])) == wanted[stream + "_sha256"]
            evidence["modes"][mode] = {"matches": True, "peak_concurrency": report["peak_concurrency"],
                                        "completion_order": [x["job_id"] for x in report["completions"]]}
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(evidence, indent=2) + "\n")
        print("Both modes match the independent Python reference:", args.report)


if __name__ == "__main__":
    sys.stdout.reconfigure(encoding="utf-8")
    main()
