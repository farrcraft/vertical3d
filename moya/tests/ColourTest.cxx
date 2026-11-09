/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/colour/Transform.h>
#include <OpenColorIO/OpenColorIO.h>

#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/filesystem/operations.hpp>
#include <boost/test/unit_test.hpp>

#include <glm/vec3.hpp>

namespace {

namespace colour = v3d::moya::colour;
namespace ocio = OCIO_NAMESPACE;

/** The display and view an 8-bit picture is shown through by default. **/
constexpr const char * kSrgb = "sRGB - Display";
constexpr const char * kSdr = "ACES 2.0 - SDR 100 nits (Rec.709)";

/** Colours from a mid grey to a bright one outside the unit cube. **/
constexpr glm::vec3 kColours[] = { { 0.18F, 0.18F, 0.18F }, { 1.0F, 0.0F, 0.0F }, { 0.05F, 0.4F, 0.9F }, { 4.0F, 2.5F, 0.3F } };

/** A colour through one of OpenColorIO's own processors for the built-in configuration. **/
glm::vec3 throughOcio(const ocio::ConstProcessorRcPtr & processor, const glm::vec3 & rgb) {
    float pixel[3] = { rgb.r, rgb.g, rgb.b };
    processor->getDefaultCPUProcessor()->applyRGB(pixel);
    return { pixel[0], pixel[1], pixel[2] };
}

ocio::ConstConfigRcPtr builtIn() {
    return ocio::Config::CreateFromFile(std::string(colour::Config::kBuiltIn).c_str());
}

BOOST_AUTO_TEST_CASE(moya_colour_the_built_in_working_space_is_acescg_test) {
    const colour::Config config;
    BOOST_TEST(config.name() == "cg-config-v4.0.0_aces-v2.0_ocio-v2.5");
    BOOST_TEST(config.workingSpace() == "ACEScg");
    BOOST_TEST(config.workingInteropId() == "lin_ap1_scene");
    BOOST_TEST(config.defaultDisplay() == kSrgb);
    BOOST_TEST(config.defaultView(kSrgb) == kSdr);
}

// the film's ACEScg is the built-in working space, so its display transform is OpenColorIO's own
// from ACEScg to the display and view
BOOST_AUTO_TEST_CASE(moya_colour_a_working_space_value_through_the_srgb_view_is_opencolorios_answer_test) {
    const colour::Transform display = colour::Config().display(kSrgb, kSdr);
    const ocio::ConstConfigRcPtr config = builtIn();
    const ocio::ConstProcessorRcPtr own = config->getProcessor("ACEScg", kSrgb, kSdr, ocio::TRANSFORM_DIR_FORWARD);
    for (const glm::vec3 & rgb : kColours) {
        const glm::vec3 ours = display.apply(rgb);
        const glm::vec3 theirs = throughOcio(own, rgb);
        for (int i = 0; i < 3; ++i) {
            BOOST_TEST(ours[i] == theirs[i], boost::test_tools::tolerance(0.00001F));
        }
    }
}

// sRGB's white is D65 and ACEScg's is ACES white, so white adapts to (1, 1, 1). Rec. 709 red is
// ACEScg (0.6131, 0.0701, 0.0206) under Bradford adaptation
BOOST_AUTO_TEST_CASE(moya_colour_an_srgb_input_converts_to_the_configurations_acescg_test) {
    const colour::Config config;
    const colour::Transform srgb = config.input("sRGB Encoded Rec.709 (sRGB)");
    const ocio::ConstProcessorRcPtr own = builtIn()->getProcessor("sRGB Encoded Rec.709 (sRGB)", "ACEScg");
    for (const glm::vec3 & rgb : kColours) {
        const glm::vec3 ours = srgb.apply(rgb);
        const glm::vec3 theirs = throughOcio(own, rgb);
        for (int i = 0; i < 3; ++i) {
            BOOST_TEST(ours[i] == theirs[i], boost::test_tools::tolerance(0.00001F));
        }
    }
    const glm::vec3 white = srgb.apply(glm::vec3(1.0F));
    const glm::vec3 red = config.input("Linear Rec.709 (sRGB)").apply({ 1.0F, 0.0F, 0.0F });
    for (int i = 0; i < 3; ++i) {
        BOOST_TEST(white[i] == 1.0F, boost::test_tools::tolerance(0.0002F));
    }
    BOOST_TEST(red.r == 0.6131F, boost::test_tools::tolerance(0.001F));
    BOOST_TEST(red.g == 0.0701F, boost::test_tools::tolerance(0.01F));
    BOOST_TEST(red.b == 0.0206F, boost::test_tools::tolerance(0.01F));
}

BOOST_AUTO_TEST_CASE(moya_colour_an_untagged_colour_is_in_the_working_space_test) {
    const colour::Config config;
    for (const glm::vec3 & rgb : kColours) {
        const glm::vec3 untagged = config.input("").apply(rgb);
        const glm::vec3 output = config.output().apply(rgb);
        for (int i = 0; i < 3; ++i) {
            BOOST_TEST(untagged[i] == rgb[i], boost::test_tools::tolerance(0.00001F));
            BOOST_TEST(output[i] == rgb[i], boost::test_tools::tolerance(0.00001F));
        }
    }
}

// a configuration whose working space is linear Rec. 709 writes its outputs there, and reads an
// untagged colour from there
BOOST_AUTO_TEST_CASE(moya_colour_another_working_space_converts_outputs_and_inputs_test) {
    const ocio::ConfigRcPtr edited = builtIn()->createEditableCopy();
    edited->setRole(ocio::ROLE_SCENE_LINEAR, "Linear Rec.709 (sRGB)");
    const std::string path = (boost::filesystem::temp_directory_path() / "v3d_colour_rec709.ocio").string();
    {
        std::ofstream file(path);
        edited->serialize(file);
    }
    const colour::Config config(path);
    BOOST_TEST(config.workingSpace() == "Linear Rec.709 (sRGB)");
    BOOST_TEST(config.workingInteropId() == "lin_rec709_scene");
    const glm::vec3 red = config.input("").apply({ 1.0F, 0.0F, 0.0F });
    BOOST_TEST(red.r == 0.6131F, boost::test_tools::tolerance(0.001F));
    const glm::vec3 back = config.output().apply(red);
    BOOST_TEST(back.r == 1.0F, boost::test_tools::tolerance(0.0001F));
    BOOST_TEST(back.g == 0.0F, boost::test_tools::tolerance(0.0001F));

    // a run of pixels converts in place, leaving the floats after the first three
    std::vector<float> pixels = { 0.6131F, 0.0701F, 0.0206F, 7.0F, 0.6131F, 0.0701F, 0.0206F, 8.0F };
    config.output().apply(pixels.data(), 2, 4);
    BOOST_TEST(pixels[0] == back.r, boost::test_tools::tolerance(0.0001F));
    BOOST_TEST(pixels[3] == 7.0F);
    BOOST_TEST(pixels[4] == pixels[0]);
    BOOST_TEST(pixels[7] == 8.0F);
    boost::filesystem::remove(path);
}

BOOST_AUTO_TEST_CASE(moya_colour_refuses_what_the_configuration_lacks_test) {
    const colour::Config config;
    BOOST_CHECK_THROW(config.input("No Such Space"), std::runtime_error);
    BOOST_CHECK_THROW(config.display(kSrgb, "No Such View"), std::runtime_error);
    BOOST_CHECK_THROW(colour::Config("no-such-config.ocio"), std::runtime_error);
}

};  // namespace
