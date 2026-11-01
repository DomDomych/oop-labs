"""Loopback HTTP contract test; no external Loki instance or credentials."""
import http.server
import json
import pathlib
import subprocess
import sys
import tempfile
import threading
import time


class Handler(http.server.BaseHTTPRequestHandler):
    status = 204
    delay = 0
    received = []

    def do_POST(self):
        body = self.rfile.read(int(self.headers['Content-Length']))
        self.received.append((self.path, self.headers.get('Content-Type'), json.loads(body)))
        time.sleep(self.delay)
        self.send_response(self.status)
        self.end_headers()

    def log_message(self, *_):
        pass


with tempfile.TemporaryDirectory(prefix='compositor-http-') as folder:
    root = pathlib.Path(folder)
    server = http.server.ThreadingHTTPServer(('127.0.0.1', 0), Handler)
    worker = threading.Thread(target=server.serve_forever, daemon=True)
    worker.start()
    try:
        config = {
            'canvas': {'width': 1, 'height': 1, 'background': [1, 2, 3]},
            'render': {'mode': 'sequential', 'threads': 1, 'region': {'width': 1, 'height': 1}},
            'rectangles': [], 'output': {'image': 'result.ppm'},
            'logging': {'file': 'run.jsonl', 'loki': {
                'url': f'http://127.0.0.1:{server.server_port}/loki/api/v1/push', 'timeout_ms': 200}}
        }
        path = root / 'config.json'
        path.write_text(json.dumps(config))
        def run():
            return subprocess.run([sys.argv[1], str(path)], capture_output=True, text=True, timeout=10)
        result = run()
        assert result.returncode == 0, result.stderr
        endpoint, content_type, payload = Handler.received[0]
        assert endpoint == '/loki/api/v1/push' and content_type == 'application/json'
        values = payload['streams'][0]['values']
        assert len(values) == 2 and all(isinstance(v[0], str) for v in values)
        events = [json.loads(v[1]) for v in values]
        assert [e['event'] for e in events] == ['program_started', 'program_finished']
        assert int(events[-1]['fields']['duration_ns']) >= 0
        assert events[-1]['fields']['exit_code'] == '0'
        Handler.status = 500
        result = run()
        assert result.returncode == 3 and 'HTTP 500' in result.stderr, result.stderr
        assert len((root / 'run.jsonl').read_text().splitlines()) == 4
        Handler.status = 260
        result = run()
        assert result.returncode == 3 and 'HTTP 260' in result.stderr
        Handler.status = 204
        Handler.delay = 1
        start = time.monotonic()
        result = run()
        assert result.returncode == 3 and time.monotonic() - start < 3, result.stderr
        assert len((root / 'run.jsonl').read_text().splitlines()) == 8
        assert (root / 'result.ppm').read_bytes().endswith(bytes([1, 2, 3]))
        assert len(Handler.received) == 4  # One attempt per run, never retried.
    finally:
        server.shutdown()
        server.server_close()
        worker.join()
