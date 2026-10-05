/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/image/Image.h>
#include <api/image/reader/Png.h>
#include <api/render/realtime/Canvas.h>
#include <api/render/realtime/Frame.h>
#include <api/render/realtime/Grade.h>
#include <api/render/realtime/Pass.h>
#include <api/render/realtime/vulkan/frame/Capture.h>
#include <api/render/realtime/vulkan/frame/Recorder.h>
#include <api/render/realtime/vulkan/frame/RenderTarget.h>
#include <api/render/realtime/vulkan/renderer/FullScreen.h>

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <iterator>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

#include "Headless.h"
#include "Reference.h"

using v3d::render::realtime::Canvas;
using v3d::render::realtime::Frame;
using v3d::render::realtime::Grade;
using v3d::render::realtime::MaterialHandle;
using v3d::render::realtime::Pass;
using v3d::render::realtime::TextureHandle;
using v3d::render::realtime::vulkan::frame::Capture;
using v3d::render::realtime::vulkan::frame::Recorder;
using v3d::render::realtime::vulkan::frame::RenderTarget;
using v3d::render::realtime::vulkan::renderer::FullScreen;

namespace {

const VkFormat colourFormat = VK_FORMAT_R8G8B8A8_UNORM;

const uint32_t copyShader[] =
#include "shaders/copy.frag.inc"
;  // NOLINT(whitespace/semicolon) - the initialiser it terminates is the include above

/**
 * Record a frame whose every pass names its own target, then copy one target out.
 **/
void record(v3d::test::Headless* headless, const Frame& frame, const boost::shared_ptr<RenderTarget>& output,
    Capture* capture) {
    VkCommandBuffer commands = headless->context->ring()->begin();
    Recorder::record(commands, frame, Recorder::Target(), *headless->context->resources(),
        headless->context->frameUniforms().get());

    Capture::Source source;
    source.image = output->image();
    source.extent = output->extent();
    source.format = output->format();
    source.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    capture->record(commands, source);
    headless->submitAndWait(commands);
}

/**
 * Every value a channel can take, spread over a square so that red, green and blue all vary
 * independently - 64 by 64 is 4096 texels, and each channel sweeps 0 to 255 along its own axis.
 **/
std::vector<unsigned char> sweep(uint32_t size) {
    std::vector<unsigned char> texels(static_cast<std::size_t>(size) * size * 4);
    for (uint32_t y = 0; y < size; y++) {
        for (uint32_t x = 0; x < size; x++) {
            unsigned char* at = texels.data() + (static_cast<std::size_t>(y) * size + x) * 4;
            at[0] = static_cast<unsigned char>(x * 4 + (y & 3));
            at[1] = static_cast<unsigned char>(y * 4 + (x & 3));
            at[2] = static_cast<unsigned char>((x + y * size) & 255);
            at[3] = 255;
        }
    }
    return texels;
}

/**
 * The strip that turns every colour into its complement, baked as a game would bake one.
 **/
boost::shared_ptr<v3d::image::Image> invertingStrip() {
    const uint32_t size = Grade::SIZE;
    boost::shared_ptr<v3d::image::Image> strip = boost::make_shared<v3d::image::Image>(size * size, size, 24);
    for (uint32_t y = 0; y < size; y++) {
        for (uint32_t x = 0; x < size * size; x++) {
            unsigned char* at = strip->data() + (static_cast<std::size_t>(y) * size * size + x) * 3;
            at[0] = static_cast<unsigned char>(255 - (x % size) * 17);
            at[1] = static_cast<unsigned char>(255 - y * 17);
            at[2] = static_cast<unsigned char>(255 - (x / size) * 17);
        }
    }
    return strip;
}

/**
 * Draw the sweep into a scene a texel per pixel, grade it, and read back the graded picture.
 **/
boost::shared_ptr<v3d::image::Image> grade(v3d::test::Headless* headless, const boost::shared_ptr<v3d::image::Image>& strip,
    const std::vector<unsigned char>& texels, uint32_t size, const std::string& path) {
    boost::shared_ptr<RenderTarget> scene = boost::make_shared<RenderTarget>(headless->device, headless->context->ring(),
        size, size, colourFormat);
    boost::shared_ptr<RenderTarget> output = boost::make_shared<RenderTarget>(headless->device, headless->context->ring(),
        size, size, colourFormat);
    const TextureHandle uploaded = headless->context->textures()->texture(texels.data(), size, size, 4);

    Grade graded(headless->logger, headless->context, colourFormat, VK_FORMAT_UNDEFINED, strip);
    const MaterialHandle source = graded.source(*scene);

    Canvas canvas;
    canvas.resize(size, size);
    canvas.clear();
    canvas.rect(glm::vec2(0.0f), glm::vec2(static_cast<float>(size)), glm::vec2(0.0f), glm::vec2(1.0f), glm::vec4(1.0f),
        uploaded);

    // the grade is made first and reads the scene, so the frame draws the scene ahead of it
    Frame frame;
    boost::shared_ptr<Pass> post = frame.pass("grade");
    post->target(output);
    post->reads(scene);
    graded.submit(source, post.get());

    boost::shared_ptr<Pass> drawn = frame.pass("scene");
    drawn->target(scene);
    drawn->clearColour(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    headless->context->quads()->submit(canvas, drawn.get());

    Capture capture(headless->device, headless->logger);
    record(headless, frame, output, &capture);
    graded.release(source);

    if (!capture.write(path)) {
        return boost::shared_ptr<v3d::image::Image>();
    }
    v3d::image::reader::Png png(headless->logger);
    return png.read(path);
}

/**
 * Record a frame that draws the sweep into a scene and grades it into an output, with the
 * output's capture recorded after it, and hand back the commands unsubmitted.
 **/
VkCommandBuffer recordGrade(v3d::test::Headless* headless, const Grade& graded, const MaterialHandle& source,
    const boost::shared_ptr<RenderTarget>& scene, const boost::shared_ptr<RenderTarget>& output,
    const TextureHandle& sweepTexture, uint32_t size, Capture* capture) {
    Canvas canvas;
    canvas.resize(size, size);
    canvas.clear();
    canvas.rect(glm::vec2(0.0f), glm::vec2(static_cast<float>(size)), glm::vec2(0.0f), glm::vec2(1.0f), glm::vec4(1.0f),
        sweepTexture);

    Frame frame;
    boost::shared_ptr<Pass> post = frame.pass("grade");
    post->target(output);
    post->reads(scene);
    graded.submit(source, post.get());

    boost::shared_ptr<Pass> drawn = frame.pass("scene");
    drawn->target(scene);
    drawn->clearColour(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    headless->context->quads()->submit(canvas, drawn.get());

    VkCommandBuffer commands = headless->context->ring()->begin();
    Recorder::record(commands, frame, Recorder::Target(), *headless->context->resources(),
        headless->context->frameUniforms().get());
    Capture::Source captured;
    captured.image = output->image();
    captured.extent = output->extent();
    captured.format = output->format();
    captured.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    capture->record(commands, captured);
    return commands;
}

/**
 * @return the sweep with every colour channel turned to its complement
 **/
std::vector<unsigned char> complement(const std::vector<unsigned char>& texels) {
    std::vector<unsigned char> inverted = texels;
    for (std::size_t at = 0; at < inverted.size(); at++) {
        if (at % 4 != 3) {
            inverted[at] = static_cast<unsigned char>(255 - inverted[at]);
        }
    }
    return inverted;
}

/**
 * The largest difference in any channel between a picture and what was expected of it.
 **/
int largest(const boost::shared_ptr<v3d::image::Image>& picture, const std::vector<unsigned char>& expected) {
    int most = 0;
    for (std::size_t at = 0; at < expected.size(); at++) {
        if (at % 4 != 3) {
            most = std::max(most, std::abs(static_cast<int>(picture->data()[at]) - static_cast<int>(expected[at])));
        }
    }
    return most;
}

};  // namespace

BOOST_AUTO_TEST_SUITE(post_test)

/**
 * A full-screen pass that copies is the identity: the quad case's picture, drawn into a target
 * and copied through FullScreen into another, is the committed quad reference byte for byte. The
 * copy reads by position, so the case is exact on any implementation and needs no reference of
 * its own.
 **/
BOOST_AUTO_TEST_CASE(a_copying_pass_is_the_identity) {
    const uint32_t width = 64;
    const uint32_t height = 32;
    v3d::test::Headless headless(colourFormat, width, height);
    boost::shared_ptr<RenderTarget> scene = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(),
        width, height, colourFormat);
    boost::shared_ptr<RenderTarget> output = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(),
        width, height, colourFormat);

