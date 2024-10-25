#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <array>
#include <string_view>
#include <algorithm>

using namespace std::literals;

constexpr std::array bot_ids = {
    "597216680271282192"sv, // wheatley
    "155149108183695360"sv, // dyno
    "843841057833877588"sv, // dyno
    "1013956772245020774"sv, // dyno
    "1013960757127422113"sv // dyno
};

constexpr std::array blacklisted_channels = {
    "506274405500977153"sv // bot-spam
};

inline bool is_bot_id(std::string_view sv) {
    return std::ranges::contains(bot_ids, sv);
}

constexpr std::size_t minimum_occurrences = 40;

#endif
