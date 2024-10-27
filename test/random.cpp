#include <vector>

#include "utils.hpp"
#include "utils/sha.hpp"
#include "utils/random.hpp"

#include <libassert/assert-gtest.hpp>

TEST(Random, Basic) {
    auto hash = sha256(ngram<1>{"foo"}, "nonce");
    auto rng = make_xoroshiro128plus(hash);
    EXPECT(rng() == 0xd13e4f7fa1e569bb);
    EXPECT(rng() == 0x7779092e6bf0f200);
    EXPECT(rng() == 0x355ab02f4826dd1e);
    EXPECT(rng() == 0x81efb91f6f639c04);
    EXPECT(rng() == 0x1855ef386c55db8e);
    EXPECT(rng() == 0x650ffb4f031183fd);
    EXPECT(rng() == 0x268b652fd3c0346a);
    EXPECT(rng() == 0x77d45e31be33eaac);
    EXPECT(rng() == 0x57c2c07dd874f965);
    EXPECT(rng() == 0x57ee5878475ccba1);
    EXPECT(rng() == 0xafde09c1a36b2afe);
    EXPECT(rng() == 0xc02356ac898dcb72);
    EXPECT(rng() == 0x1df2f9a3b0b29002);
    EXPECT(rng() == 0xe832efdb48de9500);
    EXPECT(rng() == 0x725d0b67c7695e92);
    EXPECT(rng() == 0x1ee0459502989719);
    EXPECT(rng() == 0xbd1afea637842b0c);
}

TEST(Random, Float) {
    auto hash = sha256(ngram<1>{"foo"}, "nonce");
    auto rng = make_xoroshiro128plus(hash);
    EXPECT(random_double(rng()) ==  0.634714066811095320);
    EXPECT(random_double(rng()) == -0.066618778546505997);
    EXPECT(random_double(rng()) == -0.583169915116465320);
    EXPECT(random_double(rng()) ==  0.015128269528557281);
    EXPECT(random_double(rng()) == -0.809877488565079460);
    EXPECT(random_double(rng()) == -0.210449778003624740);
    EXPECT(random_double(rng()) == -0.698870994231127530);
    EXPECT(random_double(rng()) == -0.063831544600902346);
    EXPECT(random_double(rng()) == -0.314369143058762510);
    EXPECT(random_double(rng()) == -0.313038769979607560);
    EXPECT(random_double(rng()) ==  0.373963565397241070);
    EXPECT(random_double(rng()) ==  0.501078447577226750);
    EXPECT(random_double(rng()) == -0.766022486756451130);
    EXPECT(random_double(rng()) ==  0.814054472038823200);
    EXPECT(random_double(rng()) == -0.106535505606349950);
    EXPECT(random_double(rng()) == -0.758780767670767190);
    EXPECT(random_double(rng()) ==  0.477386313591553040);
}