    FullScreen::Spec spec;
    spec.name = "copy";
    spec.fragment.assign(std::begin(copyShader), std::end(copyShader));
    spec.colour = colourFormat;
    FullScreen copy(headless.device, headless.context->pipelineCache(), headless.context->resources(),
        headless.context->ring(), headless.context->frameUniforms(), spec);
    const MaterialHandle source = copy.source({headless.context->textures()->texture(*scene)});

    // the quad case's canvas, whose picture the reference holds
    Canvas canvas;
    canvas.resize(width, height);
    canvas.clear();
    canvas.rect(glm::vec2(16.0f, 8.0f), glm::vec2(48.0f, 24.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

    Frame frame;
    boost::shared_ptr<Pass> post = frame.pass("copy");
    post->target(output);
    post->reads(scene);
    copy.submit(source, post.get());

    boost::shared_ptr<Pass> drawn = frame.pass("scene");
    drawn->target(scene);
    drawn->clearColour(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    headless.context->quads()->submit(canvas, drawn.get());

    Capture capture(headless.device, headless.logger);
    record(&headless, frame, output, &capture);
    BOOST_CHECK(copy.release(source));

    BOOST_CHECK(headless.silent());
    v3d::test::checkReference(headless.logger, &capture, "quad");
}

/**
 * The identity table leaves a scene as it was, to within the filtering the implementation does
 * between entries. In exact arithmetic the result is the input, because every entry is its own
 * position and a linear blend of neighbours is the input. The precision of a filter's weights
 * is left to the implementation, so the case allows one step in any channel and has no
 * reference picture.
 **/
BOOST_AUTO_TEST_CASE(the_identity_grade_leaves_the_scene_within_a_step) {
    const uint32_t size = 64;
    v3d::test::Headless headless(colourFormat, size, size);
    const std::vector<unsigned char> texels = sweep(size);

    boost::shared_ptr<v3d::image::Image> picture =
        grade(&headless, boost::shared_ptr<v3d::image::Image>(), texels, size, "data_out/grade_identity.png");
    BOOST_REQUIRE(picture);
    BOOST_CHECK(headless.silent());

    const int most = largest(picture, texels);
    BOOST_TEST_MESSAGE("the identity grade moved a channel by at most " << most);
    BOOST_CHECK_LE(most, 1);
}

/**
 * A strip reaches the table the right way round: one baked to invert gives the complement of
 * every colour, which a slice, a row or a column read in the wrong order would not.
 **/
BOOST_AUTO_TEST_CASE(a_strip_grades_the_scene) {
    const uint32_t size = 64;
    v3d::test::Headless headless(colourFormat, size, size);
    const std::vector<unsigned char> texels = sweep(size);

    boost::shared_ptr<v3d::image::Image> picture = grade(&headless, invertingStrip(), texels, size, "data_out/grade_inverted.png");
    BOOST_REQUIRE(picture);
    BOOST_CHECK(headless.silent());

    std::vector<unsigned char> inverted = texels;
    for (std::size_t at = 0; at < inverted.size(); at++) {
        if (at % 4 != 3) {
            inverted[at] = static_cast<unsigned char>(255 - inverted[at]);
        }
    }
    const int most = largest(picture, inverted);
    BOOST_TEST_MESSAGE("the inverting grade missed the complement by at most " << most);
    BOOST_CHECK_LE(most, 1);
}

/**
 * A grade given a new table grades with it from the next frame, through the source handle made
 * before the swap: a grade built as the identity and given the inverting table gives the
 * complement. Texels of the wrong count are refused and change nothing.
 **/
BOOST_AUTO_TEST_CASE(a_replaced_table_regrades_its_sources) {
    const uint32_t size = 64;
    v3d::test::Headless headless(colourFormat, size, size);
    const std::vector<unsigned char> texels = sweep(size);
    boost::shared_ptr<RenderTarget> scene = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(),
        size, size, colourFormat);
    boost::shared_ptr<RenderTarget> output = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(),
        size, size, colourFormat);
    const TextureHandle uploaded = headless.context->textures()->texture(texels.data(), size, size, 4);

    Grade graded(headless.logger, headless.context, colourFormat, VK_FORMAT_UNDEFINED);
    const MaterialHandle source = graded.source(*scene);
    BOOST_CHECK(!graded.replace(std::vector<uint8_t>(16)));
    BOOST_REQUIRE(graded.replace(Grade::table(invertingStrip())));

    Capture capture(headless.device, headless.logger);
    headless.submitAndWait(recordGrade(&headless, graded, source, scene, output, uploaded, size, &capture));
    BOOST_CHECK(graded.release(source));
    BOOST_CHECK(headless.silent());

    BOOST_REQUIRE(capture.write("data_out/grade_replaced.png"));
    v3d::image::reader::Png png(headless.logger);
    const boost::shared_ptr<v3d::image::Image> picture = png.read("data_out/grade_replaced.png");
    BOOST_REQUIRE(picture);
    const int most = largest(picture, complement(texels));
    BOOST_TEST_MESSAGE("the replaced grade missed the complement by at most " << most);
    BOOST_CHECK_LE(most, 1);
}

/**
 * A table replaced while a frame that grades with the old one is still in flight is not
 * destroyed under it, and neither is the material that paired it with the scene. Only the
 * validation layer would report either, so the case checks that it reports no errors, and that
 * the frame after the swap grades with the new table.
 **/
BOOST_AUTO_TEST_CASE(a_table_replaced_in_flight_keeps_the_frame_silent) {
    const uint32_t size = 64;
    v3d::test::Headless headless(colourFormat, size, size);
    const std::vector<unsigned char> texels = sweep(size);
    boost::shared_ptr<RenderTarget> scene = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(),
        size, size, colourFormat);
    boost::shared_ptr<RenderTarget> output = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(),
        size, size, colourFormat);
    const TextureHandle uploaded = headless.context->textures()->texture(texels.data(), size, size, 4);

    Grade graded(headless.logger, headless.context, colourFormat, VK_FORMAT_UNDEFINED);
    const MaterialHandle source = graded.source(*scene);

    Capture before(headless.device, headless.logger);
    headless.submit(recordGrade(&headless, graded, source, scene, output, uploaded, size, &before));

    BOOST_REQUIRE(graded.replace(Grade::table(invertingStrip())));

    Capture after(headless.device, headless.logger);
    headless.submitAndWait(recordGrade(&headless, graded, source, scene, output, uploaded, size, &after));
    for (int frame = 0; frame < 3; frame++) {
        headless.submit(headless.context->ring()->begin());
    }
    headless.context->ring()->waitIdle();
    BOOST_CHECK(graded.release(source));
    BOOST_CHECK(headless.silent());

    BOOST_REQUIRE(after.write("data_out/grade_replaced_in_flight.png"));
    v3d::image::reader::Png png(headless.logger);
    const boost::shared_ptr<v3d::image::Image> picture = png.read("data_out/grade_replaced_in_flight.png");
    BOOST_REQUIRE(picture);
    BOOST_CHECK_LE(largest(picture, complement(texels)), 1);
}

BOOST_AUTO_TEST_SUITE_END()
