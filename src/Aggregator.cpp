#include "Aggregator.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <ranges>

#include "MessageDatabaseReader.hpp"
#include "MessageDatabaseManager.hpp"
#include "utils.hpp"

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <libassert/assert.hpp>
#include <spdlog/spdlog.h>
#include <SQLiteCpp/SQLiteCpp.h>

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

void Aggregator::run() {
    spdlog::info("Preprocessing");
    preprocess();
    spdlog::info("Finished preprocessing");
    spdlog::info("Finished preprocessing, 1-grams with 10 or more occurrences:");
    // indexinator<ngram_max_width>([&] <auto I> {
    //     for(const auto& [k, v] : std::get<I>(counts)) {
    //         if(v >= 20) {
    //             fmt::println("{}\t{}", k, v);
    //         }
    //     }
    // });

    spdlog::info("Preparing ngram maps");
    setup_ngram_maps();

    spdlog::info("Preparing db");
    setup_database();
    populate_ngram_tables();

    spdlog::info("Aggregating");
    do_aggregation();

    spdlog::info("Finished aggregating, creating indexes");
    setup_indices();
    spdlog::info("Finished");
}

void Aggregator::preprocess() {
    // TODO: The preprocessed_counts map here is absolutely massive, many GiB's. This is something worth
    // reconsidering later, e.g. by trimming periodically or by using some streaming setup to better estimate
    // the ngrams we care about.
    auto reader = db.make_message_database_reader();
    process_messages(reader, [&](sys_ms, std::string_view content) {
        ngrams(content, [&](const ngram_window& ngram) {
            indexinator<ngram_max_width>([&] <auto I> {
                if(auto value = ngram.subview<I + 1>()) {
                    std::get<I>(preprocessed_counts)[*value]++;
                }
            });
        });
    });
}

void Aggregator::setup_ngram_maps() {
    std::uint32_t id = 0;
    indexinator<ngram_max_width>([&] <auto I> {
        for(const auto& [k, v] : std::get<I>(preprocessed_counts)) {
            if(v >= 20) {
                std::get<I>(counts).emplace(k, id++);
            }
        }
    });
}

void Aggregator::setup_database() {
    spdlog::info("Preparing db");
    if(std::filesystem::exists("ngrams.db3")) {
        std::filesystem::remove("ngrams.db3");
    }
    aggdb.emplace("ngrams.db3", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    aggdb->exec("CREATE TABLE frequencies (months_since_epoch INTEGER, ngram_id INTEGER, frequency REAL)");
    indexinator<ngram_max_width>([&] <auto I> {
        auto text_columns = std::ranges::iota_view{std::size_t(0), I + 1} | std::views::transform([](auto i) {
            return fmt::format("gram_{}", i);
        });
        aggdb->exec(
            fmt::format(
                "CREATE TABLE ngrams_{} (ngram_id INTEGER PRIMARY KEY, {}, total INTEGER)",
                I + 1,
                fmt::join(
                    text_columns | std::views::transform([](auto&& name){ return fmt::format("{} TEXT", name); }),
                    ", "
                )
            )
        );
    });
}

void Aggregator::populate_ngram_tables() {
    indexinator<ngram_max_width>([&] <auto I> {
        auto text_columns = std::ranges::iota_view{std::size_t(0), I + 1} | std::views::transform([](auto i) {
            return fmt::format("gram_{}", i);
        });
        auto query = fmt::format(
            "INSERT INTO ngrams_{} (ngram_id, {}, total) VALUES (?, {}, ?)",
            I + 1,
            fmt::join(text_columns, ", "),
            fmt::join(text_columns | std::views::transform([](auto&&){ return '?'; }), ", ")
        );
        SQLite::Statement ngram_insert = SQLite::Statement{*aggdb, query};
        SQLite::Transaction transaction(*aggdb);
        for(const auto& [ngram, entry] : std::get<I>(counts)) {
            ngram_insert.bind(1, entry.id);
            for(const auto& [i, gram] : std::views::enumerate(ngram)) {
                ngram_insert.bind(2 + i, gram);
            }
            ngram_insert.bind(2 + ngram.size(), std::get<I>(preprocessed_counts).at(ngram));
            ngram_insert.exec();
            ngram_insert.reset();
        }
        transaction.commit();
    });
}

void Aggregator::do_flush(std::chrono::year_month date, std::uint64_t total_for_month) {
    SQLite::Statement frequency_insert {
        *aggdb,
        "INSERT INTO frequencies (months_since_epoch, ngram_id, frequency) VALUES (?, ?, ?)"
    };
    SQLite::Transaction transaction(*aggdb);
    indexinator<ngram_max_width>([&] <auto I> {
        for(auto& [k, entry] : std::get<I>(counts)) {
            if(entry.count == 0) {
                continue;
            }
            auto months_since_epoch = date - agg_epoch;
            frequency_insert.bind(1, months_since_epoch.count());
            frequency_insert.bind(2, entry.id);
            frequency_insert.bind(3, entry.count / double(total_for_month));
            frequency_insert.exec();
            frequency_insert.reset();
            entry.count = 0;
        }
    });
    transaction.commit();
}

void Aggregator::do_aggregation() {
    std::uint64_t total_for_month = 0;
    std::optional<std::chrono::year_month> last_year_month;
    auto reader = db.make_message_database_reader();
    process_messages(reader, [&](sys_ms timestamp, std::string_view content) {
        auto date = std::chrono::year_month_day(std::chrono::floor<std::chrono::days>(timestamp));
        auto year_month = std::chrono::year_month(date.year(), date.month());
        if(!last_year_month) {
            last_year_month = year_month;
        } else if(year_month != *last_year_month) {
            spdlog::info("Flush {}", timestamp);
            do_flush(*last_year_month, total_for_month);
            total_for_month = 0;
            last_year_month = year_month;
        }
        ngrams(content, [&](const ngram_window& ngram) {
            indexinator<ngram_max_width>([&] <auto I> {
                if(auto value = ngram.subview<I + 1>()) {
                    auto& the_counts = std::get<I>(counts);
                    if(auto it = the_counts.find(*value); it != the_counts.end()) {
                        it->second.count++;
                        if(I == 0) {
                            total_for_month++;
                        }
                    }
                }
            });
        });
    });
}

void Aggregator::setup_indices() {
    indexinator<ngram_max_width>([&] <auto I> {
        auto text_columns = std::ranges::iota_view{std::size_t(0), I + 1} | std::views::transform([](auto i) {
            return fmt::format("gram_{}", i);
        });
        aggdb->exec(fmt::format("CREATE INDEX ngrams_{0}_ngram_id ON ngrams_{0}(ngram_id);", I + 1));
        for(const auto& column : text_columns) {
            aggdb->exec(fmt::format("CREATE INDEX ngrams_{0}_{1} ON ngrams_{0}({1});", I + 1, column));
        }
        aggdb->exec(fmt::format("CREATE INDEX ngrams_{0}_total ON ngrams_{0}(total);", I + 1));
    });
    aggdb->exec("CREATE INDEX frequencies_ngram_id ON frequencies(ngram_id);");
    aggdb->exec("CREATE INDEX frequencies_months_since_epoch ON frequencies(months_since_epoch);");
}
