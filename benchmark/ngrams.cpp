#include <array>
#include <bit>
#include <string_view>

#include <benchmark/benchmark.h>
#include <xoshiro-cpp/XoshiroCpp.hpp>

#include "utils.hpp"

using namespace std::literals;

constexpr std::array messages = {
    "this is what i have to check the size"sv,
    "cppreference\nhttps://en.cppreference.com/w/cpp/compiler_support/26"sv,
    "unfortunately we still get a `heap-use-after-free` if we use `new` before the allocator templates are declared"sv,
    "Every proved model could just be an approximation for the metrics that us as humans can comprehend, but that approximation could not be tied to the universe's actual mechanics"sv,
    "struct Node* next;"sv,
    "rain#4000"sv,
    ";compile gsnapshot -std=c++23"sv,
    "like really really small"sv,
    "I guess"sv,
    "is unsigned bit-field wrapping defined?"sv,
    "it was long ago"sv,
    "what is an `RL` though lol"sv,
    "That works"sv,
    "\"put size in memory around the memory\" is extremely nebulous if you do it blindly"sv,
    "you play fortnite?"sv,
    "```cpp\n#define NILAI_CALLABLE_CONCEPT(Concept)\\\n    []<typename... Ts>() consteval\\\n    {\\\n        return Concept<Ts...>;\\\n    }\n#define NILAI_CALL_CONCEPT_WITH_TYPE(Concept, type, ...)\\\n    NILAI_CALLABLE_CONCEPT(Concept).template operator()<type __VA_OPT__(,) __VA_ARGS__>()\n```\nthis (should)  give you a callable function that you can then static_assert directly. Feel free to just take that and do what you want with it"sv,
    "was that you?"sv,
    "MAY GOD BE WITH ME I NEVER WANT TO HAVE THIS HAPPEN TO ME"sv,
    "it's infinite recursion"sv,
    "yeah, you can just move it (shallow copy) with ="sv,
    "Wait so when I turned 88 to binary, I get 1011000, which is 7 bits. Do I need to do 0101 1000 before I negate? Or do I negate and then extend??"sv,
    "Using boost? Nah use bool 👌"sv,
    "<@!500000000000000000> just do web dev, you'll know what you need as you get more experienced."sv,
    "like this"sv,
    "doesnt that defeat the purpose of pch"sv,
    "`using namespace std;` no more theme <:ez:526892488208809984>"sv,
    "Can I add Java and Kotlin then?"sv,
    "and they won't use it"sv,
    "wait, this is actually broken in a worse way than I initially read"sv,
    "`new int{1337};`"sv,
    "well, it's not the most people"sv,
    "No. You are most likely on an intel processor where `std::atomic` usually results in the exact same assembler instructions."sv
};
static_assert(std::popcount(messages.size()) == 1);

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
        ngrams(messages[rng() & (messages.size() - 1)], [&](const ngram_window& container) {
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
