#include <fstream>

#include <cpptrace/cpptrace.hpp>
#include <cpptrace/from_current.hpp>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <libassert/assert.hpp>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>
#include <lyra/lyra.hpp>

#include "Aggregator.hpp"

using namespace std::literals;

template<> struct fmt::formatter<lyra::cli> : ostream_formatter {};

int main(int argc, char** argv) CPPTRACE_TRY {
    std::string log_level = "info";
    bool show_help = false;
    auto cli = lyra::cli()
        | lyra::help(show_help)
        | lyra::opt(log_level, "log level" )["--log-level"]("Spdlog log level")
            .choices("trace", "debug", "info", "warn", "err", "critical", "off");
    if(auto result = cli.parse({ argc, argv }); !result) {
        fmt::println(stderr, "Error in command line: {}", result.message());
        return 1;
    }
    if(show_help) {
        fmt::println("{}", cli);
        return 0;
    }
    spdlog::set_level(spdlog::level::from_str(log_level));

    spdlog::info("Starting up");
    std::ifstream auth_file("auth.txt");
    std::string auth_url{std::istreambuf_iterator<char>(auth_file), std::istreambuf_iterator<char>()};

    spdlog::info("Setting up database connection");
    MessageDatabaseManager db(auth_url);

    Aggregator{db}.run();
} CPPTRACE_CATCH(const std::exception& e) {
    fmt::println(stderr, "Caught exception {}: {}", cpptrace::demangle(typeid(e).name()), e.what());
    cpptrace::from_current_exception().print();
}
