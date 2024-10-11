#ifndef AGGREGATOR_HPP
#define AGGREGATOR_HPP

#include <chrono>

#include "MessageDatabaseManager.hpp"
#include "utils.hpp"

#include <ankerl/unordered_dense.h>
#include <SQLiteCpp/SQLiteCpp.h>

using namespace std::literals;

class Aggregator {
public:
    Aggregator(MessageDatabaseManager& db) : db(db) {}

    void run();

private:
    static constexpr std::chrono::year_month agg_epoch{std::chrono::year(2017), std::chrono::January};
    // april fool's 2023 is blacklisted, the logic can be expanded later if needed
    // https://discord.com/channels/331718482485837825/331881381477089282/1091618654405283940
    static constexpr sys_ms april_fools_2023_start{1680332568s};
    // https://discord.com/channels/331718482485837825/1091622651723784222/1092186981724848138
    static constexpr sys_ms april_fools_2023_end{1680468068s};

    MessageDatabaseManager& db;
    std::optional<SQLite::Database> aggdb; // using an optional here to defer construction
    struct augmented_entry {
        std::uint32_t id;
        std::uint32_t count;
    };
    using Counts = std::tuple<
        ngram_map<1, std::uint32_t>,
        ngram_map<2, std::uint32_t>,
        ngram_map<3, std::uint32_t>,
        ngram_map<4, std::uint32_t>,
        ngram_map<5, std::uint32_t>
    >;
    using AugmentedCounts = std::tuple<
        ngram_map<1, augmented_entry>,
        ngram_map<2, augmented_entry>,
        ngram_map<3, augmented_entry>,
        ngram_map<4, augmented_entry>,
        ngram_map<5, augmented_entry>
    >;
    Counts preprocessed_counts;
    AugmentedCounts counts;

    void preprocess();
    void setup_ngram_maps();
    void setup_database();
    void populate_ngram_tables();
    void do_flush(std::chrono::year_month date, std::uint64_t total_for_month);
    void do_aggregation();
    void setup_indices();
};

#endif
