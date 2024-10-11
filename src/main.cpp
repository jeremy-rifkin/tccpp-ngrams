#include <fstream>

#include <cpptrace/cpptrace.hpp>
#include <cpptrace/from_current.hpp>
#include <fmt/format.h>
#include <libassert/assert.hpp>

#include <spdlog/spdlog.h>

#include "Aggregator.hpp"

using namespace std::literals;

int main() CPPTRACE_TRY {
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
