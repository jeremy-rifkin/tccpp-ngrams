#ifndef NGRAM_HPP
#define NGRAM_HPP

#include <cstddef>
#include <ranges>
#include <span>
#include <utility>

#include <ankerl/unordered_dense.h>
#include <libassert/assert.hpp>
#include <fmt/format.h>

#include "constants.hpp"
#include "utils.hpp"

template<typename T, std::size_t N> requires(N >= 1)
class ngram_tmpl {
    std::array<T, N> grams;
public:
    ngram_tmpl() = default;

    template<typename U>
    ngram_tmpl(std::span<U> data) {
        ASSERT(data.size() <= N);
        for(std::size_t i = 0; auto& item : data) {
            grams[i++] = T(std::move(item));
        }
    }

    template<typename U>
    ngram_tmpl(std::initializer_list<U> data) {
        ASSERT(data.size() <= N);
        for(std::size_t i = 0; auto& item : data) {
            grams[i++] = T(item);
        }
    }

    template<typename U>
    ngram_tmpl(ngram_tmpl<U, N>&& data) {
        for(std::size_t i = 0; auto& item : data) {
            grams[i++] = T(std::move(item));
        }
    }

    template<typename U>
    ngram_tmpl(const ngram_tmpl<U, N>& data) {
        for(std::size_t i = 0; auto& item : data) {
            grams[i++] = T(item);
        }
    }

    auto size() const {
        return N;
    }

    const T& operator[](std::size_t i) const {
        return grams[i];
    }

    auto begin() const {
        return grams.begin();
    }

    auto end() const {
        return grams.end();
    }

    bool operator==(const ngram_tmpl& other) const {
        return grams == other.grams;
    }

    template<typename TT, std::size_t NN> requires(NN >= 1) friend class ngram_tmpl;

    template<typename O>
    bool operator==(const ngram_tmpl<O, N>& other) const {
        return std::ranges::equal(grams, other.grams);
    }
};

template<std::size_t N>
using ngram = ngram_tmpl<std::string, N>;

template<std::size_t N>
using ngram_view = ngram_tmpl<std::string_view, N>;

struct ngram_hash {
    using is_transparent = void; // enable heterogeneous overloads
    using is_avalanching = void; // mark class as high quality avalanching hash

    template<typename T, std::size_t N>
    [[nodiscard]] auto operator()(const ngram_tmpl<T, N>& ngram) const noexcept -> uint64_t {
        std::size_t hash = 0;
        for(const auto& part : ngram) {
            hash_combine(hash, part);
        }
        return hash;
    }
};

template<std::size_t N, typename T> using ngram_map = ankerl::unordered_dense::map<ngram<N>, T, ngram_hash, std::equal_to<>>;

template<typename T, std::size_t N>
struct fmt::formatter<ngram_tmpl<T, N>> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const ngram_tmpl<T, N>& ngram, format_context& ctx) const {
        return fmt::format_to(ctx.out(), "{}", fmt::join(ngram, ", "));
    }
};

template<typename T, std::size_t N> requires(N >= 1)
class ngram_window_tmpl {
    std::array<T, N> grams;
    std::size_t cursor = 0;
public:
    ngram_window_tmpl() = default;

    template<typename U>
    ngram_window_tmpl(std::span<U> data) {
        ASSERT(data.size() <= N);
        for(std::size_t i = 0; auto& item : data) {
            grams[i++] = T(std::move(item));
        }
        cursor = data.size();
    }

    template<typename U>
    ngram_window_tmpl(std::initializer_list<U> data) {
        ASSERT(data.size() <= N);
        for(std::size_t i = 0; auto& item : data) {
            grams[i++] = T(item);
        }
        cursor = data.size();
    }

    template<typename U>
    ngram_window_tmpl(U&& item) {
        grams[0] = T(std::forward<U>(item));
        cursor = 1;
    }

    template<typename U>
    void push(U&& item) {
        if(cursor == N) {
            // TODO: This shifting is temporary, may reconsider later.
            for(std::size_t i = 0; i < N - 1; i++) {
                grams[i] = std::move(grams[i + 1]);
            }
            grams[N - 1] = std::forward<U>(item);
        } else {
            grams[cursor] = std::forward<U>(item);
            cursor++;
        }
    }

    auto size() const {
        return cursor;
    }

    void clear() {
        cursor = 0;
    }

    const T& operator[](std::size_t i) const {
        return grams[i];
    }

    template<std::size_t W>
    std::optional<ngram_tmpl<T, W>> subview() const {
        DEBUG_ASSERT(W != 0);
        if(W > cursor) {
            return std::nullopt;
        } else {
            return std::span(end() - W, end());
        }
    }

    auto begin() const {
        return grams.begin();
    }

    auto end() const {
        return grams.begin() + cursor;
    }

    bool operator==(const ngram_window_tmpl& other) const {
        return grams == other.grams;
    }
};

template<typename T, std::size_t N>
struct std::hash<ngram_window_tmpl<T, N>> {
    std::size_t operator()(const ngram_window_tmpl<T, N>& ngram) const noexcept {
        std::size_t h = 0;
        for(const auto& gram : ngram) {
            hash_combine(h, gram);
        }
        return h;
    }
};

using ngram_window = ngram_window_tmpl<std::string_view, ngram_max_width>;

#endif
