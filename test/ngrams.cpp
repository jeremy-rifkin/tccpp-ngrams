#include <vector>

#include <libassert/assert-gtest.hpp>

#include "utils.hpp"

TEST(Ngrams, Basic) {
    std::string_view input = "foo bar. is c++ isn't [foo]";
    std::vector<std::string_view> expected{"foo", "bar", "is", "c++", "isn't", "foo"};
    std::vector<std::string_view> output;
    ngrams(input, [&](std::string_view gram) {
        output.push_back(gram);
    });
    ASSERT(output == expected);
}
