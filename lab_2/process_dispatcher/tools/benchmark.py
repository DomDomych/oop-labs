#!/usr/bin/env python3
"""Reproducible dispatcher experiment. Verifies every output before using a timing."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import statistics
import subprocess
import tempfile
import time


def digest(path):
    with open(path, "rb") as file:
        return hashlib.file_digest(file, "sha256").hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--program", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--repetitions", type=int, default=3)
    args = parser.parse_args()
    if args.repetitions < 1:
        parser.error("repetitions must be positive")
    program = args.program.resolve()
    matrix = []
    for scenario, delay, size in (("waiting", 120, 0), ("mixed", 30, 65536), ("output", 0, 1048576)):
        for mode, limit in (("sync", 1), ("async", 1), ("async", 2), ("async", 4), ("async", 8)):
            matrix.append(dict(scenario=scenario, mode=mode, limit=limit, delay_ms=delay,
                               bytes_per_stream=size, logging=True, samples=[]))
    matrix.append(dict(scenario="waiting", mode="async", limit=4, delay_ms=120,
                       bytes_per_stream=0, logging=False, samples=[]))
    with tempfile.TemporaryDirectory(prefix="dispatcher-bench-") as temp:
        root = Path(temp)
        for repetition in range(args.repetitions + 1):
            # Warmup is omitted. Rotate the order to reduce systematic run-order bias.
            order = matrix[repetition:] + matrix[:repetition]
            for index, case in enumerate(order):
                directory = root / f"{repetition}-{index}"
                directory.mkdir()
                jobs = [{"id": f"j{i}", "executable": str(program), "arguments": ["--fixture",
                         "--delay-ms", str(case["delay_ms"]), "--bytes", str(case["bytes_per_stream"])]}
                        for i in range(8)]
                config = dict(mode=case["mode"], max_processes=case["limit"], jobs=jobs,
                              report="report.json", logging={"console": False})
                if case["logging"]:
                    config["logging"]["file"] = "run.jsonl"
                path = directory / "config.json"
                path.write_text(json.dumps(config))
                start = time.perf_counter_ns()
                subprocess.run([str(program), str(path)], check=True, stdout=subprocess.DEVNULL,
                               stderr=subprocess.PIPE, timeout=30)
                total = time.perf_counter_ns() - start
                report = json.loads((directory / "report.json").read_text(encoding="utf-8"))
                assert report["successful"] and report["peak_concurrency"] <= case["limit"]
                for result in report["results"]:
                    for name, byte in (("stdout", b"O"), ("stderr", b"E")):
                        expected = hashlib.sha256(byte * case["bytes_per_stream"]).hexdigest()
                        assert result[name]["bytes"] == case["bytes_per_stream"]
                        assert digest(result[name]["path"]) == expected, (case, result["id"], name)
                if repetition:
                    case["samples"].append({"dispatch_ns": report["duration_ns"], "total_ns": total,
                                            "peak_concurrency": report["peak_concurrency"], "outputs_verified": True})
                # Retain measurements, not repeated megabytes of known fixture bytes.
                import shutil
                shutil.rmtree(directory)
    for case in matrix:
        case["median_ns"] = statistics.median(x["dispatch_ns"] for x in case["samples"])
        case["median_total_ns"] = statistics.median(x["total_ns"] for x in case["samples"])
    for case in matrix:
        baseline = next(x for x in matrix if x["scenario"] == case["scenario"] and x["mode"] == "sync")
        case["speedup"] = baseline["median_ns"] / case["median_ns"]
    report = {"schema_version": 1, "checked_at": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
              "platform": platform.platform(), "logical_cpus": os.cpu_count(), "program": str(program),
              "temporary_directory": tempfile.gettempdir(), "durability": "no fsync; filesystem/cache affects output timings",
              "jobs_per_run": 8, "warmup_runs_per_case": 1, "repetitions": args.repetitions,
              "cases": matrix}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    lines = ["# Исследование синхронного и асинхронного выполнения", "",
             f"Среда: `{report['platform']}`, логических CPU: {report['logical_cpus']}.",
             f"По 8 процессов, 1 прогрев и {args.repetitions} измерения на конфигурацию. Указаны медианы.",
             "Каждый результат проверен по размеру и SHA-256 обоих потоков; неуспешные результаты не включаются.", "",
             "| Сценарий | Режим | Лимит | Логи в файл | Диспетчер, мс | Весь запуск, мс | Ускорение |",
             "|---|---|---:|---|---:|---:|---:|"]
    for case in matrix:
        lines.append(f"| {case['scenario']} | {case['mode']} | {case['limit']} | {'да' if case['logging'] else 'нет'} | "
                     f"{case['median_ns']/1e6:.2f} | {case['median_total_ns']/1e6:.2f} | {case['speedup']:.2f} |")
    lines += ["", "## Методика и границы выводов", "",
              "`waiting`: каждый дочерний процесс ждёт 120 мс без вывода. `mixed`: 30 мс ожидания и 64 КиБ в каждый поток. "
              "`output`: без задержки, 1 МиБ в каждый поток. Задержка моделирует ожидание внешнего ресурса; это управляемый эталон, а не замер сети.", "",
              "Время диспетчера включает запуск процессов, ожидание, чтение и сохранение вывода. "
              "Полное время дополнительно включает загрузку конфигурации, отчёт, доставку логов и накладные расходы запуска программы. "
              "Логи буферизуются и записываются после измеряемой работы; последнее сравнение включает отключённые логи.", "",
              "Синхронный режим обслуживает один процесс через блокирующий poll; асинхронный использует корутины и два потока записи файлов. "
              "Поэтому разница включает стоимость этих механизмов. Для маленьких задач и интенсивной записи ускорение не гарантируется. "
              "Порядок завершения при близких длительностях зависит от ОС; содержимое и коды результатов воспроизводимы. "
              "Файлы находятся во временном каталоге ОС, fsync не выполняется; перед выводами о диске проверьте его файловую систему.", "",
              "Исходные измерения сохранены в `benchmark.json`. Повтор: `python3 tools/benchmark.py --program /path/to/process_dispatcher --output reports/benchmark.json`."]
    args.output.with_suffix(".md").write_text("\n".join(lines) + "\n")
    print(f"Verified {len(matrix) * (args.repetitions + 1)} runs; report: {args.output}")


if __name__ == "__main__":
    main()
