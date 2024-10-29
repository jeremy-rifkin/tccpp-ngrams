#ifndef TOKENIZATION_HPP
#define TOKENIZATION_HPP

#include "ngram.hpp"

constexpr std::string_view before_gram_delimiters = " \t\n\r\v!\"#$%&()*,./:;<=>?@[\\]^`{|}~'-+";
constexpr std::string_view end_of_gram_delimiters = " \t\n\r\v!\"#$%&()*,./:;<=>?@[\\]^`{|}~";
constexpr std::string_view not_at_end = "'-";

constexpr std::size_t snowflake_max_length = 19;
constexpr std::size_t snowflake_min_length = 17;
inline bool looks_like_snowflake(std::string_view str) {
    if(str.size() >= snowflake_min_length && str.size() <= snowflake_max_length) {
        return std::ranges::all_of(str, [](char c){ return isdigit(c); });
    } else {
        return false;
    }
}

template<typename C>
void tokenize(std::string_view str, const C& callback) {
    std::size_t cursor = 0;
    ngram_window container;
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
        if(looks_like_snowflake(gram)) {
            container.clear();
            continue;
        }
        container.push(gram);
        callback(container);
    }
}

#endif
