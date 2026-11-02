# Лабораторные работы по ООП

Общий репозиторий лабораторных работ по ООП на C++. Каждое задание собирается отдельно.

## Работы

| Лабораторная | Задание | Каталог |
|---|---|---|
| 1 | Решатель уравнений, вариант 2 | [equation_solver](lab_1/equation_solver/) |
| 1 | Генерация и прохождение лабиринта, вариант 2 | [maze](lab_1/maze/) |
| 2 | Компоновщик графических объектов, вариант 4 | [compositor](lab_2/compositor/) |

Новые работы размещаются по лабораторным в `lab_3/`, `lab_4/` и так далее.

В компоновщике реализованы последовательный, незащищённый параллельный и
защищённый параллельный режимы, загрузка JSON-конфигурации, сохранение PPM,
логирование, тесты и отдельный C++-бенчмарк. Описание архитектуры и запуска —
в [README компоновщика](lab_2/compositor/README.md).

## Сборка и тестирование

Нужны компилятор с поддержкой C++20 и CMake 3.20 или новее. Задания первой
лабораторной используют GoogleTest: CMake найдёт установленный пакет либо
загрузит его при первой конфигурации (для этого нужен интернет). Для компоновщика
нужен установленный Boost.JSON версии 1.75 или новее с CMake-конфигурацией.

Из корня репозитория:

```sh
cmake -S lab_1/equation_solver -B build/equation_solver -DCMAKE_BUILD_TYPE=Release
cmake --build build/equation_solver --parallel
ctest --test-dir build/equation_solver --output-on-failure

cmake -S lab_1/maze -B build/maze -DCMAKE_BUILD_TYPE=Release
cmake --build build/maze --parallel
ctest --test-dir build/maze --output-on-failure

cmake -S lab_2/compositor -B build/compositor -DCMAKE_BUILD_TYPE=Release
cmake --build build/compositor --parallel
ctest --test-dir build/compositor --output-on-failure
```

Запуск компоновщика и бенчмарка из корня репозитория:

```sh
./build/compositor/compositor lab_2/compositor/configs/example.json
./build/compositor/compositor_bench lab_2/compositor/configs/benchmark-example.json
```

Оба результата сохраняются по путям из конфигураций. Сборка Loki HTTP-приёмника
необязательна и требует libcurl и отдельно запущенного сервера Loki; подробности
есть в README компоновщика. Для сборок Visual Studio используйте `--config Release`
и запускайте `.exe` из соответствующей папки конфигурации.

Инструкции по вводу и запуску первой лабораторной находятся в README каждого
задания. Корневой CI монорепозитория пока не настроен.

## Перенос первой лабораторной

Сохранена история исходного репозитория `DomDomych/lab1_oop`.
Исходники, тесты и скрипты перенесены в `lab_1/` без изменения алгоритмов.
Вложенные `.github/workflows/windows-release.yml` сохранены как исходные
настройки отдельных проектов; они не запускаются автоматически в монорепозитории.
Ссылки на Windows-релизы в README заданий ведут к прежним репозиториям.
