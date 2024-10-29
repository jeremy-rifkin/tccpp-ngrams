#include <vector>

#include "ngram.hpp"
#include "tokenization.hpp"

#include <libassert/assert-gtest.hpp> // has to come last, not ideal

TEST(Ngrams, Basic) {
    std::string_view input = "foo bar. is c++ isn't [foo]";
    std::vector<std::optional<ngram<1>>> expected{{{"foo"}}, {{"bar"}}, {{"is"}}, {{"c++"}}, {{"isn't"}}, {{"foo"}}};
    std::vector<std::optional<ngram<1>>> output;
    tokenize(input, [&](const ngram_window& gram) {
        output.push_back(gram.subview<1>().transform([&](auto value) { return ngram<1>(value); }));
    });
    ASSERT(output == expected);
}

TEST(Ngrams, TwoGrams) {
    std::string_view input = "foo bar. is c++ isn't [foo]";
    std::vector<std::optional<ngram<2>>> expected{
        std::nullopt,
        {{"foo", "bar"}},
        {{"bar", "is"}},
        {{"is", "c++"}},
        {{"c++", "isn't"}},
        {{"isn't", "foo"}}
    };
    std::vector<std::optional<ngram<2>>> output;
    tokenize(input, [&](const ngram_window& gram) {
        output.push_back(gram.subview<2>().transform([&](auto value) { return ngram<2>(value); }));
    });
    ASSERT(output == expected);
}

TEST(Ngrams, ThreeGrams) {
    std::string_view input = "foo bar. is c++ isn't [foo]";
    std::vector<std::optional<ngram<3>>> expected{
        std::nullopt,
        std::nullopt,
        {{"foo", "bar", "is"}},
        {{"bar", "is", "c++"}},
        {{"is", "c++", "isn't"}},
        {{"c++", "isn't", "foo"}}
    };
    std::vector<std::optional<ngram<3>>> output;
    tokenize(input, [&](const ngram_window& gram) {
        output.push_back(gram.subview<3>().transform([&](auto value) { return ngram<3>(value); }));
    });
    ASSERT(output == expected);
}

TEST(Ngrams, FourGrams) {
    std::string_view input = "foo bar. is c++ isn't [foo]";
    std::vector<std::optional<ngram<4>>> expected{
        std::nullopt,
        std::nullopt,
        std::nullopt,
        {{"foo", "bar", "is", "c++"}},
        {{"bar", "is", "c++", "isn't"}},
        {{"is", "c++", "isn't", "foo"}}
    };
    std::vector<std::optional<ngram<4>>> output;
    tokenize(input, [&](const ngram_window& gram) {
        output.push_back(gram.subview<4>().transform([&](auto value) { return ngram<4>(value); }));
    });
    ASSERT(output == expected);
}

TEST(Ngrams, FiveGrams) {
    std::string_view input = "foo bar. is c++ isn't [foo]";
    std::vector<std::optional<ngram<5>>> expected{
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        {{"foo", "bar", "is", "c++", "isn't"}},
        {{"bar", "is", "c++", "isn't", "foo"}}
    };
    std::vector<std::optional<ngram<5>>> output;
    tokenize(input, [&](const ngram_window& gram) {
        output.push_back(gram.subview<5>().transform([&](auto value) { return ngram<5>(value); }));
    });
    ASSERT(output == expected);
}
