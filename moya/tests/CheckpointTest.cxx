/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/film/Channel.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/film/State.h>
#include <moya/libmoya/kernel/film/Pixels.h>
#include <moya/libmoya/output/Checkpoint.h>
#include <moya/libmoya/output/Exr.h>
#include <moya/libmoya/output/ExrImage.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/test/unit_test.hpp>

namespace {

namespace film = v3d::moya::film;
namespace kernel = v3d::moya::kernel;
namespace output = v3d::moya::output;

/** The fingerprint the cases write, and another. **/
constexpr std::uint64_t kFingerprint = 0x0123456789ABCDEFULL;
constexpr std::uint64_t kOther = 0x0123456789ABCDEEULL;

/** A directory of its own per case, removed with everything under it. **/
class Sandbox {
 public:
    explicit Sandbox(const std::string & name) : path_(boost::filesystem::temp_directory_path() / ("v3d_checkpoint_" + name)) {
        boost::system::error_code ignored;
        boost::filesystem::remove_all(path_, ignored);
        boost::filesystem::create_directories(path_);
    }

    ~Sandbox() {
        boost::system::error_code ignored;
        boost::filesystem::remove_all(path_, ignored);
    }

    Sandbox(const Sandbox &) = delete;
    Sandbox & operator=(const Sandbox &) = delete;

    std::string file(const std::string & name) const {
        return (path_ / name).string();
    }

 private:
    boost::filesystem::path path_;
};

/** The channels of the cases' films: a depth and an albedo. **/
std::vector<film::Channel> channels() {
    return { { .name = "depth", .components = 1 }, { .name = "albedo", .components = 3 } };
}

/**
 * A state of three by two pixels of channels(), after five passes, whose floats differ by pixel
 * and by float and include a negative zero and a denormal, and whose middle column has stopped.
 **/
film::State stateOf() {
    film::State out { .resolution = { 3, 2 }, .passes = 5, .channels = channels(), .stored = 3U + 4U + kernel::film::kTailFloats };
    for (std::uint32_t pixel = 0; pixel < 6; ++pixel) {
        for (std::uint32_t i = 0; i < out.stored; ++i) {
            out.sums.push_back((static_cast<float>(pixel) / 3.0F) + (static_cast<float>(i) * 0.1F));
        }
        out.stopped.push_back(pixel % 3U == 1U ? 1U : 0U);
    }
    out.sums[1] = -0.0F;
    out.sums[2] = std::numeric_limits<float>::denorm_min();
    return out;
}

// a checkpoint holds every float as it was written, bit for bit, with the stopped pixels and the
// passes, and the partial file it was written through is gone
BOOST_AUTO_TEST_CASE(moya_checkpoint_holds_the_films_state_exactly_test) {
    const Sandbox sandbox("exact");
    const std::string path = sandbox.file("render.checkpoint.exr");
    const film::State written = stateOf();
    output::writeCheckpoint(path, written, kFingerprint);
    BOOST_TEST(!boost::filesystem::exists(path + ".partial"));
    const film::State read = output::readCheckpoint(path, channels(), kFingerprint);
    BOOST_TEST(read.resolution.x == 3U);
    BOOST_TEST(read.resolution.y == 2U);
    BOOST_TEST(read.passes == 5U);
    BOOST_TEST(read.stored == written.stored);
    BOOST_TEST(read.channels.size() == 2U);
    BOOST_TEST(read.sums.size() == written.sums.size());
    BOOST_TEST(std::memcmp(read.sums.data(), written.sums.data(), written.sums.size() * sizeof(float)) == 0);
    BOOST_TEST(read.stopped == written.stopped, boost::test_tools::per_element());
}

// any EXR reader opens a checkpoint, and sees the beauty's weighted sums as R, G and B
BOOST_AUTO_TEST_CASE(moya_checkpoint_opens_as_a_picture_test) {
    const Sandbox sandbox("picture");
    const std::string path = sandbox.file("render.checkpoint.exr");
    const film::State written = stateOf();
    output::writeCheckpoint(path, written, kFingerprint);
    const output::ExrImage image = output::readExr(path);
    BOOST_TEST(image.width == 3U);
    BOOST_TEST(image.height == 2U);
    BOOST_TEST(image.texels[4].x == written.sums[std::size_t { 4 } * written.stored]);
    BOOST_TEST(image.texels[4].z == written.sums[(std::size_t { 4 } * written.stored) + 2U]);
}

// a second checkpoint replaces the first
BOOST_AUTO_TEST_CASE(moya_checkpoint_replaces_the_one_before_test) {
    const Sandbox sandbox("replaces");
    const std::string path = sandbox.file("render.checkpoint.exr");
    output::writeCheckpoint(path, stateOf(), kFingerprint);
    film::State later = stateOf();
    later.passes = 6;
    output::writeCheckpoint(path, later, kOther);
    BOOST_TEST(output::readCheckpoint(path, channels(), kOther).passes == 6U);
}

// a checkpoint of another fingerprint, a picture that is not a checkpoint, and a checkpoint that
// lacks a channel the film holds are each refused
BOOST_AUTO_TEST_CASE(moya_checkpoint_refuses_what_it_cannot_resume_test) {
    const Sandbox sandbox("refuses");
    const std::string path = sandbox.file("render.checkpoint.exr");
    output::writeCheckpoint(path, stateOf(), kFingerprint);
    BOOST_CHECK_THROW(output::readCheckpoint(path, channels(), kOther), output::Refused);
    std::vector<film::Channel> more = channels();
    more.push_back({ .name = "normal", .components = 3 });
    BOOST_CHECK_THROW(output::readCheckpoint(path, more, kFingerprint), output::Refused);

    const std::string picture = sandbox.file("render.exr");
    const film::Snapshot snapshot { .resolution = { 1, 1 }, .passes = 1, .stride = 3, .channels = {}, .offsets = {},
        .values = { 1.0F, 1.0F, 1.0F }, .errors = { 0.0F }, .samples = { 1U } };
    output::writeExr(picture, snapshot, { { .name = "", .outputs = { std::string(output::kBeauty) } } }, {});
    BOOST_CHECK_THROW(output::readCheckpoint(picture, {}, kFingerprint), output::Refused);
}

// a state that does not fill its resolution, or whose channel takes a name the film's own floats
// take in the file, is refused before anything is written
BOOST_AUTO_TEST_CASE(moya_checkpoint_refuses_a_state_it_cannot_write_test) {
    const Sandbox sandbox("unwritable");
    const std::string path = sandbox.file("render.checkpoint.exr");
    film::State shorter = stateOf();
    shorter.sums.pop_back();
    BOOST_CHECK_THROW(output::writeCheckpoint(path, shorter, kFingerprint), std::invalid_argument);
    film::State clashing = stateOf();
    clashing.channels[0].name = "moya.weight";
    BOOST_CHECK_THROW(output::writeCheckpoint(path, clashing, kFingerprint), std::invalid_argument);
    BOOST_TEST(!boost::filesystem::exists(path));
}

};  // namespace
