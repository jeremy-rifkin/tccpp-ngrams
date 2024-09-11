#ifndef UTILS_HPP
#define UTILS_HPP

#include <string_view>
#include <chrono>

#include <ankerl/unordered_dense.h>
#include <libassert/assert.hpp>
#include <bsoncxx/stdx/string_view.hpp>

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

constexpr std::string_view before_gram_delimiters = " \t\n\r\v!\"#$%&()*,./:;<=>?@[\\]^_`{|}~'-+";
constexpr std::string_view end_of_gram_delimiters = " \t\n\r\v!\"#$%&()*,./:;<=>?@[\\]^_`{|}~";
constexpr std::string_view not_at_end = "'-";

template<typename C>
void ngrams(std::string_view str, const C& callback) {
    std::size_t cursor = 0;
    while(cursor < str.size()) {
        auto start = str.find_first_not_of(before_gram_delimiters, cursor);
        auto end = std::min(str.find_first_of(end_of_gram_delimiters, start), str.size());
        if(start == std::string_view::npos) {
            break;
        }
        cursor = end;
        auto gram = str.substr(start, end - start);
        // trim ' and - from the end
        auto end_pos = gram.find_last_not_of(not_at_end);
        if(end_pos == std::string_view::npos) { // if the gram is only ' and -, then trimmed would be blank
            continue;
        }
        gram.remove_suffix(gram.size() - end_pos - 1);
        callback(gram);
    }
}

#endif
