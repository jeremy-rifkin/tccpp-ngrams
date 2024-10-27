#include "Aggregator.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <ranges>
#include <stdexcept>

#include "constants.hpp"
#include "MessageDatabaseReader.hpp"
#include "MessageDatabaseManager.hpp"
#include "utils.hpp"
#include "utils/sha.hpp"
#include "utils/random.hpp"

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <libassert/assert.hpp>
#include <spdlog/spdlog.h>
#include <duckdb.hpp>
#include <openssl/evp.h>
#include <xoshiro-cpp/XoshiroCpp.hpp>

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

    spdlog::info("Preparing ngram maps");
    setup_ngram_maps();

    spdlog::info("Preparing db");
    setup_database();
    spdlog::info("Populating ngram tables");
    populate_ngram_tables();

    spdlog::info("Aggregating");
    do_aggregation();

    spdlog::info("Finished");
}

void Aggregator::preprocess() {
    // TODO: The preprocessed_counts map here is absolutely massive, many GiB's. This is something worth
    // reconsidering later, e.g. by trimming periodically or by using some streaming setup to better estimate
    // the ngrams we care about.
    auto reader = db.make_message_database_reader();
    process_messages(reader, [&](sys_ms timestamp, std::string_view content) {
        if(blacklisted_timestamp(timestamp)) {
            return;
        }
        ngrams(content, [&](const ngram_window& ngram) {
            indexinator<ngram_max_width>([&] <auto I> {
                if(auto value = ngram.subview<I + 1>()) {
                    std::get<I>(preprocessed_counts)[*value]++;
                }
            });
        });
    });
    indexinator<ngram_max_width>([&] <auto I> {
        spdlog::info("Preprocessed counts for {}-grams: {}", I + 1, std::get<I>(preprocessed_counts).size());
    });
}

void Aggregator::setup_ngram_maps() {
    std::uint32_t id = 0;
    indexinator<ngram_max_width>([&] <auto I> {
        for(const auto& [k, v] : std::get<I>(preprocessed_counts)) {
            if(v >= minimum_occurrences) {
                // this is overkill
                std::get<I>(counts).emplace(k, augmented_entry{id++, 0, make_xoroshiro128plus(sha256(k, nonce))});
                spdlog::debug("{}\t{}", k, v);
            }
        }
    });
}

void Aggregator::setup_database() {
    if(std::filesystem::exists("ngrams.duckdb")) {
        std::filesystem::remove("ngrams.duckdb");
    }
    aggdb.emplace("ngrams.duckdb");
    con.emplace(*aggdb);
    do_query("CREATE TABLE frequencies (months_since_epoch INTEGER, ngram_id INTEGER, frequency REAL)");
    indexinator<ngram_max_width>([&] <auto I> {
        auto text_columns = std::ranges::iota_view{std::size_t(0), I + 1} | std::views::transform([](auto i) {
            return fmt::format("gram_{}", i);
        });
        do_query(
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
        duckdb::Appender appender(*con, fmt::format("ngrams_{}", I + 1));
        for(const auto& [ngram, entry] : std::get<I>(counts)) {
            [&]<std::size_t... NI>(std::index_sequence<NI...>) {
                appender.AppendRow(
                    int64_t(entry.id),
                    (duckdb::Value(ngram[NI]))...,
                    int64_t(std::get<I>(preprocessed_counts).at(ngram))
                );
            }(std::make_index_sequence<I + 1>{});
        }
    });
}

void Aggregator::do_flush(std::chrono::year_month date, std::uint64_t total_for_month) {
    duckdb::Appender appender(*con, "frequencies");
    indexinator<ngram_max_width>([&] <auto I> {
        for(auto& [k, entry] : std::get<I>(counts)) {
            if(entry.count == 0) {
                continue;
            }
            auto months_since_epoch = date - agg_epoch;
            double frequency = entry.count / double(total_for_month);
            frequency += frequency * 0.01 * random_double(entry.noise_source());
            appender.AppendRow(
                months_since_epoch.count(),
                int64_t(entry.id),
                frequency
            );
            entry.count = 0;
        }
    });
}

void Aggregator::do_aggregation() {
    std::uint64_t total_for_month = 0;
    std::optional<std::chrono::year_month> last_year_month;
    auto reader = db.make_message_database_reader();
    process_messages(reader, [&](sys_ms timestamp, std::string_view content) {
        if(blacklisted_timestamp(timestamp)) {
            return;
        }
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

bool Aggregator::blacklisted_timestamp(sys_ms timestamp) {
    return timestamp >= april_fools_2023_start && timestamp <= april_fools_2023_end;
}

void Aggregator::do_query(const std::string& query) {
    if(const auto result = con->Query(query); result->HasError()) {
        throw std::runtime_error(result->GetError());
    }
}
