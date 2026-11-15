/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/integrate/Render.h>
#include <moya/libmoya/output/Cryptomatte.h>
#include <moya/libmoya/output/Outputs.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>
#include <OpenEXR/ImfChannelList.h>
#include <OpenEXR/ImfHeader.h>
#include <OpenEXR/ImfInputFile.h>
#include <OpenEXR/ImfStringAttribute.h>

#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "Staged.h"
#include "Tracing.h"

namespace v3d::moya::tracing {
namespace {

namespace film = v3d::moya::film;
namespace integrate = v3d::moya::integrate;
namespace output = v3d::moya::output;
namespace scene = v3d::moya::scene;

constexpr std::uint32_t kSide = Staged::kSide;

/** A pixel inside the left square, the pixel the squares' shared edge halves, and a corner. **/
constexpr std::uint32_t kInside = 4;
constexpr std::uint32_t kEdge = 8;
constexpr std::uint32_t kRow = 8;

/** Where the squares meet: the middle of the pixel kEdge, on the screen from -1 to 1. **/
constexpr float kMeet = 0.0625F;

/** How far an edge pixel's share of each square may lie from a half. **/
constexpr float kShare = 0.03F;

/** The file the cases write. **/
constexpr const char * kFile = "cryptomatte.exr";

/**
 * Two squares side by side facing the camera, which meet at kMeet. The left one, the instance
 * left, is red. The right one, the instance right, is blue, and sits in the group rack that the
 * instance shelf places. A channel of each layer asked for collects its IDs.
 **/
struct Squares {
    Staged staged { 1, 256 };
    std::vector<scene::Handle> channels;

    Squares() {
        scene::Scene & made = staged.made;
        const scene::Handle red = staged.material("red", { { "base_color", scene::Value::colour(glm::vec3 { 0.8F, 0.1F, 0.1F }) } });
        const scene::Handle blue = staged.material("blue", { { "base_color", scene::Value::colour(glm::vec3 { 0.1F, 0.1F, 0.8F }) } });
        staged.quad("left", { { -1.0F, -0.5F, 0.0F }, { kMeet, -0.5F, 0.0F }, { kMeet, 0.5F, 0.0F }, { -1.0F, 0.5F, 0.0F } }, red);
        const scene::Handle mesh = made.create("mesh", "rightMesh");
        made.set(mesh, "P", scene::Value::points({ { kMeet, -0.5F, 0.0F }, { 1.0F, -0.5F, 0.0F }, { 1.0F, 0.5F, 0.0F }, { kMeet, 0.5F, 0.0F } }));
        made.set(mesh, "vertices", scene::Value::integers({ 3, 3 }));
        made.set(mesh, "indices", scene::Value::integers({ 0, 2, 1, 0, 3, 2 }));
        const scene::Handle rack = made.create("group", "rack");
        made.set(instanced(made, "right", mesh, rack, glm::mat4(1.0F)), "material", scene::Value::handle(blue));
        instanced(made, "shelf", rack, {}, glm::mat4(1.0F));
    }

