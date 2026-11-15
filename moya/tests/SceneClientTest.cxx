/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <api/log/Logger.h>
#include <api/render/offline/rib/Reader.h>
#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/integrate/Render.h>
#include <moya/libmoya/ri/SceneClient.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Kinds.h>
#include <moya/libmoya/scene/Samples.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>

#include <algorithm>
#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include "Heard.h"

namespace {

namespace scene = v3d::moya::scene;

/**
 * A scene read from RIB through the client, applied at each frame's commit. It counts the
 * frames, and keeps each frame's options.frame.
 **/
class Read final {
 public:
    explicit Read(const std::string & source) : scene_(scene::kinds()), logger_(boost::make_shared<v3d::log::Logger>()) {
        const v3d::moya::handling::Heard heard(logger_->get());
        v3d::moya::ri::SceneClient client(scene_, logger_);
        client.frame([this]() {
            scene_.apply();
            const scene::Handle options = *scene_.find("options", "options");
            frames_.push_back(scene_.value(options, "frame").integerArray().front());
        });
        v3d::render::offline::rib::Reader reader(logger_);
        std::istringstream stream(source);
        BOOST_REQUIRE(reader.read(stream, &client));
        warned_ = heard.said("changes nothing") || heard.said("not in order");
    }

    const scene::Scene & scene() const { return scene_; }
    const std::vector<std::int32_t> & frames() const { return frames_; }
    bool warned() const { return warned_; }

    const scene::Value & value(const std::string & kind, const std::string & name, const std::string & parameter) const {
        return scene_.value(*scene_.find(kind, name), parameter);
    }

