/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/scene/Description.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Kinds.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>

#include <limits>
#include <stdexcept>
#include <string>

#include <boost/test/unit_test.hpp>

namespace {

namespace scene = v3d::moya::scene;

BOOST_AUTO_TEST_CASE(moya_scene_a_camera_and_options_take_ris_defaults_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle camera = scene.create("camera", "main");
    const scene::Handle options = scene.create("options", "render");
    scene.commit();
    scene.apply();

    BOOST_TEST((scene.value(camera, "projection") == scene::Value::string("orthographic")));
    BOOST_TEST(scene.value(camera, "fov").floatArray().front() == 90.0F);
    BOOST_TEST(scene.value(camera, "near").floatArray().front() == 1.0e-10F);
    BOOST_TEST(scene.value(camera, "far").floatArray().front() == 1.0e38F);
    BOOST_TEST(scene.value(camera, "fstop").floatArray().front() == std::numeric_limits<float>::infinity());
    BOOST_TEST(scene.value(camera, "screen").size() == 0U);

    const scene::Value resolution = scene.value(options, "resolution");
    BOOST_TEST(resolution.integerArray().at(0) == 320);
    BOOST_TEST(resolution.integerArray().at(1) == 240);
    BOOST_TEST((scene.value(options, "filter") == scene::Value::string("gaussian")));
    BOOST_TEST((scene.value(options, "filterWidth") == scene::Value::floats({ 2.0F, 2.0F })));
    BOOST_TEST(scene.value(options, "samples").integerArray().front() == 4);
}

BOOST_AUTO_TEST_CASE(moya_scene_a_string_outside_its_choices_is_refused_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle camera = scene.create("camera", "main");
    const scene::Handle options = scene.create("options", "render");
    const scene::Handle output = scene.create("output", "beauty");
    scene.set(camera, "projection", scene::Value::string("perspective"));
    BOOST_CHECK_THROW(scene.set(camera, "projection", scene::Value::string("fisheye")), std::invalid_argument);
    BOOST_CHECK_THROW(scene.set(options, "filter", scene::Value::string("mitchell")), std::invalid_argument);
    BOOST_CHECK_THROW(scene.set(output, "compression", scene::Value::string("lzw")), std::invalid_argument);
    BOOST_CHECK_THROW(scene.set(output, "format", scene::Value::string("tiff")), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(moya_scene_a_fixed_count_is_held_to_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle options = scene.create("options", "render");
    const scene::Handle camera = scene.create("camera", "main");
    BOOST_CHECK_THROW(scene.set(options, "crop", scene::Value::floats({ 0.0F, 1.0F })), std::invalid_argument);
    BOOST_CHECK_THROW(scene.set(options, "resolution", scene::Value::integers({ 640 })), std::invalid_argument);
    scene.set(options, "resolution", scene::Value::integers({ 640, 480 }));
    scene.set(camera, "screen", scene::Value::floats({ -1.0F, 1.0F, -1.0F, 1.0F }));
}

BOOST_AUTO_TEST_CASE(moya_scene_an_output_names_its_channels_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle camera = scene.create("camera", "main");
    const scene::Handle albedo = scene.create("channel", "albedo");
    const scene::Handle output = scene.create("output", "beauty");
    const scene::Handle options = scene.create("options", "render");
    scene.set(albedo, "sourceName", scene::Value::string("albedo"));
    scene.set(output, "file", scene::Value::string("frame.exr"));
    scene.set(output, "channels", scene::Value::handles({ albedo }));
    scene.set(options, "camera", scene::Value::handle(camera));
    BOOST_CHECK_THROW(scene.set(output, "channels", scene::Value::handles({ camera })), std::invalid_argument);
    scene.commit();
    scene.apply();

    const std::string expected =
        "camera \"main\"\n"
        "    far float = 1e+38\n"
        "    focalDistance float = 0\n"
        "    focalLength float = 0\n"
        "    fov float = 90\n"
        "    fstop float = inf\n"
        "    near float = 1e-10\n"
        "    projection string = \"orthographic\"\n"
        "    screen float[0] =\n"
        "    shutter float[2] = 0 0\n"
        "    transform matrix = [1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1]\n"
        "channel \"albedo\"\n"
        "    components integer = 3\n"
        "    denoise integer = 0\n"
        "    filter string = \"default\"\n"
        "    ranks integer = 6\n"
        "    sourceName string = \"albedo\"\n"
        "    sourceType string = \"raw\"\n"
        "options \"render\"\n"
        "    background colour[0] =\n"
        "    camera handle = camera \"main\"\n"
        "    crop float[4] = 0 1 0 1\n"
        "    deepTolerance float = 0.001\n"
        "    denoise integer = 0\n"
        "    dicingCamera handle = none\n"
        "    diffuseDepth integer = 4\n"
        "    edgeLength float = 1\n"
        "    filter string = \"gaussian\"\n"
        "    filterWidth float[2] = 2 2\n"
        "    frame integer = 0\n"
        "    frameAspect float = 0\n"
        "    glossyDepth integer = 4\n"
        "    lightPicking string = \"tree\"\n"
        "    lightSampling string = \"mis\"\n"
        "    maxDepth integer = 12\n"
        "    minSamples integer = 16\n"
        "    noiseThreshold float = 0\n"
        "    pixelAspect float = 1\n"
        "    resolution integer[2] = 320 240\n"
        "    rouletteDepth integer = 3\n"
        "    samples integer = 4\n"
        "    shaderSearchPath string = \"\"\n"
        "    specularDepth integer = 12\n"
        "    textureDirectory string = \"\"\n"
        "    textureMemory integer = 1048576\n"
        "    textureSearchPath string = \"\"\n"
        "    transmissionDepth integer = 12\n"
        "output \"beauty\"\n"
        "    channels handle[1] = channel \"albedo\"\n"
        "    compression string = \"zip\"\n"
        "    file string = \"frame.exr\"\n"
        "    format string = \"exr\"\n"
        "    precision string = \"half\"\n";
    BOOST_TEST(describe(scene) == expected);
}

};  // namespace
