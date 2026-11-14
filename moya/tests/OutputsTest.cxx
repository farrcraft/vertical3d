/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/integrate/Render.h>
#include <moya/libmoya/output/Channel.h>
#include <moya/libmoya/output/ExrPart.h>
#include <moya/libmoya/output/Output.h>
#include <moya/libmoya/output/Outputs.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Kinds.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>
#include <OpenEXR/ImfChannelList.h>
#include <OpenEXR/ImfHeader.h>
#include <OpenEXR/ImfInputFile.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/test/unit_test.hpp>

namespace {

namespace colour = v3d::moya::colour;
namespace film = v3d::moya::film;
namespace integrate = v3d::moya::integrate;
namespace output = v3d::moya::output;
namespace scene = v3d::moya::scene;

/** The side of the rendered picture, in pixels. **/
constexpr std::int32_t kSide = 8;

/** A pixel at the picture's centre, which sees the dome. **/
constexpr std::uint32_t kCentre = 4;

/**
 * A scene of a dome light, seen at a few samples, with two outputs. The first holds albedo and
 * depth, and the second, in floats, depth and an expression's light. Depth is named by both.
 **/
struct Outputs {
    scene::Scene made { scene::kinds() };

    explicit Outputs(std::int32_t samples = 4) {
        const scene::Handle options = made.create("options", "options");
        const scene::Handle camera = made.create("camera", "camera");
        made.set(options, "camera", scene::Value::handle(camera));
        made.set(options, "resolution", scene::Value::integers({ kSide, kSide }));
        made.set(options, "samples", scene::Value::integer(samples));
        made.create("domeLight", "sky");

        const scene::Handle albedo = channel("albedo", "raw", "albedo", 3);
        const scene::Handle depth = channel("depth", "raw", "z", 1);
        const scene::Handle diffuse = channel("diffuse", "lpe", "C<RD>L", 3);
        const scene::Handle first = made.create("output", "first");
        made.set(first, "file", scene::Value::string("first.exr"));
        made.set(first, "channels", scene::Value::handles({ albedo, depth }));
        const scene::Handle second = made.create("output", "second");
        made.set(second, "file", scene::Value::string("second.exr"));
        made.set(second, "channels", scene::Value::handles({ depth, diffuse }));
        made.set(second, "precision", scene::Value::string("float"));
        made.set(second, "compression", scene::Value::string("piz"));
        made.commit();
        made.apply();
    }

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the name, then what it collects
    scene::Handle channel(const std::string & name, const std::string & type, const std::string & source, std::int32_t components) {
        const scene::Handle object = made.create("channel", name);
        made.set(object, "sourceType", scene::Value::string(type));
        made.set(object, "sourceName", scene::Value::string(source));
        made.set(object, "components", scene::Value::integer(components));
        return object;
    }
};

// a channel two outputs name is one film channel, and channels follow the order the outputs name
// them in
BOOST_AUTO_TEST_CASE(moya_outputs_each_channel_an_output_names_is_rendered_once_test) {
    const Outputs outputs;
    const std::vector<output::Channel> channels = output::channelsOf(outputs.made);
    BOOST_TEST_REQUIRE(channels.size() == 3U);
    BOOST_TEST(channels[0].film.name == "albedo");
    BOOST_TEST(channels[0].film.components == 3U);
    BOOST_TEST((channels[0].source == output::Source::Raw));
    BOOST_TEST(channels[1].film.name == "depth");
    BOOST_TEST(channels[1].film.components == 1U);
    BOOST_TEST(channels[1].sourceName == "z");
    BOOST_TEST(channels[2].film.name == "diffuse");
    BOOST_TEST((channels[2].source == output::Source::Lpe));
    BOOST_TEST(channels[2].sourceName == "C<RD>L");
}

// a channel no output names is not rendered
BOOST_AUTO_TEST_CASE(moya_outputs_a_channel_no_output_names_is_left_out_test) {
    scene::Scene made(scene::kinds());
    made.create("channel", "alone");
    made.commit();
    made.apply();
    BOOST_TEST(output::channelsOf(made).empty());
}

BOOST_AUTO_TEST_CASE(moya_outputs_an_output_reads_its_file_format_precision_and_compression_test) {
    const Outputs outputs;
    const std::vector<output::Output> read = output::outputsOf(outputs.made);
    BOOST_TEST_REQUIRE(read.size() == 2U);
    BOOST_TEST(read[0].file == "first.exr");
    BOOST_TEST((read[0].format == output::Format::Exr));
    BOOST_TEST(read[0].channels == (std::vector<std::string> { "albedo", "depth" }), boost::test_tools::per_element());
    BOOST_TEST((read[0].precision == output::Precision::Half));
    BOOST_TEST((read[0].compression == output::Compression::Zip));
    BOOST_TEST(read[1].channels == (std::vector<std::string> { "depth", "diffuse" }), boost::test_tools::per_element());
    BOOST_TEST((read[1].precision == output::Precision::Float));
    BOOST_TEST((read[1].compression == output::Compression::Piz));
}

BOOST_AUTO_TEST_CASE(moya_outputs_a_files_name_gives_its_format_test) {
    BOOST_TEST((output::formatOfFile("frame.png") == output::Format::Png));
    BOOST_TEST((output::formatOfFile("FRAME.PNG") == output::Format::Png));
    BOOST_TEST((output::formatOfFile("frame.exr") == output::Format::Exr));
    BOOST_TEST((output::formatOfFile("png") == output::Format::Exr));
}

// the render makes its film from the scene's channels. A source the path tracer does not fill
// holds zero
BOOST_AUTO_TEST_CASE(moya_outputs_the_render_holds_the_scenes_channels_test) {
    const Outputs outputs;
    v3d::moya::device::CpuDevice device;
    const film::Snapshot picture = integrate::render(device, outputs.made, {}, colour::Config());
    const std::vector<std::string> names { "albedo", "depth", "diffuse" };
    BOOST_TEST_REQUIRE(picture.channels.size() == names.size());
    for (std::size_t index = 0; index < names.size(); ++index) {
        BOOST_TEST(picture.channels[index].name == names[index]);
    }
    BOOST_TEST(picture.at(kCentre, kCentre, picture.offset("albedo")) == 0.0F);
    BOOST_TEST(picture.at(kCentre, kCentre, picture.offset("depth")) == 0.0F);
    BOOST_TEST(picture.at(kCentre, kCentre, 0) > 0.0F);
}

// an EXR output holds the beauty and its own channels, at its precision
BOOST_AUTO_TEST_CASE(moya_outputs_an_exr_output_holds_the_beauty_and_its_channels_test) {
    const Outputs outputs;
    v3d::moya::device::CpuDevice device;
    const film::Snapshot picture = integrate::render(device, outputs.made, {}, colour::Config());
    output::Output second = output::outputsOf(outputs.made)[1];
    const boost::filesystem::path file = boost::filesystem::temp_directory_path() / "v3d_moya_outputs_second.exr";
    second.file = file.string();
    output::write(second, picture, colour::Config(), {});

    std::vector<std::string> names;
    {
        const Imf::InputFile read(file.string().c_str());
        for (auto channel = read.header().channels().begin(); channel != read.header().channels().end(); ++channel) {
            names.emplace_back(channel.name());
            BOOST_TEST((channel.channel().type == Imf::FLOAT), channel.name());
        }
    }
    boost::filesystem::remove(file);
    // OpenEXR lists a file's channels by name
    BOOST_TEST(names == (std::vector<std::string> { "B", "G", "R", "depth", "diffuse.B", "diffuse.G", "diffuse.R" }), boost::test_tools::per_element());
}

// a checkpoint holds the film's channels, so a render stopped and resumed gives the bytes of one
// that never stopped
BOOST_AUTO_TEST_CASE(moya_outputs_a_checkpoint_resumes_the_channels_test) {
    const Outputs outputs(64);
    v3d::moya::device::CpuDevice device;
    const std::string path = (boost::filesystem::temp_directory_path() / "v3d_moya_outputs_checkpoint.exr").string();
    boost::system::error_code ignored;
    boost::filesystem::remove(path, ignored);
    const film::Snapshot whole = integrate::render(device, outputs.made, {}, colour::Config());
    integrate::render(device, outputs.made, {}, colour::Config(), { .path = path, .stop = [](std::uint32_t passes) { return passes == 2U; } });
    const film::Snapshot resumed = integrate::render(device, outputs.made, {}, colour::Config(), { .path = path, .resume = true });
    boost::filesystem::remove(path, ignored);
    BOOST_TEST(resumed.channels.size() == 3U);
    BOOST_TEST(resumed.passes == whole.passes);
    BOOST_TEST(resumed.values == whole.values, boost::test_tools::per_element());
}

};  // namespace
