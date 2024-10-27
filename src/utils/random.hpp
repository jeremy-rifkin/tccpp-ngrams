#ifndef RANDOM_HPP
#define RANDOM_HPP

#include <cstdint>
#include <cmath>

#include "utils/sha.hpp"

#include <xoshiro-cpp/XoshiroCpp.hpp>

inline XoshiroCpp::Xoroshiro128Plus make_xoroshiro128plus(sha256_digest hash) {
    auto seed = 0
            | uint64_t(hash[7]) << (7 * 8)
            | uint64_t(hash[6]) << (6 * 8)
            | uint64_t(hash[5]) << (5 * 8)
            | uint64_t(hash[4]) << (4 * 8)
            | uint64_t(hash[3]) << (3 * 8)
            | uint64_t(hash[2]) << (2 * 8)
            | uint64_t(hash[1]) << (1 * 8)
            | uint64_t(hash[0]) << (0 * 8);
    return XoshiroCpp::Xoroshiro128Plus{seed};
}

// random double [-1, 1)
inline double random_double(uint64_t x) {
    return std::fma(double(x >> 11), 0x2.0p-53, -1.0);
}

#endif
