#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <ranges>

#include <ankerl/unordered_dense.h>
#include <cpptrace/cpptrace.hpp>
#include <fmt/chrono.h>
#include <fmt/format.h>
#include <libassert/assert.hpp>

#include <spdlog/spdlog.h>
#include <SQLiteCpp/SQLiteCpp.h>

#include "MessageDatabaseReader.hpp"
#include "MessageDatabaseManager.hpp"
#include "utils.hpp"

using namespace std::literals;

template<typename C>
void process_messages(MessageDatabaseReader& reader, const C& callback) {
    sys_ms last_timestamp{};
    std::uint64_t processed = 0;
    while(auto entry = reader.read()) {
        const auto& [timestamp, content] = *entry;
        ASSERT(timestamp >= last_timestamp, "Time went backwards");
        last_timestamp = timestamp;
        callback(timestamp, content);
        processed++;
        if(processed % (1024 * 1024) == 0) {
            spdlog::info("Processed {}", processed);
        }
    }
}

struct entry {
    std::uint32_t id;
    std::uint32_t count;
};

using Counts = string_map<std::uint32_t>;
using AugmentedCounts = string_map<entry>;

Counts preprocess(MessageDatabaseManager& db) {
    spdlog::info("Preprocessing");
    Counts counts;
    auto reader = db.make_message_database_reader();
    process_messages(reader, [&](sys_ms, std::string_view content) {
        ngrams(content, [&](std::string_view gram) {
            counts[gram]++;
        });
    });
    spdlog::info("Finished preprocessing");
    spdlog::info("Finished preprocessing, 1-grams with 10 or more occurrences:");
    // for(const auto& [k, v] : counts) {
    //     if(v >= 10) {
    //         fmt::println("{}\t{}", k, v);
    //     }
    // }
    return counts;
}

constexpr std::chrono::year_month agg_epoch(std::chrono::year(2017), std::chrono::January);

void aggregate(MessageDatabaseManager& db, const Counts& preprocessed_counts) {
    spdlog::info("Preparing counts");
    AugmentedCounts counts;
    std::uint32_t id = 0;
    for(const auto& [k, v] : preprocessed_counts) {
        if(v >= 20) {
            counts.emplace(k, id++);
        }
    }

    spdlog::info("Preparing db");
    if(std::filesystem::exists("test.db3")) {
        std::filesystem::remove("test.db3");
    }
    SQLite::Database aggdb{"test.db3", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE};
    aggdb.exec("CREATE TABLE ngrams (ngram_id INTEGER PRIMARY KEY, ngram TEXT, total INTEGER)");
    aggdb.exec("CREATE TABLE frequencies (months_since_epoch INTEGER, ngram_id INTEGER, frequency REAL)");
    {
        SQLite::Statement ngram_insert = SQLite::Statement{aggdb, "INSERT INTO ngrams (ngram_id, ngram, total) VALUES (?, ?, ?)"};
        SQLite::Transaction transaction(aggdb);
        for(const auto& [gram, entry] : counts) {
            ngram_insert.bind(1, entry.id);
            ngram_insert.bind(2, gram);
            ngram_insert.bind(3, preprocessed_counts.at(gram));
            ngram_insert.exec();
            ngram_insert.reset();
        }
        transaction.commit();
    }
    SQLite::Statement insert = SQLite::Statement{aggdb, "INSERT INTO frequencies (months_since_epoch, ngram_id, frequency) VALUES (?, ?, ?)"};

    spdlog::info("Aggregating");

    auto reader = db.make_message_database_reader();

    std::uint64_t total_for_month = 0;
    auto flush = [&](std::chrono::year_month date) {
        SQLite::Transaction transaction(aggdb);
        for(auto& [k, entry] : counts) {
            if(entry.count == 0) {
                continue;
            }
            auto months_since_epoch = date - agg_epoch;
            insert.bind(1, months_since_epoch.count());
            insert.bind(2, entry.id);
            insert.bind(3, entry.count / double(total_for_month));
            insert.exec();
            insert.reset();
            entry.count = 0;
        }
        transaction.commit();
        total_for_month = 0;
    };

    std::optional<std::chrono::year_month> last_year_month;

    process_messages(reader, [&](sys_ms timestamp, std::string_view content) {
        auto date = std::chrono::year_month_day(std::chrono::floor<std::chrono::days>(timestamp));
        auto year_month = std::chrono::year_month(date.year(), date.month());
        if(!last_year_month) {
            last_year_month = year_month;
        } else if(year_month != *last_year_month) {
            spdlog::info("Flush {}", timestamp);
            flush(*last_year_month);
            last_year_month = year_month;
        }
        ngrams(content, [&](std::string_view gram) {
            if(auto it = counts.find(gram); it != counts.end()) {
                it->second.count++;
                total_for_month++;
            }
        });
    });

    spdlog::info("Finished aggregating, creating indexes");
    aggdb.exec("CREATE INDEX ngrams_ngram_id ON ngrams(ngram_id);");
    aggdb.exec("CREATE INDEX ngrams_ngram ON ngrams(ngram);");
    aggdb.exec("CREATE INDEX ngrams_total ON ngrams(total);");
    aggdb.exec("CREATE INDEX frequencies_ngram_id ON frequencies(ngram_id);");
    aggdb.exec("CREATE INDEX frequencies_months_since_epoch ON frequencies(months_since_epoch);");
    spdlog::info("Finished");
}

int main() {
    spdlog::info("Starting up");
    std::ifstream auth_file("auth.txt");
    std::string auth_url{std::istreambuf_iterator<char>(auth_file), std::istreambuf_iterator<char>()};
    spdlog::info("Setting up database connection");
    MessageDatabaseManager db(auth_url);
    auto counts = preprocess(db);
    aggregate(db, counts);
}