    /** A layer of the source name given, of the ranks given. **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the layer's name, then its source
    void layer(const std::string & name, const std::string & source, std::int32_t ranks = 6) {
        const scene::Handle channel = staged.made.create("channel", name);
        staged.made.set(channel, "sourceType", scene::Value::string("raw"));
        staged.made.set(channel, "sourceName", scene::Value::string(source));
        staged.made.set(channel, "ranks", scene::Value::integer(ranks));
        channels.push_back(channel);
    }

    /** The picture of one output that holds every layer. **/
    film::Snapshot render() {
        const scene::Handle out = staged.made.create("output", "picture");
        staged.made.set(out, "file", scene::Value::string(kFile));
        staged.made.set(out, "channels", scene::Value::handles(channels));
        return staged.render(nullptr);
    }
};

/** The ID and coverage of a pixel's rank in a layer. **/
struct Rank {
    float id { 0.0F };
    float coverage { 0.0F };
};

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the pixel, then the rank
Rank rankOf(const film::Snapshot & picture, const std::string & layer, std::uint32_t x, std::uint32_t y, std::uint32_t rank) {
    char number[3] {};
    std::snprintf(number, sizeof(number), "%02u", rank / 2U);
    const std::uint32_t offset = picture.offset(layer + number) + (2U * (rank % 2U));
    return Rank { .id = picture.at(x, y, offset), .coverage = picture.at(x, y, offset + 1U) };
}

/** The float a name's ID is written as. **/
float idFloat(const std::string & name) {
    return output::floatOf(output::idOf(name));
}

/** A name's ID as the manifest writes it. **/
std::string hexOf(const std::string & name) {
    char text[9] {};
    std::snprintf(text, sizeof(text), "%08x", output::idOf(name));
    return text;
}

// MurmurHash3's 32-bit hash gives the values its reference gives
BOOST_AUTO_TEST_CASE(moya_cryptomatte_murmur3_matches_its_reference_test) {
    BOOST_TEST(output::murmur3("") == 0U);
    BOOST_TEST(output::murmur3("", 1U) == 0x514e28b7U);
    BOOST_TEST(output::murmur3("abc") == 0xb3dd93faU);
    BOOST_TEST(output::murmur3("hello") == 0x248bfa47U);
    BOOST_TEST(output::murmur3("The quick brown fox jumps over the lazy dog") == 0x2e4ff723U);
}

// an ID is a name's hash whose float is neither a NaN, an infinity nor a denormal. It differs
// from the hash only in the lowest bit of the exponent, and only where the exponent was all zeros
// or all ones. A layer's key is the first seven hex digits of its name's hash
BOOST_AUTO_TEST_CASE(moya_cryptomatte_an_id_is_a_float_of_every_kind_but_the_special_ones_test) {
    std::uint32_t flipped = 0;
    for (std::uint32_t i = 0; i < 20000U; ++i) {
        const std::string name = "object" + std::to_string(i);
        const std::uint32_t id = output::idOf(name);
        const std::uint32_t exponent = (id >> 23U) & 0xffU;
        BOOST_TEST_REQUIRE((exponent != 0U && exponent != 0xffU));
        BOOST_TEST_REQUIRE(std::isnormal(output::floatOf(id)));
        const std::uint32_t changed = id ^ output::murmur3(name);
        BOOST_TEST_REQUIRE((changed == 0U || changed == (1U << 23U)));
        flipped += changed != 0U ? 1U : 0U;
    }
    BOOST_TEST(flipped > 0U);
    char key[9] {};
    std::snprintf(key, sizeof(key), "%08x", output::murmur3("CryptoObject"));
    BOOST_TEST(output::keyOf("CryptoObject") == std::string(key, 7));
}

// a pixel inside one object holds that object's ID at a coverage of one, and nothing in its other
// ranks. A pixel that meets nothing holds nothing
BOOST_AUTO_TEST_CASE(moya_cryptomatte_a_pixel_inside_one_object_holds_it_whole_test) {
    Squares squares;
    squares.layer("CryptoObject", "crypto_object");
    const film::Snapshot picture = squares.render();
    BOOST_TEST(rankOf(picture, "CryptoObject", kInside, kRow, 0).id == idFloat("left"));
    BOOST_TEST(rankOf(picture, "CryptoObject", kInside, kRow, 0).coverage == 1.0F);
    for (std::uint32_t rank = 1; rank < output::kRanks; ++rank) {
        BOOST_TEST(rankOf(picture, "CryptoObject", kInside, kRow, rank).coverage == 0.0F);
    }
    BOOST_TEST(rankOf(picture, "CryptoObject", 0, 0, 0).coverage == 0.0F);
    BOOST_TEST(rankOf(picture, "CryptoObject", 0, 0, 0).id == 0.0F);
}

// the pixel the squares' shared edge halves holds both, each at about half, and the two shares
// together cover it whole. The layers of materials and of assets name their own IDs: a material's
// name, and the group an instance sits in
BOOST_AUTO_TEST_CASE(moya_cryptomatte_an_edge_pixel_holds_both_objects_by_their_shares_test) {
    Squares squares;
    squares.layer("CryptoObject", "crypto_object");
    squares.layer("CryptoMaterial", "crypto_material");
    squares.layer("CryptoAsset", "crypto_asset");
    const film::Snapshot picture = squares.render();
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the layer, then the two names
    const auto check = [&](const std::string & layer, const std::string & left, const std::string & right) {
        BOOST_TEST_CONTEXT(layer) {
            const Rank first = rankOf(picture, layer, kEdge, kRow, 0);
            const Rank second = rankOf(picture, layer, kEdge, kRow, 1);
            BOOST_TEST(first.coverage >= second.coverage);
            BOOST_TEST(std::fabs(first.coverage - 0.5F) < kShare);
            BOOST_TEST(std::fabs(second.coverage - 0.5F) < kShare);
            BOOST_TEST(first.coverage + second.coverage == 1.0F, boost::test_tools::tolerance(1.0e-6F));
            const bool named = (first.id == idFloat(left) && second.id == idFloat(right)) || (first.id == idFloat(right) && second.id == idFloat(left));
            BOOST_TEST(named);
            BOOST_TEST(rankOf(picture, layer, kEdge, kRow, 2).coverage == 0.0F);
        }
    };
    check("CryptoObject", "left", "right");
    check("CryptoMaterial", "red", "blue");
    check("CryptoAsset", "left", "rack");
}

// the file holds each layer's ranks as 32-bit floats beside a beauty of halves. Its header names
// each layer under its key, with the hash, the conversion and a manifest of every name in the
// picture and its ID. A layer of three ranks writes two channels
BOOST_AUTO_TEST_CASE(moya_cryptomatte_the_file_holds_the_ranks_and_the_manifest_test) {
    Squares squares;
    squares.layer("CryptoObject", "crypto_object");
    squares.layer("CryptoMaterial", "crypto_material", 3);
    const film::Snapshot picture = squares.render();
    BOOST_TEST(picture.manifests.at("CryptoObject") == "{\"left\":\"" + hexOf("left") + "\",\"right\":\"" + hexOf("right") + "\"}");
    output::write(output::outputsOf(squares.staged.made).front(), picture, colour::Config(), {});
    const Imf::InputFile file(kFile);
    const Imf::Header & header = file.header();
    BOOST_TEST(header.channels().findChannel("R")->type == Imf::HALF);
    for (const char * name : { "CryptoObject00.R", "CryptoObject00.A", "CryptoObject02.G", "CryptoMaterial01.A" }) {
        BOOST_TEST_CONTEXT(name) {
            BOOST_TEST_REQUIRE(header.channels().findChannel(name) != nullptr);
            BOOST_TEST(header.channels().findChannel(name)->type == Imf::FLOAT);
        }
    }
    BOOST_TEST(header.channels().findChannel("CryptoObject03.R") == nullptr);
    BOOST_TEST(header.channels().findChannel("CryptoMaterial02.R") == nullptr);
    const std::string prefix = "cryptomatte/" + output::keyOf("CryptoObject") + "/";
    const auto text = [&](const std::string & name) {
        const auto * attribute = header.findTypedAttribute<Imf::StringAttribute>(name);
        return attribute == nullptr ? std::string("missing") : attribute->value();
    };
    BOOST_TEST(text(prefix + "name") == "CryptoObject");
    BOOST_TEST(text(prefix + "hash") == "MurmurHash3_32");
    BOOST_TEST(text(prefix + "conversion") == "uint32_to_float32");
    BOOST_TEST(text(prefix + "manifest") == picture.manifests.at("CryptoObject"));
    BOOST_TEST(text("cryptomatte/" + output::keyOf("CryptoMaterial") + "/manifest") ==
        "{\"blue\":\"" + hexOf("blue") + "\",\"red\":\"" + hexOf("red") + "\"}");
    std::remove(kFile);
}

// a checkpoint holds each pixel's coverage exactly, so a render stopped after its first pass and
// resumed gives each layer's ranks and manifest bit for bit as a render that never stopped
BOOST_AUTO_TEST_CASE(moya_cryptomatte_a_resumed_render_keeps_its_coverage_test) {
    const auto rendered = [](const integrate::Checkpoints & checkpoints) {
        Squares squares;
        squares.layer("CryptoObject", "crypto_object");
        squares.layer("CryptoAsset", "crypto_asset");
        const scene::Handle out = squares.staged.made.create("output", "picture");
        squares.staged.made.set(out, "file", scene::Value::string(kFile));
        squares.staged.made.set(out, "channels", scene::Value::handles(squares.channels));
        squares.staged.made.commit();
        squares.staged.made.apply();
        device::CpuDevice device;
        return integrate::render(device, squares.staged.made, {}, colour::Config(), checkpoints);
    };
    const film::Snapshot whole = rendered({});
    const std::string checkpoint = "cryptomatte-checkpoint.exr";
    const film::Snapshot stopped = rendered(integrate::Checkpoints { .path = checkpoint, .stop = [](std::uint32_t passes) { return passes == 1U; } });
    BOOST_TEST(stopped.passes == 1U);
    const film::Snapshot resumed = rendered(integrate::Checkpoints { .path = checkpoint, .resume = true });
    std::remove(checkpoint.c_str());
    BOOST_TEST_REQUIRE(resumed.passes == whole.passes);
    BOOST_TEST(resumed.manifests == whole.manifests);
    bool same = true;
    for (const char * layer : { "CryptoObject0", "CryptoAsset0" }) {
        for (std::uint32_t channel = 0; channel < 3U; ++channel) {
            const std::uint32_t offset = whole.offset(layer + std::to_string(channel));
            for (std::uint32_t y = 0; y < kSide; ++y) {
                for (std::uint32_t x = 0; x < kSide; ++x) {
                    for (std::uint32_t i = 0; i < 4U; ++i) {
                        same = same && std::bit_cast<std::uint32_t>(resumed.at(x, y, offset + i)) == std::bit_cast<std::uint32_t>(whole.at(x, y, offset + i));
                    }
                }
            }
        }
    }
    BOOST_TEST(same);
    BOOST_TEST(rankOf(resumed, "CryptoObject", kEdge, kRow, 1).coverage > 0.0F);
}

};  // namespace
};  // namespace v3d::moya::tracing
