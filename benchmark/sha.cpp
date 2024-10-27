#include <vector>

#include <benchmark/benchmark.h>
#include <xoshiro-cpp/XoshiroCpp.hpp>
#include <libassert/assert.hpp>

#include "utils.hpp"
#include "utils/sha.hpp"
#include "common.hpp"

// using the first 32 of each for trivial modulo
constexpr size_t ngrams_count = 32;
std::tuple<
    std::vector<ngram<1>>,
    std::vector<ngram<2>>,
    std::vector<ngram<3>>,
    std::vector<ngram<4>>,
    std::vector<ngram<5>>
> ngrams_for_bench;

[[maybe_unused]] auto init_ngrams_for_bench = [] {
    for(const auto& message : dummy_messages) {
        ngrams(message, [&](const ngram_window& container) {
            if(auto x = container.subview<1>()) { std::get<0>(ngrams_for_bench).emplace_back(*x); }
            if(auto x = container.subview<2>()) { std::get<1>(ngrams_for_bench).emplace_back(*x); }
            if(auto x = container.subview<3>()) { std::get<2>(ngrams_for_bench).emplace_back(*x); }
            if(auto x = container.subview<4>()) { std::get<3>(ngrams_for_bench).emplace_back(*x); }
            if(auto x = container.subview<5>()) { std::get<4>(ngrams_for_bench).emplace_back(*x); }
        });
    }
    ASSERT(std::get<0>(ngrams_for_bench).size() >= ngrams_count);
    ASSERT(std::get<1>(ngrams_for_bench).size() >= ngrams_count);
    ASSERT(std::get<2>(ngrams_for_bench).size() >= ngrams_count);
    ASSERT(std::get<3>(ngrams_for_bench).size() >= ngrams_count);
    ASSERT(std::get<4>(ngrams_for_bench).size() >= ngrams_count);
    return 0;
} ();

static void Nop(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(1);
    }
}
BENCHMARK(Nop);

static void Rng(benchmark::State& state) {
    XoshiroCpp::Xoshiro128Plus rng(42);
    for (auto _ : state) {
        benchmark::DoNotOptimize(rng());
    }
}
BENCHMARK(Rng);

constexpr std::string_view benchmark_nonce = "benchmark";

#define SHA_NGRAM_BENCH(N) \
    static void Sha_##N##gram(benchmark::State& state) { \
        XoshiroCpp::Xoshiro128Plus rng(42); \
        for (auto _ : state) { \
            benchmark::DoNotOptimize( \
                sha256(std::get<N - 1>(ngrams_for_bench)[rng() % ngrams_count], benchmark_nonce) \
            ); \
        } \
    } \
    BENCHMARK(Sha_##N##gram);

SHA_NGRAM_BENCH(1);
SHA_NGRAM_BENCH(2);
SHA_NGRAM_BENCH(3);
SHA_NGRAM_BENCH(4);
SHA_NGRAM_BENCH(5);

BENCHMARK_MAIN();
