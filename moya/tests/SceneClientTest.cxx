/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <api/log/Logger.h>
#include <api/render/offline/rib/Reader.h>
#include <moya/libmoya/ri/SceneClient.h>
#include <moya/libmoya/scene/Kinds.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>

#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include "Handling.h"

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
    BOOST_TEST((read.value("options", "options", "variance") == scene::Value::number(0.01F)));
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
        "WorldBegin\nWorldEnd\n");
    const scene::Scene & made = read.scene();
    BOOST_TEST(made.objects("output").size() == 3U);
    BOOST_TEST((read.value("output", "first.exr", "format") == scene::Value::string("exr")));
    BOOST_TEST((read.value("output", "first.exr", "channels") == scene::Value::handles({})));
    BOOST_TEST((read.value("output", "depth.png", "format") == scene::Value::string("png")));
    BOOST_TEST((read.value("output", "depth.png", "channels") == scene::Value::handles({ *made.find("channel", "z") })));
    BOOST_TEST((read.value("channel", "z", "source") == scene::Value::string("z")));
    BOOST_TEST((read.value("output", "view", "format") == scene::Value::string("framebuffer")));

    // a display with no + replaces every one before it
    const Read replaced("Display \"first.exr\" \"file\" \"rgba\"\nDisplay \"second.exr\" \"file\" \"rgba\"\nWorldBegin\nWorldEnd\n");
    BOOST_TEST(replaced.scene().objects("output").size() == 1U);
    BOOST_TEST(replaced.scene().find("output", "second.exr").has_value());
}

BOOST_AUTO_TEST_CASE(moya_sceneclient_each_frame_is_a_commit_test) {
    const Read read(
        "FrameBegin 1\nFormat 64 48 1\nWorldBegin\nWorldEnd\nFrameEnd\n"
        "FrameBegin 2\nFormat 128 96 1\nWorldBegin\nWorldEnd\nFrameEnd\n");
    BOOST_TEST(read.frames() == (std::vector<std::int32_t> { 1, 2 }), boost::test_tools::per_element());
    BOOST_TEST(read.scene().version() == 2U);
    BOOST_TEST((read.value("options", "options", "resolution") == scene::Value::integers({ 128, 96 })));
}

};  // namespace
