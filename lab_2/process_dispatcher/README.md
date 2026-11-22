# Диспетчер выполнения программ

Лабораторная 2, задание 2, вариант 4. Консольное приложение на C++20 запускает произвольный список программ с аргументами, сохраняет stdout, stderr и коды завершения. Доступны синхронный режим и асинхронный режим на Boost.Asio с корутинами. Есть JSON-отчёт, логирование в консоль, JSONL-файл и Loki.

## Готовая сборка для Windows

[Скачать Windows x64](https://github.com/DomDomych/oop-labs/releases/tag/dispatcher-v1.0.0).
Нужна Windows 10/11 x64. Распакуйте весь `process-dispatcher-windows-x64.zip`
в доступную для записи папку. Boost, libcurl и среда исполнения C++ включены
статически: устанавливать компилятор или библиотеки для запуска не требуется.

- `demo-async.cmd` — демонстрация параллельного запуска; окно остаётся открытым.
- `demo-sync.cmd` — тот же список программ последовательно.
- `run.cmd` — пример без паузы; также принимает путь к своему JSON и `--mode`.
- `output/report.json` — отчёт; `output/run.jsonl` — журнал примера.
- `tests.cmd` — модульные, интеграционные и эталонные проверки; нужен Python 3.11+ в PATH.

Из PowerShell в распакованной папке:

```powershell
.\run.cmd
.\run.cmd .\configs\example.json --mode sync
.\process_dispatcher.exe .\configs\loki.json
.\tests.cmd
Get-FileHash ..\process-dispatcher-windows-x64.zip -Algorithm SHA256
```

Сверьте хеш архива с приложенным к релизу `SHA256SUMS.txt`. `configs/loki.json`
требует запущенный Loki; обычный пример работает автономно. Конфигурации и пути
в JSON — UTF-8; в Windows удобно писать пути с `/`, например `C:/Tools/app.exe`.
Пробелы и кириллица в путях поддерживаются. Формат командной строки дочернего
процесса соответствует правилам Windows CRT; программы с собственным разбором
командной строки могут интерпретировать её иначе.

### Сборка Windows из исходников

Нужны Visual Studio 2022 с компонентом «Разработка классических приложений на C++»,
Windows SDK, PowerShell 7, CMake 3.20–3.31, Git и Python 3.11+ для тестов. Из корня монорепозитория:

```powershell
git clone https://github.com/microsoft/vcpkg.git build/vcpkg
git -C build/vcpkg checkout ef7dbf94b9198bc58f45951adcf1f041fcbc5ea0
.\build\vcpkg\bootstrap-vcpkg.bat -disableMetrics
cmake -S lab_2/process_dispatcher -B build/dispatcher -A x64 `
  "-DCMAKE_TOOLCHAIN_FILE=$PWD/build/vcpkg/scripts/buildsystems/vcpkg.cmake" `
  -DVCPKG_TARGET_TRIPLET=x64-windows-static-release `
  "-DVCPKG_OVERLAY_TRIPLETS=$PWD/lab_2/process_dispatcher/windows" `
  -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded -DBUILD_TESTING=ON
cmake --build build/dispatcher --config Release --parallel 2
ctest --test-dir build/dispatcher -C Release --output-on-failure
.\lab_2\process_dispatcher\windows\package.ps1 -BuildDirectory "$PWD/build/dispatcher" -OutputDirectory "$PWD/build/dist"
```

Версии зависимостей закреплены в `vcpkg.json`: Boost 1.88 (API Process v2), curl
8.14.1. TLS в Windows использует системный Schannel. Скрипт упаковки включает
лицензии зависимостей и проверяет уже распакованный архив в папке с кириллицей
и пробелами. GitHub Actions выполняет эти же шаги на Windows Server 2022;
тег `dispatcher-v*` публикует архив только после успешных проверок.

## Сборка и запуск Linux

Поддерживаются Linux и Windows. Для сборки Linux Нужны CMake 3.20+, компилятор C++20, Boost 1.83 или 1.88 (JSON, Filesystem, System, Process), libcurl с заголовками. Для тестов нужен Python 3.11+, дополнительных Python-пакетов нет. В Debian/Ubuntu зависимости: `build-essential cmake libboost-json-dev libboost-filesystem-dev libboost-system-dev libcurl4-openssl-dev python3`; для Linux проверена версия Boost 1.83; Windows-релиз использует 1.88.

Из корня репозитория:

```sh
cmake -S lab_2/process_dispatcher -B build/process_dispatcher -DCMAKE_BUILD_TYPE=Release
cmake --build build/process_dispatcher --parallel 3
ctest --test-dir build/process_dispatcher --output-on-failure
./build/process_dispatcher/process_dispatcher lab_2/process_dispatcher/configs/example.json
./build/process_dispatcher/process_dispatcher lab_2/process_dispatcher/configs/example.json --mode sync
```

Если заголовки libcurl установлены в нестандартном месте, передайте `-DCURL_INCLUDE_DIR=/path/to/include` и `-DCURL_LIBRARY=/path/to/libcurl.so` при конфигурации. В текущем рабочем окружении использованы уже имевшиеся заголовки из `build/local-deps/usr/include/x86_64-linux-gnu` и системная `/usr/lib/x86_64-linux-gnu/libcurl.so.4`.

Отчёт примера: `lab_2/process_dispatcher/output/report.json`. Файлы вывода лежат в отдельном подкаталоге `report.json.outputs/run-.../`; новый запуск не перезаписывает старые захваты. Сам JSON-отчёт заменяется атомарно, JSONL-журнал дописывается. Для IDE предусмотрена цель `run_dispatcher_demo`.

## Конфигурация

```json
{
  "mode": "async",
  "max_processes": 2,
  "block_size": 65536,
  "report": "result/report.json",
  "output_directory": "result/streams",
  "logging": {
    "console": true,
    "file": "result/run.jsonl"
  },
  "jobs": [
    {"id": "example", "executable": "/usr/bin/printf", "arguments": ["%s", "Hello\n"]}
  ]
}
```

Пути разрешаются относительно конфигурационного файла. `executable` — путь к файлу; поиска по PATH и разбора команды оболочкой нет. Каждый элемент `arguments` передаётся отдельным аргументом, включая пустые строки, пробелы и кавычки. `@self` означает исполняемый файл диспетчера и нужен только для переносимых учебных примеров. Обычные программы запускаются тем же механизмом.

Обязательные поля: `mode`, `max_processes`, `report`, `jobs`; у задания — `id`, `executable`, `arguments`. Неизвестные поля и неверные типы отклоняются. ID уникальны, содержат 1–128 ASCII-букв, цифр, `_`, `-`. Допустимы 0–10000 заданий, лимит 1–256 процессов, блок 1–1048576 байт. Эти ограничения явно проверяются; размер конфигурации ограничен 8 МиБ. Пустой список корректен. Отсутствующая программа превращается в результат `launch_failed`, остальные задания выполняются.

В `sync` фактически работает один дочерний процесс, даже если `max_processes` больше единицы. В `async` лимит действует на задания от запуска до полного сбора вывода. Поэтому одновременно живых процессов также не больше лимита. `stdin` дочерних процессов подключён к `/dev/null` (Linux) или `NUL` (Windows); задача рассчитана на неинтерактивные программы.

## Результаты и ошибки

`results` в отчёте идут в порядке входного списка. Каждый результат содержит ID, статус, nullable-код завершения, время начала/конца, длительность и описания stdout/stderr: путь, количество сохранённых байтов, ошибку записи/чтения. Содержимое сохраняется как байты, без перекодирования; корректны пустые и двоичные потоки. Один процесс может завершиться неуспешно и всё равно иметь полностью сохранённый вывод.

Статусы: `succeeded`, `nonzero_exit`, `launch_failed`, `io_failed`, `wait_failed`. При сигнале сохраняется код, возвращённый Boost.Process (для POSIX это номер сигнала); он ненулевой. Ошибка запуска отличается от ненулевого кода: у неё `exit_code: null`. При ошибке записи канал продолжают вычитывать, чтобы дочерний процесс не завис на заполненной трубе.

`completions` содержит последовательные номера и порядок обработки уведомлений о завершении процессов. В асинхронном режиме событие `process_exited` фиксируется сразу после `async_wait`, даже если последние блоки вывода ещё сохраняются. `job_finished` в логах означает завершение всей корутины задания, включая сохранение вывода. Поэтому порядки этих двух видов событий могут различаться.

Коды выхода диспетчера:

| Код | Значение |
|---:|---|
| 0 | Все задания и доставка логов успешны |
| 2 | Ошибка конфигурации, подготовки или записи отчёта; исключительная инфраструктурная ошибка |
| 3 | Есть неуспешные задания; отчёт сохранён |
| 4 | Задания успешны, но хотя бы один приёмник логов не принял данные |

Если одновременно есть ошибка заданий и логирования, возвращается 3, а ошибка доставки печатается в stderr. При ошибках конфигурации до создания логгера диагностика выводится в stderr.

## Логи и Loki

Контракты `ILogger`, `ILogSink`, `ILogFormatter`, `IHttpTransport` разделяют буферизацию, приёмник, формат и HTTP. Подробности — в [архитектуре](docs/architecture.md).

По умолчанию включена консоль. `logging.file` включает JSONL-файл, `logging.loki` — Loki; приёмники можно комбинировать. Чтобы отключить логи для отдельного эксперимента, задайте только `"logging": {"console": false}`.

```json
"loki": {
  "url": "http://127.0.0.1:3100/loki/api/v1/push",
  "labels": {"app": "process_dispatcher"},
  "timeout_ms": 2000
}
```

Полный пример: `configs/loki.json`. Локальный сервер:

```sh
docker compose -f lab_2/process_dispatcher/observability/compose.yaml up -d
./build/process_dispatcher/process_dispatcher lab_2/process_dispatcher/configs/loki.json
```

В Grafana с источником данных Loki запрос: `{app="process_dispatcher"} | json`. Можно использовать уже запущенный Loki от компоновщика на порту 3100 вместо второго сервера. Остановка отдельного стенда: `docker compose -f lab_2/process_dispatcher/observability/compose.yaml down`.

События `program_started` и `program_finished` описывают выполнение приложения после чтения конфигурации и подготовки зависимостей; последнее содержит длительность и код. `dispatch_started` / `dispatch_finished` выделяют время исполнения заданий. Длительности измеряются монотонными часами, даты — системными. Записи копятся в ограниченном буфере (два события на задание + запас) и доставляются после работы, поэтому сеть и форматирование не входят в измеряемое время диспетчера. Это пакетная доставка: во время долгого запуска логи пока остаются в памяти.

Loki получает пакеты не более 256 записей, с ограничением объёма строк. Порядок поступления сохраняется даже при переводе системных часов назад: индексные timestamps Loki монотонно повышаются, исходные даты остаются в JSON. Успех — HTTP 204; ошибки сервера, тайм-аут и HTTP 260 диагностируются. Каждый HTTP-запрос ограничен `timeout_ms`; автоматических повторов нет, чтобы не дублировать частично доставленный пакет. Сбой одного приёмника не мешает остальным. Локальный стенд работает без аутентификации; HTTPS поддерживается libcurl.

Автономная проверка настоящего сервера без Docker (понадобится распакованный бинарник Loki):

```sh
python3 lab_2/process_dispatcher/tools/verify_loki.py \
  --program build/process_dispatcher/process_dispatcher \
  --loki-bin /path/to/loki-linux-amd64 \
  --report lab_2/process_dispatcher/reports/loki-check.json
```

## Проверки и исследование

`tools/check_reference.py` независимо запускает эталонные задания через Python `subprocess.run`, затем сравнивает с обоими C++-режимами коды, статусы, размеры и SHA-256 обоих потоков. В эталоне есть задержка, ненулевой код, ошибка запуска, пустой и двоичный вывод. Проверка включена в CTest; сохранённый результат — `reports/reference-check.json`.

Тесты проверяют общие контракты обоих диспетчеров, модель, подмену компонентов приложения, изоляцию ошибок приёмников и выходных файлов, размер буфера, пакетирование Loki, перевод часов назад, реальные дочерние процессы, коды/сигналы, раннее закрытие канала, лимиты, порядок завершений, буквальную передачу аргументов, большие двоичные потоки и настоящий HTTP-транспорт. При HTTP-тестах требуется возможность открыть локальный сокет.

[Результаты исследования](reports/benchmark.md), [сырые измерения](reports/benchmark.json) и [порядок защиты](docs/defense.md). Повтор исследования:

```sh
python3 lab_2/process_dispatcher/tools/benchmark.py \
  --program build/process_dispatcher/process_dispatcher \
  --output lab_2/process_dispatcher/reports/benchmark.json
```

Проверка памяти и неопределённого поведения:

```sh
cmake -S lab_2/process_dispatcher -B build/process_dispatcher-sanitize \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'
cmake --build build/process_dispatcher-sanitize --parallel 3
ctest --test-dir build/process_dispatcher-sanitize --output-on-failure
```

Синхронный адаптер использует `poll` в Linux и два блокирующих читателя каналов в Windows, при этом запускает строго один дочерний процесс за раз. Оба режима используют Boost.Process v2. Платформенные операции изолированы в `src/platform`; модель и интерфейсы исполнения общие. Процессы должны самостоятельно завершаться; управление деревьями потомков и тайм-ауты заданий не входят в текущий контракт. Ресурсы дочерних процессов освобождаются через RAII, завершившиеся процессы собираются через `wait`.
