#include "logging/BufferedLogger.hpp"
#include "logging/JsonLogFormatter.hpp"
#include "logging/StreamLogSink.hpp"
#include "logging/FileLogSink.hpp"
#include "logging/LokiLogSink.hpp"
#include "application/LoggingSetup.hpp"
#include "input/JsonConfigLoader.hpp"

#include <boost/json.hpp>
#include <atomic>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <thread>

namespace {
using namespace compositor::logging;
void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
template <class F> void fails(F action) {
    try { action(); } catch (const std::exception&) { return; }
    throw std::runtime_error("Expected an exception");
}
LogRecord record(std::int64_t ns, std::string event = "test") {
    return {std::chrono::system_clock::time_point{std::chrono::nanoseconds{ns}}, Level::Info, std::move(event), {{"value", "quote\"\nline"}}};
}
class Collector final : public ILogSink {
public:
    std::vector<LogRecord> records;
    void write(std::span<const LogRecord> batch) override { records.insert(records.end(), batch.begin(), batch.end()); }
};
class BrokenSink final : public ILogSink {
public:
    void write(std::span<const LogRecord>) override { throw std::runtime_error("injected"); }
};
class FakeHttp final : public IHttpTransport {
public:
    std::string url, body;
    std::chrono::milliseconds timeout{};
    unsigned status = 204;
    unsigned calls = 0;
    bool broken = false;
    unsigned postJson(std::string_view u, std::string_view b, std::chrono::milliseconds t) override {
        ++calls;
        if (broken) throw std::runtime_error("transport failed");
        url = u; body = b; timeout = t; return status;
    }
};
void loggerTests() {
    auto collector = std::make_unique<Collector>();
    auto* seen = collector.get();
    std::vector<std::unique_ptr<ILogSink>> sinks;
    sinks.push_back(std::make_unique<BrokenSink>());
    sinks.push_back(std::move(collector));
    BufferedLogger logger{std::move(sinks), 2};
    auto debug = record(0); debug.level = Level::Debug;
    logger.log(debug);
    logger.log(record(1)); logger.log(record(2)); logger.log(record(3));
    require(seen->records.empty(), "No I/O before flush");
    const auto result = logger.flush();
    require(result.errors.size() == 1 && result.droppedRecords == 1 && seen->records.size() == 2,
            "Fanout continues after sink failure; overflow is counted; filtering works");
    require(logger.flush().errors.empty() && logger.flush().droppedRecords == 0, "No implicit retries or stale counts");
    logger.log(record(4)); logger.flush();
    require(seen->records.size() == 3, "Logger can be reused");

    auto concurrent = std::make_unique<Collector>();
    auto* all = concurrent.get();
    std::vector<std::unique_ptr<ILogSink>> outputs;
    outputs.push_back(std::move(concurrent));
    BufferedLogger shared{std::move(outputs), 4000};
    std::atomic<bool> stop{false};
    std::jthread flusher([&] { while (!stop) { shared.flush(); std::this_thread::yield(); } });
    {
        std::vector<std::jthread> workers;
        for (unsigned worker = 0; worker < 4; ++worker)
            workers.emplace_back([&] { for (unsigned i = 0; i < 1000; ++i) shared.log(record(i)); });
    }
    stop = true; flusher.join();
    require(shared.flush().droppedRecords == 0 && all->records.size() == 4000, "Concurrent producers and flush preserve all records");
}
void sinkTests() {
    auto formatter = std::make_shared<JsonLogFormatter>();
    std::ostringstream out;
    StreamLogSink stream(out, formatter);
    std::vector<LogRecord> records{record(20, "one\nline"), record(10)};
    stream.write(records);
    std::istringstream lines(out.str()); std::string line;
    std::getline(lines,line);
    const auto json = boost::json::parse(line);
    require(json.at("event") == "one\nline" && json.at("timestamp_ns") == "20", "JSON escaping and timestamp");
    require(json.at("fields").at("value") == "quote\"\nline", "Structured fields");
    std::ostringstream broken; broken.setstate(std::ios::badbit);
    StreamLogSink bad(broken, formatter);
    fails([&] { bad.write(records); });

    auto http = std::make_unique<FakeHttp>(); auto* transport = http.get();
    LokiLogSink loki("http://localhost:3100/loki/api/v1/push", {{"app","compositor"}},
                     std::chrono::milliseconds{150}, std::move(http), formatter);
    loki.write({}); require(transport->calls == 0, "Empty batch sends nothing");
    loki.write(records);
    const auto payload = boost::json::parse(transport->body);
    const auto& entry = payload.at("streams").as_array().at(0);
    require(entry.at("stream").at("app") == "compositor", "Loki labels");
    const auto& values = entry.at("values").as_array();
    require(values[0].as_array()[0] == "10" && values[1].as_array()[0] == "20", "Loki timestamps are sorted strings");
    require(boost::json::parse(std::string(values[1].as_array()[1].as_string())).at("event") == "one\nline", "Loki line is formatted record");
    require(transport->timeout.count() == 150 && transport->url.ends_with("/loki/api/v1/push"), "Transport arguments");
    transport->status = 500; fails([&] { loki.write(records); });
    transport->status = 260; fails([&] { loki.write(records); });
    transport->broken = true; fails([&] { loki.write(records); });
    require(transport->calls == 4, "No automatic retries");
    fails([&] { LokiLogSink invalid("http://localhost", {{"bad-label","v"}}, std::chrono::milliseconds{1}, std::make_unique<FakeHttp>(), formatter); });
}
class TempDirectory {
public:
    std::filesystem::path path;
    TempDirectory() {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        for (unsigned i=0; i<100; ++i) {
            path = std::filesystem::temp_directory_path() / ("compositor-log-" + std::to_string(stamp) + "-" + std::to_string(i));
            if (std::filesystem::create_directory(path)) return;
        }
        throw std::runtime_error("Cannot create temporary directory");
    }
    ~TempDirectory() { std::error_code error; std::filesystem::remove_all(path,error); }
};
void configAndFileTests() {
    TempDirectory directory;
    const auto path = directory.path / "config.json";
    const std::string prefix = R"({"canvas":{"width":1,"height":1,"background":[0,0,0]},"render":{"mode":"sequential","threads":1,"region":{"width":1,"height":1}},"rectangles":[],"output":{"image":"result.ppm"})";
    auto load = [&](std::string logging) {
        std::ofstream file(path); file << prefix << ",\"logging\":" << logging << '}'; file.close();
        return compositor::JsonConfigLoader{}.load(path);
    };
    const auto config = load(R"({"console":true,"file":"run.jsonl","capacity":10,"loki":{"url":"http://localhost:3100/loki/api/v1/push"}})");
    require(config.logging.file == directory.path / "run.jsonl" && config.logging.loki->timeout.count() == 2000, "Logging paths and defaults");
    for (const std::string text : {R"({"console":false})", R"({"capacity":1})", R"({"capacity":2.5})", R"({"console":1})", R"({"unknown":1})", R"({"file":"result.ppm"})", R"({"file":"config.json"})", R"({"loki":{"url":"file:///tmp/x"}})", R"({"loki":{"url":"http://localhost","labels":{"bad-name":"a"}}})"})
        fails([&] { (void)load(text); });
    const auto settings = load(R"({"file":"run.jsonl"})").logging;
    std::ostringstream console;
    for (int i=0; i<2; ++i) {
        auto logger = compositor::createLogger(settings, console);
        logger->log(record(i)); require(logger->flush().errors.empty(), "File and console delivery");
    }
    std::ifstream file(*settings.file);
    std::string line; unsigned count=0;
    while (std::getline(file,line)) { (void)boost::json::parse(line); ++count; }
    const auto consoleText = console.str();
    require(count == 2 && std::count(consoleText.begin(), consoleText.end(), '\n') == 2, "Append file and fanout across runs");
    fails([&] { FileLogSink bad(directory.path / "missing" / "run.jsonl", std::make_shared<JsonLogFormatter>()); });
}
}
int main() {
    try { loggerTests(); sinkTests(); configAndFileTests(); }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
