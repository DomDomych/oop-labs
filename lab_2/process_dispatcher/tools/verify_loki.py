#!/usr/bin/env python3
"""Start an isolated real Loki, push dispatcher logs, query back and compare every record."""
import argparse
import json
from pathlib import Path
import socket
import subprocess
import sys
import tempfile
import time
import urllib.error
import urllib.parse
import urllib.request
import uuid


def get(url):
    # The test server is always loopback; do not send requests through a system proxy.
    opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))
    with opener.open(url, timeout=3) as response:
        return response.read()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--program", required=True, type=Path)
    parser.add_argument("--loki-bin", required=True, type=Path)
    parser.add_argument("--report", required=True, type=Path)
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="dispatcher-loki-") as temp:
        root = Path(temp)
        with socket.socket() as sock:
            sock.bind(("127.0.0.1", 0))
            port = sock.getsockname()[1]
        with socket.socket() as sock:
            sock.bind(("127.0.0.1", 0))
            grpc_port = sock.getsockname()[1]
        url = f"http://127.0.0.1:{port}"
        config = f'''auth_enabled: false
server:
  http_listen_address: 127.0.0.1
  http_listen_port: {port}
  grpc_listen_address: 127.0.0.1
  grpc_listen_port: {grpc_port}
common:
  instance_addr: 127.0.0.1
  path_prefix: {root}/data
  storage:
    filesystem:
      chunks_directory: {root}/data/chunks
      rules_directory: {root}/data/rules
  replication_factor: 1
  ring:
    kvstore:
      store: inmemory
schema_config:
  configs:
    - from: 2024-01-01
      store: tsdb
      object_store: filesystem
      schema: v13
      index:
        prefix: index_
        period: 24h
analytics:
  reporting_enabled: false
'''
        (root / "loki.yaml").write_text(config)
        with (root / "server.log").open("wb") as log:
            process = subprocess.Popen([str(args.loki_bin.resolve()), "-config.file=" + str(root / "loki.yaml")],
                                       stdout=log, stderr=subprocess.STDOUT)
            try:
                deadline = time.monotonic() + 55
                while True:
                    try:
                        get(url + "/ready")
                        break
                    except Exception as error:
                        detail = error.read().decode(errors="replace") if isinstance(error, urllib.error.HTTPError) else str(error)
                        if process.poll() is not None or time.monotonic() >= deadline:
                            raise RuntimeError(detail + "\n" + (root / "server.log").read_text(encoding="utf-8")[-6000:])
                        time.sleep(0.25)
                verification = uuid.uuid4().hex
                start = time.time_ns()
                request = {"mode": "async", "max_processes": 2, "report": "report.json", "jobs": [
                    {"id": "slow", "executable": "@self", "arguments": ["--fixture", "--delay-ms", "100"]},
                    {"id": "fast", "executable": "@self", "arguments": ["--fixture", "--stdout", "ok"]}],
                    "logging": {"console": False, "file": "events.jsonl", "loki": {
                        "url": url + "/loki/api/v1/push", "labels": {"app": "process_dispatcher", "verification": verification}}}}
                (root / "config.json").write_text(json.dumps(request))
                subprocess.run([str(args.program.resolve()), str(root / "config.json")], check=True, timeout=15)
                expected = [json.loads(line) for line in (root / "events.jsonl").read_text(encoding="utf-8").splitlines()]
                query = urllib.parse.urlencode({"query": '{app="process_dispatcher",verification="' + verification + '"}',
                                                "start": start, "end": time.time_ns(), "direction": "forward", "limit": 1000})
                fetched = []
                for _ in range(20):
                    response = json.loads(get(url + "/loki/api/v1/query_range?" + query))
                    values = [pair for stream in response["data"]["result"] for pair in stream["values"]]
                    fetched = [json.loads(pair[1]) for pair in sorted(values, key=lambda pair: int(pair[0]))]
                    if fetched == expected:
                        break
                    time.sleep(0.25)
                assert fetched == expected, (fetched, expected)
                report = {"checked_at": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
                          "loki": json.loads(get(url + "/loki/api/v1/status/buildinfo")),
                          "verification": verification, "file_matches_loki": True,
                          "completion_order": [x["fields"]["job_id"] for x in fetched if x["event"] == "process_exited"],
                          "events": fetched}
                args.report.parent.mkdir(parents=True, exist_ok=True)
                args.report.write_text(json.dumps(report, indent=2) + "\n")
                print(f"Real Loki: {len(fetched)} records match; report: {args.report}")
            finally:
                process.terminate()
                try:
                    process.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()


if __name__ == "__main__":
    sys.stdout.reconfigure(encoding="utf-8")
    main()
