#include "dispatcher/application/Application.hpp"
#include "dispatcher/execution/Dispatchers.hpp"
#include "dispatcher/logging/BufferedLogger.hpp"
#include "dispatcher/logging/JsonLogFormatter.hpp"
#include "dispatcher/logging/LokiLogSink.hpp"
#include <boost/json.hpp>
#include <iostream>
#include <stdexcept>
#include <thread>
using namespace dispatcher;
void check(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
template <class F> void rejects(F f) {
    bool rejected = false;
    try {
        f();
    } catch (const std::exception &) {
        rejected = true;
    }
    check(rejected, "Expected rejection");
}
class MemoryLogger : public logging::ILogger {
  public:
    std::vector<logging::LogRecord> records;
    void log(logging::LogRecord r) noexcept override { records.push_back(std::move(r)); }
    logging::FlushReport flush() override { return {}; }
};
class Sink : public logging::ILogSink {
  public:
    std::vector<logging::LogRecord> records;
    bool fail = false;
    void write(std::span<const logging::LogRecord> batch) override {
        if (fail)
            throw std::runtime_error("sink failed");
        records.insert(records.end(), batch.begin(), batch.end());
    }
};
class Transport : public logging::IHttpTransport {
  public:
    unsigned status = 204;
    std::string body, url;
    std::vector<std::string> bodies;
    unsigned postJson(std::string_view u, std::string_view b, std::chrono::milliseconds) override {
        body = b;
        url = u;
        bodies.emplace_back(b);
        return status;
    }
};
class Capture : public IOutputCapture {
  public:
    explicit Capture(bool fail) : fail_(fail) {}
    void append(std::span<const char> bytes) noexcept override { bytes_ += bytes.size(); }
    CapturedStream finish() override { return {{}, bytes_, fail_ ? "injected write failure" : ""}; }

  private:
    bool fail_;
    std::uint64_t bytes_ = 0;
};
class Store : public IOutputStore {
  public:
    OutputPair open(const Job &job) override {
        if (job.id() == "openfail")
            throw std::runtime_error("injected open failure");
        return {std::make_unique<Capture>(job.id() == "writefail"), std::make_unique<Capture>(false)};
    }
};
void model() {
    rejects([] { Job("../bad", "/bin/true", {}); });
    rejects([] { Job("x", "", {}); });
    rejects([] { Job("x", "/bin/true", {std::string("a\0b", 3)}); });
    rejects([] { ExecutionPlan({}, 0); });
    rejects([] { ExecutionPlan({}, 1, 0); });
    rejects([] { ExecutionPlan({Job("x", "a", {}), Job("x", "b", {})}, 1); });
    check(ExecutionPlan({}, 1).jobs().empty(), "Empty plan is valid");
}
void loggingTest() {
    auto bad = std::make_unique<Sink>();
    bad->fail = true;
    auto good = std::make_unique<Sink>();
    auto *ptr = good.get();
    std::vector<std::unique_ptr<logging::ILogSink>> sinks;
    sinks.push_back(std::move(bad));
    sinks.push_back(std::move(good));
    logging::BufferedLogger logger(std::move(sinks), 2);
    for (int i = 0; i < 3; ++i)
        logger.log({WallClock::now(), logging::Level::Info, "event", {{"i", std::to_string(i)}}});
    check(ptr->records.empty(), "No sink I/O before flush");
    auto result = logger.flush();
    check(result.errors.size() == 1 && result.droppedRecords == 1, "Delivery errors and overflow reported");
    check(ptr->records.size() == 2 && ptr->records[1].fields.at("i") == "1", "Failure isolation and order");
    check(logger.flush().errors.empty(), "No automatic duplicate deliveries");
    auto transport = std::make_unique<Transport>();
    auto *http = transport.get();
    logging::LokiLogSink loki("http://localhost/loki/api/v1/push", {{"app", "test"}},
                              std::chrono::milliseconds(10), std::move(transport),
                              std::make_shared<logging::JsonLogFormatter>());
    std::vector<logging::LogRecord> records{
        {WallClock::now(), logging::Level::Info, "quote\"", {{"message", "line\nnext"}}}};
    loki.write(records);
    auto payload = boost::json::parse(http->body);
    auto line = payload.at("streams").as_array()[0].at("values").as_array()[0].as_array()[1].as_string();
    check(boost::json::parse(line).at("event") == "quote\"", "Loki JSON escaping");
    http->bodies.clear();
    records.clear();
    const auto now = WallClock::now();
    for (int i = 0; i < 600; ++i)
        records.push_back(
            {now - std::chrono::seconds(i), logging::Level::Info, "ordered", {{"i", std::to_string(i)}}});
    loki.write(records);
    check(http->bodies.size() == 3, "Large batch split into bounded requests");
    std::int64_t previous = 0;
    int position = 0;
    for (const auto &body : http->bodies) {
        const auto payload = boost::json::parse(body);
        for (const auto &item : payload.at("streams").as_array()[0].at("values").as_array()) {
            const auto &pair = item.as_array();
            const auto stamp = std::stoll(std::string(pair[0].as_string()));
            check(stamp > previous, "Loki timestamps strictly increasing despite clock rollback");
            previous = stamp;
            const auto record = boost::json::parse(pair[1].as_string());
            check(std::string(record.at("fields").at("i").as_string()) == std::to_string(position++),
                  "Submission order preserved");
        }
    }
    http->status = 500;
    rejects([&] { loki.write(records); });
    http->status = 260;
    rejects([&] { loki.write(records); });
}
void dispatchers(const std::filesystem::path &executable) {
    for (bool async : {false, true}) {
        MemoryLogger logger;
        Store store;
        std::unique_ptr<IDispatcher> dispatcher;
        if (async)
            dispatcher = std::make_unique<AsyncDispatcher>(store, logger);
        else
            dispatcher = std::make_unique<SyncDispatcher>(store, logger);
        ExecutionPlan plan({Job("openfail", executable, {"--fixture"}),
                            Job("writefail", executable, {"--fixture", "--bytes", "1048576"}),
                            Job("good", executable, {"--fixture", "--stdout", "ok"})},
                           2, 4096);
        auto r = dispatcher->run(plan);
        check(r.results.size() == 3 && r.completions.size() == 3, "Every job gets a result");
        check(r.results[0].status == JobStatus::IoFailed, "Output creation failure");
        check(r.results[1].status == JobStatus::IoFailed && r.results[1].exitCode == 0,
              "Capture failure preserves exit code");
        check(r.results[1].standardError.bytes == 1048576, "Continue draining after write failure");
        check(r.results[2].status == JobStatus::Succeeded, "Failure does not cancel other jobs");
        check(r.peakConcurrency <= (async ? 2U : 1U), "Concurrency bound");
        check(dispatcher->run(ExecutionPlan({}, 2)).successful(), "Dispatcher reusable, empty plan");
    }
}
class FakeDispatcher : public IDispatcher {
  public:
    bool called = false;
    RunReport run(const ExecutionPlan &) override {
        called = true;
        return {};
    }
};
class FakeWriter : public IReportWriter {
  public:
    bool called = false, fail = false;
    void write(const RunReport &) override {
        called = true;
        if (fail)
            throw std::runtime_error("report failure");
    }
};
void application() {
    FakeDispatcher dispatcher;
    FakeWriter writer;
    MemoryLogger logger;
    Application app(dispatcher, writer, logger);
    check(app.run(ExecutionPlan({}, 1)) == 0 && dispatcher.called && writer.called,
          "Application uses interfaces");
    writer.fail = true;
    rejects([&] { app.run(ExecutionPlan({}, 1)); });
    check(logger.records.back().event == "program_finished" &&
              logger.records.back().fields.at("exit_code") == "2",
          "Failed run logged");
}
#ifdef _WIN32
int wmain(int argc, wchar_t **argv)
#else
int main(int argc, char **argv)
#endif
{
    try {
        model();
        loggingTest();
        application();
        if (argc == 2)
            dispatchers(argv[1]);
        else
            throw std::runtime_error("Fixture executable required");
        std::cout << "Model, logging, application and both dispatcher contracts passed\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
