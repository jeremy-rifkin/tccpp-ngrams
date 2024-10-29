#include <benchmark/benchmark.h>
#include <xoshiro-cpp/XoshiroCpp.hpp>

#include "ngram.hpp"
#include "tokenization.hpp"
#include "common.hpp"

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

static void Ngrams(benchmark::State& state) {
    XoshiroCpp::Xoshiro128Plus rng(42);
    std::uint64_t count = 0;
    for (auto _ : state) {
        tokenize(dummy_messages[rng() & (dummy_messages.size() - 1)], [&](const ngram_window& container) {
            benchmark::DoNotOptimize(container.subview<1>());
            benchmark::DoNotOptimize(container.subview<2>());
            benchmark::DoNotOptimize(container.subview<3>());
            benchmark::DoNotOptimize(container.subview<4>());
            benchmark::DoNotOptimize(container.subview<5>());
            count++;
        });
        benchmark::DoNotOptimize(count);
    }
}
BENCHMARK(Ngrams);

BENCHMARK_MAIN();
