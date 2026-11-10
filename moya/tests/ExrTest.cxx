/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/output/Exr.h>
#include <moya/libmoya/output/ExrPart.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Kinds.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>
#include <moya/libmoya/view/Hits.h>
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
#include <glm/vec3.hpp>

namespace {

namespace film = v3d::moya::film;
namespace output = v3d::moya::output;
namespace scene = v3d::moya::scene;
namespace view = v3d::moya::view;

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
        .values = {}, .errors = {} };
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
    BOOST_TEST(Imf::colorInteropID(file.header()) == "lin_ap1_scene");
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            BOOST_TEST(Imf::worldToCamera(file.header())[i][j] == camera[i][j]);
            BOOST_TEST(Imf::worldToNDC(file.header())[i][j] == screen[i][j]);
        }
    }
    BOOST_TEST(file.header().typedAttribute<Imf::FloatAttribute>("moya/renderTime").value() == 12.5F);
}

// a space with standard primaries writes them beside its ID, and a space with no ID writes neither
BOOST_AUTO_TEST_CASE(moya_exr_the_header_names_the_colour_space_it_is_given_test) {
    const Sandbox sandbox("space");
    const std::string rec709 = sandbox.file("rec709.exr");
    const std::string unnamed = sandbox.file("unnamed.exr");
    output::writeExr(rec709, snapshot(), { { .name = "", .outputs = { "beauty" } } }, { .colourSpace = "lin_rec709_scene" });
    output::writeExr(unnamed, snapshot(), { { .name = "", .outputs = { "beauty" } } }, { .colourSpace = "" });

    const Imf::InputFile named(rec709.c_str());
    BOOST_TEST(Imf::colorInteropID(named.header()) == "lin_rec709_scene");
    BOOST_TEST(Imf::chromaticities(named.header()).red.x == 0.64F);
    BOOST_TEST(Imf::chromaticities(named.header()).white.x == 0.3127F);
    const Imf::InputFile bare(unnamed.c_str());
    BOOST_TEST(!Imf::hasColorInteropID(bare.header()));
    BOOST_TEST(!Imf::hasChromaticities(bare.header()));
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

// a channel named R gives the file a second R beside the beauty's, and albedo.R a second albedo.R
// beside a three-float albedo's, and OpenEXR would keep only one of each
BOOST_AUTO_TEST_CASE(moya_exr_refuses_two_channels_of_one_name_in_a_part_test) {
    const Sandbox sandbox("clash");
    const std::string path = sandbox.file("clash.exr");
    film::Snapshot written { .resolution = { kWidth, kHeight }, .passes = 1, .stride = 8,
        .channels = { { .name = "R", .components = 1 }, { .name = "albedo.R", .components = 1 }, { .name = "albedo", .components = 3 } },
        .offsets = { 3, 4, 5 }, .values = {}, .errors = {} };
    written.values.assign(static_cast<std::size_t>(kWidth) * kHeight * 8, 0.5F);

    BOOST_CHECK_THROW(output::writeExr(path, written, { { .name = "", .outputs = { "beauty", "R" } } }, {}), std::invalid_argument);
    BOOST_CHECK_THROW(output::writeExr(path, written, { { .name = "", .outputs = { "albedo.R", "albedo" } } }, {}), std::invalid_argument);
    BOOST_TEST(!boost::filesystem::exists(path));

    const std::string apart = sandbox.file("apart.exr");
    output::writeExr(apart, written,
        { { .name = "rgba", .outputs = { "beauty" } }, { .name = "aovs", .outputs = { "R", "albedo" } } }, {});
    const Imf::MultiPartInputFile file(apart.c_str());
    BOOST_TEST(namesOf(file.header(1)) == std::vector<std::string>({ "R", "albedo.B", "albedo.G", "albedo.R" }), boost::test_tools::per_element());
}

/**
 * A hit view written to a file holds the beauty and each hit channel as floats. A pixel whose ray
 * met the triangle reads its depth and instance back, and one whose ray met nothing reads an
 * infinite depth and an instance of -1.
 *
 * Covers the base: e8e902d8 added view::writeHits(). Checked by writing the hit view at half
 * precision in writeHits(), and by leaving the instance channel out of its outputs. Each fails
 * this case.
 **/
BOOST_AUTO_TEST_CASE(moya_exr_a_hit_view_reads_back_from_its_file_test) {
    constexpr std::int32_t kSide = 8;
    scene::Scene scene(scene::kinds());
    const scene::Handle options = scene.create("options", "options");
    const scene::Handle camera = scene.create("camera", "camera");
    scene.set(options, "resolution", scene::Value::integers({ kSide, kSide }));
    scene.set(options, "camera", scene::Value::handle(camera));
    scene.set(camera, "projection", scene::Value::string("perspective"));
    scene.set(camera, "transform", scene::Value::matrix(glm::mat4(1.0F)));
    // the triangle covers the pixels below the screen's diagonal, at a depth of two
    const scene::Handle mesh = scene.create("mesh", "triangle");
    scene.set(mesh, "P", scene::Value::points({ { -2.0F, -2.0F, 2.0F }, { 2.0F, -2.0F, 2.0F }, { -2.0F, 2.0F, 2.0F } }));
    scene.set(mesh, "vertices", scene::Value::integers({ 3 }));
    scene.set(mesh, "indices", scene::Value::integers({ 0, 1, 2 }));
    const scene::Handle instance = scene.create("instance", "triangle");
    scene.set(instance, "prototype", scene::Value::handle(mesh));
    scene.set(instance, "transform", scene::Value::matrix(glm::mat4(1.0F)));
    scene.commit();
    scene.apply();

    const Sandbox sandbox("hits");
    const std::string path = sandbox.file("hits.exr");
    v3d::moya::device::CpuDevice device;
    view::writeHits(path, view::hits(device, scene, {}));

    Imf::InputFile file(path.c_str());
    const std::vector<std::string> names = namesOf(file.header());
    BOOST_TEST(names == std::vector<std::string>({ "B", "G", "R", "coverage", "depth", "face", "instance", "normal.B", "normal.G", "normal.R" }),
        boost::test_tools::per_element());
    for (const std::string & name : names) {
        BOOST_TEST_CONTEXT(name) {
            BOOST_TEST(file.header().channels().find(name).channel().type == Imf::FLOAT);
        }
    }
    const std::vector<float> depth = channelOf(file.header(), "depth", file);
    const std::vector<float> instances = channelOf(file.header(), "instance", file);
    constexpr std::size_t kMet = static_cast<std::size_t>(kSide - 1) * kSide;  // the first column of the last row
    constexpr std::size_t kMissed = kSide - 1;                                  // the last column of the first row
    BOOST_TEST(std::fabs(depth[kMet] - 2.0F) < 1.0e-5F);
    BOOST_TEST(instances[kMet] == static_cast<float>(instance.index));
    BOOST_TEST(std::isinf(depth[kMissed]));
    BOOST_TEST(instances[kMissed] == -1.0F);
}

};  // namespace