 private:
    scene::Scene scene_;
    boost::shared_ptr<v3d::log::Logger> logger_;
    std::vector<std::int32_t> frames_;
    bool warned_ { false };
};

BOOST_AUTO_TEST_CASE(moya_sceneclient_the_options_and_camera_follow_the_frames_requests_test) {
    const Read read(
        "Format 640 480 1.5\n"
        "FrameAspectRatio 2\n"
        "ScreenWindow -2 2 -1 1\n"
        "CropWindow 0.25 0.75 0 1\n"
        "Projection \"perspective\" \"fov\" [40]\n"
        "Clipping 0.1 100\n"
        "DepthOfField 8 0.1 3\n"
        "Shutter 0 0.5\n"
        "PixelSamples 4 2\n"
        "PixelFilter \"box\" 1 1\n"
        "PixelVariance 0.01\n"
        "WorldBegin\nWorldEnd\n");
    BOOST_TEST((read.value("options", "options", "resolution") == scene::Value::integers({ 640, 480 })));
    BOOST_TEST((read.value("options", "options", "pixelAspect") == scene::Value::number(1.5F)));
    BOOST_TEST((read.value("options", "options", "frameAspect") == scene::Value::number(2.0F)));
    BOOST_TEST((read.value("options", "options", "crop") == scene::Value::floats({ 0.25F, 0.75F, 0.0F, 1.0F })));
    BOOST_TEST((read.value("options", "options", "samples") == scene::Value::integer(8)));
    BOOST_TEST((read.value("options", "options", "filter") == scene::Value::string("box")));
    BOOST_TEST((read.value("options", "options", "filterWidth") == scene::Value::floats({ 1.0F, 1.0F })));
    BOOST_TEST((read.value("options", "options", "noiseThreshold") == scene::Value::number(0.01F)));
    BOOST_TEST((read.value("camera", "camera", "screen") == scene::Value::floats({ -2.0F, 2.0F, -1.0F, 1.0F })));
    BOOST_TEST((read.value("camera", "camera", "projection") == scene::Value::string("perspective")));
    BOOST_TEST((read.value("camera", "camera", "fov") == scene::Value::number(40.0F)));
    BOOST_TEST((read.value("camera", "camera", "near") == scene::Value::number(0.1F)));
    BOOST_TEST((read.value("camera", "camera", "far") == scene::Value::number(100.0F)));
    BOOST_TEST((read.value("camera", "camera", "fstop") == scene::Value::number(8.0F)));
    BOOST_TEST((read.value("camera", "camera", "shutter") == scene::Value::floats({ 0.0F, 0.5F })));
    BOOST_TEST((read.value("options", "options", "camera") == scene::Value::handle(*read.scene().find("camera", "camera"))));
}

BOOST_AUTO_TEST_CASE(moya_sceneclient_a_format_side_of_zero_takes_ris_default_test) {
    const Read read("Format 0 100 1\nWorldBegin\nWorldEnd\n");
    BOOST_TEST((read.value("options", "options", "resolution") == scene::Value::integers({ 320, 100 })));
}

BOOST_AUTO_TEST_CASE(moya_sceneclient_another_projection_changes_nothing_test) {
    const Read read("Projection \"fisheye\"\nWorldBegin\nWorldEnd\n");
    BOOST_TEST((read.value("camera", "camera", "projection") == scene::Value::string("orthographic")));
    BOOST_TEST(read.warned());
}

BOOST_AUTO_TEST_CASE(moya_sceneclient_the_camera_holds_camera_to_world_test) {
    const Read read("Translate 1 2 5\nWorldBegin\nWorldEnd\n");
    const glm::mat4 camera = read.value("camera", "camera", "transform").matrixAt(0);
    BOOST_TEST((camera[3] == glm::vec4(-1.0F, -2.0F, -5.0F, 1.0F)));
}

// Transform replaces the current transformation with its matrix as written, row by row, so the
// camera holds its inverse
BOOST_AUTO_TEST_CASE(moya_sceneclient_a_transform_replaces_the_cameras_matrix_test) {
    const Read read("Translate 9 9 9\nTransform [1 0 0 0  0 1 0 0  0 0 1 0  0 0 4 1]\nWorldBegin\nWorldEnd\n");
    const glm::mat4 camera = read.value("camera", "camera", "transform").matrixAt(0);
    BOOST_TEST((camera[3] == glm::vec4(0.0F, 0.0F, -4.0F, 1.0F)));
}

BOOST_AUTO_TEST_CASE(moya_sceneclient_depth_of_field_sets_the_lens_test) {
    const Read read("DepthOfField 8 0.1 3\nWorldBegin\nWorldEnd\n");
    BOOST_TEST((read.value("camera", "camera", "fstop") == scene::Value::number(8.0F)));
    BOOST_TEST((read.value("camera", "camera", "focalLength") == scene::Value::number(0.1F)));
    BOOST_TEST((read.value("camera", "camera", "focalDistance") == scene::Value::number(3.0F)));
}

// a screen window holds whatever comes after it, a Format or a Projection, and one given after a
// projection replaces the window without changing the projection
BOOST_AUTO_TEST_CASE(moya_sceneclient_a_screen_window_holds_in_any_order_test) {
    const Read before("ScreenWindow -2 2 -2 2\nFormat 64 48 1\nProjection \"perspective\"\nWorldBegin\nWorldEnd\n");
    BOOST_TEST((before.value("camera", "camera", "screen") == scene::Value::floats({ -2.0F, 2.0F, -2.0F, 2.0F })));
    const Read after("Projection \"orthographic\"\nScreenWindow 0 2 -1 1\nWorldBegin\nWorldEnd\n");
    BOOST_TEST((after.value("camera", "camera", "screen") == scene::Value::floats({ 0.0F, 2.0F, -1.0F, 1.0F })));
    BOOST_TEST((after.value("camera", "camera", "projection") == scene::Value::string("orthographic")));
}

BOOST_AUTO_TEST_CASE(moya_sceneclient_a_moving_camera_keeps_each_time_test) {
    const Read read(
        "Translate 0 1 0\n"
        "MotionBegin [0 0.25 1]\nTranslate 0 0 5\nTranslate 0 0 6\nTranslate 0 0 8\nMotionEnd\n"
        "WorldBegin\nWorldEnd\n");
    const scene::Samples & samples = read.scene().samples(*read.scene().find("camera", "camera"), "transform");
    BOOST_TEST(samples.times == (std::vector<float> { 0.0F, 0.25F, 1.0F }), boost::test_tools::per_element());
    BOOST_TEST(samples.values.size() == 3U);
    // each time's request builds on the transformation the block opened with
    BOOST_TEST((samples.values[1].matrixAt(0)[3] == glm::vec4(0.0F, -1.0F, -6.0F, 1.0F)));
    BOOST_TEST((samples.values[2].matrixAt(0)[3] == glm::vec4(0.0F, -1.0F, -8.0F, 1.0F)));
}

BOOST_AUTO_TEST_CASE(moya_sceneclient_a_block_whose_times_do_not_rise_keeps_its_first_test) {
    const Read read("MotionBegin [1 0]\nTranslate 0 0 5\nTranslate 0 0 6\nMotionEnd\nWorldBegin\nWorldEnd\n");
    const scene::Samples & samples = read.scene().samples(*read.scene().find("camera", "camera"), "transform");
    BOOST_TEST(samples.times.empty());
    BOOST_TEST((samples.values.front().matrixAt(0)[3] == glm::vec4(0.0F, 0.0F, -5.0F, 1.0F)));
    BOOST_TEST(read.warned());
}

BOOST_AUTO_TEST_CASE(moya_sceneclient_displays_add_and_replace_outputs_test) {
    const Read read(
        "Display \"first.exr\" \"file\" \"rgba\"\n"
        "Display \"+depth.png\" \"file\" \"z\"\n"
        "Display \"+view\" \"framebuffer\" \"rgb\"\n"
        "Display \"+diffuse.exr\" \"file\" \"lpe:C<RD>L\"\n"
        "WorldBegin\nWorldEnd\n");
    const scene::Scene & made = read.scene();
    BOOST_TEST(made.objects("output").size() == 4U);
    BOOST_TEST((read.value("output", "first.exr", "format") == scene::Value::string("exr")));
    BOOST_TEST((read.value("output", "first.exr", "channels") == scene::Value::handles({})));
    BOOST_TEST((read.value("output", "depth.png", "format") == scene::Value::string("png")));
    BOOST_TEST((read.value("output", "depth.png", "channels") == scene::Value::handles({ *made.find("channel", "z") })));
    // a mode names a value the renderer computes, RenderMan's z the depth, or a light path
    // expression after lpe:
    BOOST_TEST((read.value("channel", "z", "sourceType") == scene::Value::string("raw")));
    BOOST_TEST((read.value("channel", "z", "sourceName") == scene::Value::string("depth")));
    BOOST_TEST((read.value("channel", "lpe:C<RD>L", "sourceType") == scene::Value::string("lpe")));
    BOOST_TEST((read.value("channel", "lpe:C<RD>L", "sourceName") == scene::Value::string("C<RD>L")));
    BOOST_TEST((read.value("output", "view", "format") == scene::Value::string("framebuffer")));

    // a display with no + replaces every one before it
    const Read replaced("Display \"first.exr\" \"file\" \"rgba\"\nDisplay \"second.exr\" \"file\" \"rgba\"\nWorldBegin\nWorldEnd\n");
    BOOST_TEST(replaced.scene().objects("output").size() == 1U);
    BOOST_TEST(replaced.scene().find("output", "second.exr").has_value());
}

// a DisplayChannel declares a channel of its declaration's name, of its type's components, that
// collects its source: an expression after lpe:, or a value, RenderMan's z the depth. Its filter
// and denoise carry over. A Display's mode names its channels apart by commas, and Ci and a name
// the beauty. A display of the type deepexr is a deep file
BOOST_AUTO_TEST_CASE(moya_sceneclient_display_channels_name_what_an_output_holds_test) {
    const Read read(
        "DisplayChannel \"varying color diffuse\" \"string source\" [\"color lpe:C<RD>L\"] \"string filter\" [\"sum\"] \"int denoise\" [1]\n"
        "DisplayChannel \"float z\" \"string source\" [\"float z\"] \"string filter\" [\"zmin\"]\n"
        "Display \"out.exr\" \"openexr\" \"Ci,a, diffuse ,z\"\n"
        "Display \"+deep.exr\" \"deepexr\" \"rgba\"\n"
        "WorldBegin\nWorldEnd\n");
    const scene::Scene & made = read.scene();
    BOOST_TEST((read.value("output", "out.exr", "channels") == scene::Value::handles({ *made.find("channel", "diffuse"), *made.find("channel", "z") })));
    BOOST_TEST((read.value("channel", "diffuse", "sourceType") == scene::Value::string("lpe")));
    BOOST_TEST((read.value("channel", "diffuse", "sourceName") == scene::Value::string("C<RD>L")));
    BOOST_TEST((read.value("channel", "diffuse", "components") == scene::Value::integer(3)));
    BOOST_TEST((read.value("channel", "diffuse", "filter") == scene::Value::string("sum")));
    BOOST_TEST((read.value("channel", "diffuse", "denoise") == scene::Value::integer(1)));
    BOOST_TEST((read.value("channel", "z", "sourceName") == scene::Value::string("depth")));
    BOOST_TEST((read.value("channel", "z", "components") == scene::Value::integer(1)));
    BOOST_TEST((read.value("channel", "z", "filter") == scene::Value::string("closest")));
    BOOST_TEST((read.value("output", "deep.exr", "format") == scene::Value::string("deepexr")));
}

// RenderMan's PxrCryptomatte adds an output of its file, in floats, of one Cryptomatte layer of
// what its layer names. Option "moya" asks for the beauty denoised and sets the deep tolerance
BOOST_AUTO_TEST_CASE(moya_sceneclient_cryptomatte_and_denoising_come_from_ri_test) {
    const Read read(
        "SampleFilter \"PxrCryptomatte\" \"ids\" \"string filename\" [\"ids.exr\"] \"string layer\" [\"Material\"]\n"
        "Option \"moya\" \"int denoise\" [1] \"float deepTolerance\" [0.01]\n"
        "WorldBegin\nWorldEnd\n");
    const scene::Scene & made = read.scene();
    BOOST_TEST((read.value("output", "ids.exr", "channels") == scene::Value::handles({ *made.find("channel", "CryptoMaterial") })));
    BOOST_TEST((read.value("output", "ids.exr", "precision") == scene::Value::string("float")));
    BOOST_TEST((read.value("channel", "CryptoMaterial", "sourceName") == scene::Value::string("crypto_material")));
    BOOST_TEST((read.value("options", "options", "denoise") == scene::Value::integer(1)));
    BOOST_TEST(read.value("options", "options", "deepTolerance").floatArray().front() == 0.01F);
}

// a light's "string lightGroup" names its light group, on RI's LightSource and RenderMan's Light
BOOST_AUTO_TEST_CASE(moya_sceneclient_a_light_names_its_group_test) {
    const Read read(
        "WorldBegin\n"
        "Attribute \"identifier\" \"name\" [\"key\"]\n"
        "Light \"PxrRectLight\" \"keyLight\" \"string lightGroup\" [\"key\"]\n"
        "Attribute \"identifier\" \"name\" [\"fill\"]\n"
        "LightSource \"pointlight\" 1 \"string lightGroup\" [\"fill\"]\n"
        "WorldEnd\n");
    const scene::Scene & made = read.scene();
    std::vector<std::string> groups;
    for (const char * kind : { "rectLight", "sphereLight" }) {
        for (const scene::Handle & light : made.objects(kind)) {
            groups.push_back(made.value(light, "lightGroup").stringArray().front());
        }
    }
    std::sort(groups.begin(), groups.end());
    BOOST_TEST((groups == std::vector<std::string> { "fill", "key" }));
}

// a RIB file that names a light path expression and a light group's channel renders both: the
// dome in the group key lights the square, so both channels hold light where it lies
BOOST_AUTO_TEST_CASE(moya_sceneclient_an_expression_and_a_light_group_from_ri_render_test) {
    const Read read(
        "Format 16 16 1\n"
        "PixelSamples 4 4\n"
        "Projection \"perspective\" \"fov\" [40]\n"
        "Clipping 0.01 100\n"
        "Translate 0 0 5\n"
        "DisplayChannel \"color keyed\" \"string source\" [\"color lpe:C.*<L.'key'>\"]\n"
        "DisplayChannel \"color diffuse\" \"string source\" [\"color lpe:C<RD>L\"]\n"
        "Display \"lit.exr\" \"openexr\" \"Ci,keyed,diffuse\"\n"
        "WorldBegin\n"
        "Light \"PxrDomeLight\" \"sky\" \"string lightGroup\" [\"key\"]\n"
        "Surface \"matte\"\n"
        "Polygon \"P\" [-1 -1 0  1 -1 0  1 1 0  -1 1 0]\n"
        "WorldEnd\n");
    v3d::moya::device::CpuDevice device;
    const v3d::moya::film::Snapshot picture = v3d::moya::integrate::render(device, read.scene(), {}, v3d::moya::colour::Config());
    BOOST_TEST(picture.at(8, 8, picture.offset("keyed")) > 0.0F);
    BOOST_TEST(picture.at(8, 8, picture.offset("diffuse")) > 0.0F);
}

BOOST_AUTO_TEST_CASE(moya_sceneclient_each_frame_is_a_commit_test) {
    const Read read(
        "FrameBegin 1\nFormat 64 48 1\nWorldBegin\nWorldEnd\nFrameEnd\n"
        "FrameBegin 2\nFormat 128 96 1\nWorldBegin\nWorldEnd\nFrameEnd\n");
    BOOST_TEST(read.frames() == (std::vector<std::int32_t> { 1, 2 }), boost::test_tools::per_element());
    BOOST_TEST(read.scene().version() == 2U);
    BOOST_TEST((read.value("options", "options", "resolution") == scene::Value::integers({ 128, 96 })));
}

BOOST_AUTO_TEST_CASE(moya_sceneclient_ris_hiders_are_passed_over_test) {
    BOOST_TEST(!Read("Hider \"raytrace\"\nWorldBegin\nWorldEnd\n").warned());
    BOOST_TEST(!Read("Hider \"hidden\"\nWorldBegin\nWorldEnd\n").warned());
    BOOST_TEST(Read("Hider \"zbuffer\"\nWorldBegin\nWorldEnd\n").warned());
    BOOST_TEST(Read("Hider \"raytrace\" \"int incremental\" [1]\nWorldBegin\nWorldEnd\n").warned());
}

BOOST_AUTO_TEST_CASE(moya_sceneclient_the_trace_depth_sets_the_most_bounces_test) {
    const Read read("Option \"trace\" \"int maxdepth\" [3]\nWorldBegin\nWorldEnd\n");
    BOOST_TEST((read.value("options", "options", "maxDepth") == scene::Value::integer(3)));
    BOOST_TEST(!read.warned());

    const Read refused("Option \"trace\" \"int maxdepth\" [-1]\nWorldBegin\nWorldEnd\n");
    const Read none("WorldBegin\nWorldEnd\n");
    BOOST_TEST((refused.value("options", "options", "maxDepth") == none.value("options", "options", "maxDepth")));
    BOOST_TEST(refused.warned());
}

BOOST_AUTO_TEST_CASE(moya_sceneclient_options_moya_does_not_read_are_reported_test) {
    BOOST_TEST(!Read("Option \"searchpath\" \"string shader\" [\"shaders\"]\nWorldBegin\nWorldEnd\n").warned());
    BOOST_TEST(!Read("Option \"limits\" \"bucketsize\" [16 16] \"gridsize\" [256]\nWorldBegin\nWorldEnd\n").warned());
    BOOST_TEST(Read("Option \"limits\" \"int othreshold\" [1]\nWorldBegin\nWorldEnd\n").warned());
    BOOST_TEST(Read("Option \"statistics\" \"int endofframe\" [1]\nWorldBegin\nWorldEnd\n").warned());
}

BOOST_AUTO_TEST_CASE(moya_sceneclient_attributes_moya_does_not_read_are_reported_test) {
    BOOST_TEST(!Read("WorldBegin\nAttribute \"trace\" \"float bias\" [0.01]\nAttribute \"light\" \"int nsamples\" [4]\nWorldEnd\n").warned());
    BOOST_TEST(Read("WorldBegin\nAttribute \"visibility\" \"int camera\" [0]\nWorldEnd\n").warned());
    BOOST_TEST(Read("WorldBegin\nAttribute \"identifier\" \"string shadinggroup\" [\"a\"]\nWorldEnd\n").warned());
}

BOOST_AUTO_TEST_CASE(moya_sceneclient_a_displays_parameters_are_reported_test) {
    BOOST_TEST(!Read("Display \"out.exr\" \"openexr\" \"rgba\"\nWorldBegin\nWorldEnd\n").warned());
    BOOST_TEST(Read("Display \"out.exr\" \"openexr\" \"rgba\" \"string compression\" [\"zip\"]\nWorldBegin\nWorldEnd\n").warned());
}

};  // namespace
