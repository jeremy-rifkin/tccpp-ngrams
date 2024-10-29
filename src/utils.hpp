#ifndef UTILS_HPP
#define UTILS_HPP

#include <string_view>
#include <chrono>

#include <ankerl/unordered_dense.h>
#include <libassert/assert.hpp>

using sys_ms = std::chrono::sys_time<std::chrono::milliseconds>;

// Find the last item before the end iterator, given a forward or input iterator
template<typename It>
It last(It begin, It end) {
    ASSERT(begin != end);
    auto last = begin;
    auto cursor = begin;
    while(true) {
        ++cursor;
        if(cursor == end) {
            return last;
        } else {
            last = cursor;
        }
    }
}

struct string_hash {
    using is_transparent = void; // enable heterogeneous overloads
    using is_avalanching = void; // mark class as high quality avalanching hash

    [[nodiscard]] auto operator()(std::string_view str) const noexcept -> uint64_t {
        return ankerl::unordered_dense::hash<std::string_view>{}(str);
    }
};

using string_set = ankerl::unordered_dense::set<std::string, string_hash, std::equal_to<>>;
template<typename T> using string_map = ankerl::unordered_dense::map<std::string, T, string_hash, std::equal_to<>>;

extern "C" [[gnu::noinline]] inline void trace_trigger() {
    asm volatile("");
}

template<typename T>
inline void hash_combine(std::size_t& seed, const T& v) {
    ankerl::unordered_dense::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

template<std::size_t N>
constexpr void indexinator(auto&& f) {
    [&]<std::size_t... I>(std::index_sequence<I...>) {
        (f.template operator()<I>(), ...);
    }(std::make_index_sequence<N>{});
}

#endif
