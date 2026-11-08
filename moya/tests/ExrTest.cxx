/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/film/Film.h>
#include <moya/libmoya/output/Exr.h>
#include <Imath/ImathBox.h>
#include <Imath/half.h>
#include <OpenEXR/ImfChannelList.h>
#include <OpenEXR/ImfChromaticities.h>
#include <OpenEXR/ImfCompression.h>
#include <OpenEXR/ImfFloatAttribute.h>
#include <OpenEXR/ImfFrameBuffer.h>
#include <OpenEXR/ImfHeader.h>
#include <OpenEXR/ImfInputFile.h>
#include <OpenEXR/ImfInputPart.h>
#include <OpenEXR/ImfMultiPartInputFile.h>
#include <OpenEXR/ImfPixelType.h>
#include <OpenEXR/ImfStandardAttributes.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/test/unit_test.hpp>

#include <glm/mat4x4.hpp>

namespace {

namespace film = v3d::moya::film;
namespace output = v3d::moya::output;

constexpr std::uint32_t kWidth = 5;
constexpr std::uint32_t kHeight = 4;

/**
 * A directory of its own per case, removed with everything under it, so a case that leaves a file
 * behind fails the case that looks for it rather than the next run.
 **/
class Sandbox {
 public:
    explicit Sandbox(const std::string & name) : path_(boost::filesystem::temp_directory_path() / ("v3d_exr_" + name)) {
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

/** A snapshot with a depth and an albedo channel, every float a different one with a fraction. **/
film::Snapshot snapshot() {
    film::Snapshot out { .resolution = { kWidth, kHeight }, .passes = 1, .stride = 7,
        .channels = { { .name = "depth", .components = 1 }, { .name = "albedo", .components = 3 } }, .offsets = { 3, 4 },
        .values = {} };
    for (std::size_t i = 0; i < static_cast<std::size_t>(kWidth) * kHeight * 7; ++i) {
        out.values.push_back((std::sin(static_cast<float>(i) * 0.37F) * 40.0F) + 0.123456F);
    }
    return out;
}

/** One channel of a file part, read back as floats, row by row. **/
std::vector<float> channelOf(const Imf::Header & header, const std::string & name, auto & reader) {
    const Imath::Box2i & window = header.dataWindow();
    const auto width = static_cast<std::size_t>(window.max.x - window.min.x) + 1U;
    const auto height = static_cast<std::size_t>(window.max.y - window.min.y) + 1U;
    std::vector<float> out(width * height);
    Imf::FrameBuffer buffer;
    buffer.insert(name, Imf::Slice(Imf::FLOAT, reinterpret_cast<char *>(out.data()), sizeof(float), sizeof(float) * width));
    reader.setFrameBuffer(buffer);
    reader.readPixels(window.min.y, window.max.y);
    return out;
}

/** A snapshot's float at a pixel's offset, for each pixel, row by row. **/
std::vector<float> expected(const film::Snapshot & from, std::uint32_t offset) {
    std::vector<float> out;
    for (std::uint32_t y = 0; y < kHeight; ++y) {
        for (std::uint32_t x = 0; x < kWidth; ++x) {
            out.push_back(from.at(x, y, offset));
        }
    }
    return out;
}

/** The names of a header's channels, in the order the file lists them. **/
std::vector<std::string> namesOf(const Imf::Header & header) {
    std::vector<std::string> out;
    for (auto channel = header.channels().begin(); channel != header.channels().end(); ++channel) {
        out.emplace_back(channel.name());
    }
    return out;
}

BOOST_AUTO_TEST_CASE(moya_exr_a_float_part_holds_the_snapshots_values_exactly_test) {
    const Sandbox sandbox("float");
    const film::Snapshot written = snapshot();
    const std::string path = sandbox.file("float.exr");
    output::writeExr(path, written, { { .name = "", .outputs = { "beauty", "depth", "albedo" }, .precision = output::Precision::Float,
        .compression = output::Compression::Zip } }, {});

    Imf::InputFile file(path.c_str());
    BOOST_TEST(!file.header().hasName());
    const std::vector<std::string> names = namesOf(file.header());
    BOOST_TEST(names == std::vector<std::string>({ "B", "G", "R", "albedo.B", "albedo.G", "albedo.R", "depth" }),
        boost::test_tools::per_element());
    const std::vector<std::pair<std::string, std::uint32_t>> channels = { { "R", 0 }, { "G", 1 }, { "B", 2 }, { "depth", 3 },
        { "albedo.R", 4 }, { "albedo.G", 5 }, { "albedo.B", 6 } };
    for (const auto & [name, offset] : channels) {
        BOOST_TEST_CONTEXT(name) {
            BOOST_TEST(file.header().channels().find(name).channel().type == Imf::FLOAT);
            BOOST_TEST(channelOf(file.header(), name, file) == expected(written, offset), boost::test_tools::per_element());
        }
    }
}

// a half rounds each value to its 11 significant bits, and reads back as that rounded value
BOOST_AUTO_TEST_CASE(moya_exr_a_half_part_holds_the_values_within_halfs_precision_test) {
    const Sandbox sandbox("half");
    const film::Snapshot written = snapshot();
    const std::string path = sandbox.file("half.exr");
    output::writeExr(path, written, { { .name = "", .outputs = { "beauty" }, .precision = output::Precision::Half,
        .compression = output::Compression::Piz } }, {});

    Imf::InputFile file(path.c_str());
    BOOST_TEST(file.header().compression() == Imf::PIZ_COMPRESSION);
    BOOST_TEST(file.header().channels().find("G").channel().type == Imf::HALF);
    const std::vector<float> read = channelOf(file.header(), "G", file);
    const std::vector<float> want = expected(written, 1);
    for (std::size_t i = 0; i < want.size(); ++i) {
        BOOST_TEST(read[i] == static_cast<float>(half(want[i])));
        BOOST_TEST(std::fabs(read[i] - want[i]) <= std::fabs(want[i]) / 1024.0F);
    }
}

BOOST_AUTO_TEST_CASE(moya_exr_each_part_holds_its_own_channels_and_no_others_test) {
    const Sandbox sandbox("parts");
    const film::Snapshot written = snapshot();
    const std::string path = sandbox.file("parts.exr");
    output::writeExr(path, written,
        { { .name = "rgba", .outputs = { "beauty" }, .precision = output::Precision::Half, .compression = output::Compression::Zip },
            { .name = "aovs", .outputs = { "depth", "albedo" }, .precision = output::Precision::Float,
                .compression = output::Compression::None } },
        {});

    Imf::MultiPartInputFile file(path.c_str());
    BOOST_TEST(file.parts() == 2);
    BOOST_TEST(file.header(0).name() == "rgba");
    BOOST_TEST(namesOf(file.header(0)) == std::vector<std::string>({ "B", "G", "R" }), boost::test_tools::per_element());
    BOOST_TEST(file.header(1).name() == "aovs");
    BOOST_TEST(namesOf(file.header(1)) == std::vector<std::string>({ "albedo.B", "albedo.G", "albedo.R", "depth" }),
        boost::test_tools::per_element());
    BOOST_TEST(file.header(1).compression() == Imf::NO_COMPRESSION);

    Imf::InputPart aovs(file, 1);
    BOOST_TEST(channelOf(file.header(1), "depth", aovs) == expected(written, 3), boost::test_tools::per_element());
}

// the matrices are written as glm holds them, so each float reads back at the same indices
BOOST_AUTO_TEST_CASE(moya_exr_the_header_holds_the_working_space_and_the_metadata_test) {
    const Sandbox sandbox("header");
    glm::mat4 camera(1.0F);
    camera[3][0] = 2.0F;
    camera[1][2] = -0.5F;
    const glm::mat4 screen(3.0F);
    const std::string path = sandbox.file("header.exr");
    output::writeExr(path, snapshot(), { { .name = "beauty", .outputs = { "beauty" } } },
        { .worldToCamera = camera, .worldToScreen = screen, .renderSeconds = 12.5F });

    const Imf::InputFile file(path.c_str());
    BOOST_TEST(Imf::hasChromaticities(file.header()));
    const Imf::Chromaticities chromaticities = Imf::chromaticities(file.header());
    BOOST_TEST(chromaticities.red.x == 0.713F);
    BOOST_TEST(chromaticities.white.y == 0.33767F);
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            BOOST_TEST(Imf::worldToCamera(file.header())[i][j] == camera[i][j]);
            BOOST_TEST(Imf::worldToNDC(file.header())[i][j] == screen[i][j]);
        }
    }
    BOOST_TEST(file.header().typedAttribute<Imf::FloatAttribute>("moya/renderTime").value() == 12.5F);
}

BOOST_AUTO_TEST_CASE(moya_exr_refuses_parts_it_cannot_write_test) {
    const Sandbox sandbox("refused");
    const std::string path = sandbox.file("refused.exr");
    const film::Snapshot written = snapshot();
    BOOST_CHECK_THROW(output::writeExr(path, written, {}, {}), std::invalid_argument);
    BOOST_CHECK_THROW(output::writeExr(path, written, { { .name = "", .outputs = {} } }, {}), std::invalid_argument);
    BOOST_CHECK_THROW(output::writeExr(path, written, { { .name = "", .outputs = { "normal" } } }, {}), std::invalid_argument);
    BOOST_CHECK_THROW(output::writeExr(path, written, { { .name = "", .outputs = { "depth", "depth" } } }, {}), std::invalid_argument);
    BOOST_CHECK_THROW(output::writeExr(path, written, { { .name = "a", .outputs = { "beauty" } }, { .name = "a", .outputs = { "depth" } } }, {}),
        std::invalid_argument);
    BOOST_CHECK_THROW(output::writeExr(path, written, { { .name = "", .outputs = { "beauty" } }, { .name = "b", .outputs = { "depth" } } }, {}),
        std::invalid_argument);
    BOOST_TEST(!boost::filesystem::exists(path));
}

};  // namespace
