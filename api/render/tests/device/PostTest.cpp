/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/image/Image.h>
#include <api/image/reader/Png.h>
#include <api/render/realtime/Handle.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/render/realtime/Frame.h>
#include <api/render/realtime/Grade.h>
#include <api/render/realtime/Pass.h>
#include <api/render/realtime/vulkan/frame/Capture.h>
#include <api/render/realtime/Recorder.h>
#include <api/render/realtime/RenderTarget.h>
#include <api/render/realtime/Material.h>
#include <api/render/realtime/Resources.h>
#include <api/render/realtime/renderer/FullScreen.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

#include "Headless.h"
#include "Reference.h"

using v3d::render::realtime::canvas::Canvas;
using v3d::render::realtime::Frame;
using v3d::render::realtime::Grade;
using v3d::render::realtime::MaterialHandle;
using v3d::render::realtime::Pass;
using v3d::render::realtime::TextureHandle;
using v3d::render::realtime::vulkan::frame::Capture;
using v3d::render::realtime::Recorder;
using v3d::render::realtime::RenderTarget;
using v3d::render::realtime::renderer::FullScreen;

namespace {

const VkFormat colourFormat = VK_FORMAT_R8G8B8A8_UNORM;

const uint32_t copyShader[] =
#include "shaders/copy.frag.inc"
;  // NOLINT(whitespace/semicolon) - the initialiser it terminates is the include above

/**
 * Every value a channel can take, spread over a square so that red, green and blue all vary
 * independently. A square of 64 by 64 is 4096 texels, and each channel sweeps 0 to 255 along
 * its own axis.
 **/
std::vector<unsigned char> sweep(uint32_t size) {
    std::vector<unsigned char> texels(static_cast<std::size_t>(size) * size * 4);
    for (uint32_t y = 0; y < size; y++) {
        for (uint32_t x = 0; x < size; x++) {
            unsigned char* at = texels.data() + (((static_cast<std::size_t>(y) * size) + x) * 4);
            at[0] = static_cast<unsigned char>((x * 4) + (y & 3));
            at[1] = static_cast<unsigned char>((y * 4) + (x & 3));
            at[2] = static_cast<unsigned char>((x + (y * size)) & 255);
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
    boost::shared_ptr<v3d::image::Image> strip = boost::make_shared<v3d::image::Image>(glm::uvec2(size * size, size), 24);
    for (uint32_t y = 0; y < size; y++) {
        for (uint32_t x = 0; x < size * size; x++) {
            unsigned char* at = strip->data() + (((static_cast<std::size_t>(y) * size * size) + x) * 3);
            at[0] = static_cast<unsigned char>(255 - ((x % size) * 17));
            at[1] = static_cast<unsigned char>(255 - (y * 17));
            at[2] = static_cast<unsigned char>(255 - ((x / size) * 17));
        }
    }
    return strip;
}

/**
 * Draw the sweep into a scene a texel per pixel, grade it, and read back the graded picture.
 *
 * @param sceneFormat the scene's format, which must be the one the headless context was made
 *        with, since its quads draw the sweep. A quad keeps its displayed value in either format
 * @param outputFormat what the grade draws into
 **/
boost::shared_ptr<v3d::image::Image> grade(v3d::test::Headless* headless, const boost::shared_ptr<v3d::image::Image>& strip,
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the scene's format, then the output's
    const std::vector<unsigned char>& texels, uint32_t size, const std::string& path, VkFormat sceneFormat = colourFormat,
    VkFormat outputFormat = colourFormat) {
    const boost::shared_ptr<RenderTarget> scene = boost::make_shared<RenderTarget>(*headless->context,
        RenderTarget::Spec{.width = size, .height = size, .colour = sceneFormat});
    const boost::shared_ptr<RenderTarget> output = boost::make_shared<RenderTarget>(*headless->context,
        RenderTarget::Spec{.width = size, .height = size, .colour = outputFormat});
    const TextureHandle uploaded = headless->context->textures()->texture(texels.data(), size, size, 4);

    Grade graded(*headless->context, Grade::Spec{.colour = outputFormat, .strip = strip});
    const MaterialHandle source = graded.source(*scene);

    Canvas canvas;
    canvas.resize(glm::uvec2(size, size));
    canvas.clear();
    canvas.rect(glm::vec2(0.0F), glm::vec2(static_cast<float>(size)), glm::vec2(0.0F), glm::vec2(1.0F), glm::vec4(1.0F),
        uploaded);

    // the grade is made first and reads the scene, so the frame draws the scene ahead of it
    Frame frame;
    const boost::shared_ptr<Pass> post = frame.pass("grade");
    post->target(output);
    post->reads(scene);
    graded.submit(source, post.get());

    const boost::shared_ptr<Pass> drawn = frame.pass("scene");
    drawn->target(scene);
    drawn->clearColour(glm::vec4(0.0F, 0.0F, 0.0F, 1.0F));
    headless->context->quads()->submit(canvas, drawn.get());

    Capture capture(headless->device, headless->context->ring(), headless->logger);
    headless->draw(frame, *output, &capture);
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
    canvas.resize(glm::uvec2(size, size));
    canvas.clear();
    canvas.rect(glm::vec2(0.0F), glm::vec2(static_cast<float>(size)), glm::vec2(0.0F), glm::vec2(1.0F), glm::vec4(1.0F),
        sweepTexture);

    Frame frame;
    const boost::shared_ptr<Pass> post = frame.pass("grade");
    post->target(output);
    post->reads(scene);
    graded.submit(source, post.get());

    const boost::shared_ptr<Pass> drawn = frame.pass("scene");
    drawn->target(scene);
    drawn->clearColour(glm::vec4(0.0F, 0.0F, 0.0F, 1.0F));
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
    const boost::shared_ptr<RenderTarget> scene = boost::make_shared<RenderTarget>(*headless.context,
        RenderTarget::Spec{.width = width, .height = height, .colour = colourFormat});
    const boost::shared_ptr<RenderTarget> output = boost::make_shared<RenderTarget>(*headless.context,
        RenderTarget::Spec{.width = width, .height = height, .colour = colourFormat});

    FullScreen::Spec spec;
    spec.name = "copy";
    spec.fragment.assign(std::begin(copyShader), std::end(copyShader));
    spec.colour = colourFormat;
    FullScreen copy(*headless.context, spec);
    const MaterialHandle source = copy.source({headless.context->textures()->texture(*scene)});

    // the quad case's canvas, whose picture the reference holds
    Canvas canvas;
    canvas.resize(glm::uvec2(width, height));
    canvas.clear();
    canvas.rect(glm::vec2(16.0F, 8.0F), glm::vec2(48.0F, 24.0F), glm::vec4(1.0F, 0.0F, 0.0F, 1.0F));

    Frame frame;
    const boost::shared_ptr<Pass> post = frame.pass("copy");
    post->target(output);
    post->reads(scene);
    copy.submit(source, post.get());

    const boost::shared_ptr<Pass> drawn = frame.pass("scene");
    drawn->target(scene);
    drawn->clearColour(glm::vec4(0.0F, 0.0F, 0.0F, 1.0F));
    headless.context->quads()->submit(canvas, drawn.get());

    Capture capture(headless.device, headless.context->ring(), headless.logger);
    headless.draw(frame, *output, &capture);
    BOOST_CHECK(copy.release(source));

    BOOST_CHECK(headless.silent());
    v3d::test::checkReference(headless.logger, &capture, "quad");
}

/**
 * A source released twice hands its set back once. The material goes on resolving for the frame
 * already queued, so a second release that found it and handed its set back again would leave
 * the pool giving one set to two sources.
 *
 * Passes before the change: a second release finds the material only while a released one goes
 * on resolving for its queued frame. The same change adds the rule that keeps it resolving.
 * Without that rule the first release removes the material, and the second finds nothing to
 * hand back.
 **/
BOOST_AUTO_TEST_CASE(a_source_released_twice_hands_its_set_back_once) {
    const uint32_t size = 16;
    v3d::test::Headless headless(colourFormat, size, size);
    const boost::shared_ptr<RenderTarget> scene = boost::make_shared<RenderTarget>(*headless.context,
        RenderTarget::Spec{.width = size, .height = size, .colour = colourFormat});

    FullScreen::Spec spec;
    spec.name = "copy";
    spec.fragment.assign(std::begin(copyShader), std::end(copyShader));
    spec.colour = colourFormat;
    FullScreen copy(*headless.context, spec);
    const TextureHandle texture = headless.context->textures()->texture(*scene);

    const MaterialHandle source = copy.source({texture});
    BOOST_CHECK(copy.release(source));
    BOOST_CHECK(!copy.release(source));

    for (uint32_t frame = 0; frame <= headless.context->ring()->framesInFlight(); frame++) {
        headless.submit(headless.context->ring()->begin());
    }
    headless.context->ring()->waitIdle();

    const MaterialHandle first = copy.source({texture});
    const MaterialHandle second = copy.source({texture});
    const v3d::render::realtime::Material* firstMaterial = headless.context->resources()->material(first);
    const v3d::render::realtime::Material* secondMaterial = headless.context->resources()->material(second);
    BOOST_REQUIRE(firstMaterial != nullptr && secondMaterial != nullptr);
    BOOST_CHECK(firstMaterial->set != secondMaterial->set);
    BOOST_CHECK(copy.release(first));
    BOOST_CHECK(copy.release(second));
    BOOST_CHECK(headless.silent());
}

/**
 * A full-screen pass destroyed after its source is queued, with the source released first, still
 * draws that source in the frame. The source's set belongs to the pass's pool, so the pool has
 * to outlive the frame as the material does.
 **/
BOOST_AUTO_TEST_CASE(a_pass_destroyed_after_its_source_is_queued_draws_it) {
    const uint32_t size = 16;
    v3d::test::Headless headless(colourFormat, size, size);
    const boost::shared_ptr<RenderTarget> output = boost::make_shared<RenderTarget>(*headless.context,
        RenderTarget::Spec{.width = size, .height = size, .colour = colourFormat});
    std::vector<unsigned char> texels;
    for (uint32_t texel = 0; texel < size * size; texel++) {
        texels.insert(texels.end(), {0x00, 0xFF, 0xFF, 0xFF});
    }
    const TextureHandle texture = headless.context->textures()->texture(texels.data(), size, size, 4);

    FullScreen::Spec spec;
    spec.name = "copy";
    spec.fragment.assign(std::begin(copyShader), std::end(copyShader));
    spec.colour = colourFormat;
    boost::shared_ptr<FullScreen> copy = boost::make_shared<FullScreen>(*headless.context, spec);
    const MaterialHandle source = copy->source({texture});

    Frame frame;
    const boost::shared_ptr<Pass> post = frame.pass("copy");
    post->target(output);
    copy->submit(source, post.get());
    BOOST_CHECK(copy->release(source));
    copy.reset();

    Capture capture(headless.device, headless.context->ring(), headless.logger);
    headless.draw(frame, *output, &capture);
    BOOST_CHECK(headless.silent());

    BOOST_REQUIRE(capture.write("data_out/post_destroyed_after_queueing.png"));
    v3d::image::reader::Png png(headless.logger);
    const boost::shared_ptr<v3d::image::Image> picture = png.read("data_out/post_destroyed_after_queueing.png");
    BOOST_REQUIRE(picture);
    const unsigned char* pixels = picture->data();
    for (uint32_t texel = 0; texel < size * size; texel++) {
        const unsigned char* at = pixels + (static_cast<std::size_t>(texel) * 4);
        if (at[0] != 0x00 || at[1] != 0xFF || at[2] != 0xFF || at[3] != 0xFF) {
            BOOST_ERROR("texel " << texel << " is not the queued source");
            return;
        }
    }
}

/**
 * The identity table leaves a scene as it was. Every entry is its own position, so a linear
 * blend of neighbours is the input, and the result matches it exactly in every channel.
 **/
BOOST_AUTO_TEST_CASE(the_identity_grade_leaves_the_scene_within_a_step) {
    const uint32_t size = 64;
    v3d::test::Headless headless(colourFormat, size, size);
    const std::vector<unsigned char> texels = sweep(size);

    const boost::shared_ptr<v3d::image::Image> picture =
        grade(&headless, boost::shared_ptr<v3d::image::Image>(), texels, size, "data_out/grade_identity.png");
    BOOST_REQUIRE(picture);
    BOOST_CHECK(headless.silent());

    const int most = largest(picture, texels);
    BOOST_TEST_MESSAGE("the identity grade moved a channel by at most " << most);
    BOOST_CHECK_EQUAL(most, 0);
}

/**
 * A strip reaches the table the right way round: one baked to invert gives the complement of
 * every colour, which a slice, a row or a column read in the wrong order would not.
 **/
BOOST_AUTO_TEST_CASE(a_strip_grades_the_scene) {
    const uint32_t size = 64;
    v3d::test::Headless headless(colourFormat, size, size);
    const std::vector<unsigned char> texels = sweep(size);

    const boost::shared_ptr<v3d::image::Image> picture = grade(&headless, invertingStrip(), texels, size, "data_out/grade_inverted.png");
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
    BOOST_CHECK_EQUAL(most, 0);
}

/**
 * A strip grades a scene in an sRGB target as the scene looks, not as the light it holds. The
 * inverting strip gives the complement of every value shown, whether the grade draws into an
 * sRGB target or a UNORM one. A table indexed by linear light would give the complement of the
 * light instead, which is a much brighter picture.
 **/
BOOST_AUTO_TEST_CASE(a_strip_grades_an_srgb_scene_as_it_looks) {
    const uint32_t size = 64;
    const VkFormat srgb = VK_FORMAT_R8G8B8A8_SRGB;
    v3d::test::Headless headless(srgb, size, size);
    const std::vector<unsigned char> texels = sweep(size);

    for (const VkFormat output : {srgb, colourFormat}) {
        const std::string path = output == srgb ? "data_out/grade_srgb_inverted.png" : "data_out/grade_srgb_to_unorm_inverted.png";
        const boost::shared_ptr<v3d::image::Image> picture = grade(&headless, invertingStrip(), texels, size, path, srgb, output);
        BOOST_REQUIRE(picture);
        const int most = largest(picture, complement(texels));
        BOOST_TEST_MESSAGE(path << " missed the complement by at most " << most);
        BOOST_CHECK_EQUAL(most, 0);
    }
    BOOST_CHECK(headless.silent());
}

/**
 * The identity table leaves a scene in an sRGB target as it was, drawn into an sRGB target: the
 * encode before the lookup and the decode after it cancel.
 *
 * Passes before the change: the grade read and wrote linear light, so nothing was encoded or
 * decoded that could fail to cancel.
 **/
BOOST_AUTO_TEST_CASE(the_identity_grade_leaves_an_srgb_scene_within_a_step) {
    const uint32_t size = 64;
    const VkFormat srgb = VK_FORMAT_R8G8B8A8_SRGB;
    v3d::test::Headless headless(srgb, size, size);
    const std::vector<unsigned char> texels = sweep(size);

    const boost::shared_ptr<v3d::image::Image> picture =
        grade(&headless, boost::shared_ptr<v3d::image::Image>(), texels, size, "data_out/grade_srgb_identity.png", srgb, srgb);
    BOOST_REQUIRE(picture);
    BOOST_CHECK(headless.silent());
    const int most = largest(picture, texels);
    BOOST_TEST_MESSAGE("the identity grade moved an sRGB channel by at most " << most);
    BOOST_CHECK_EQUAL(most, 0);
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
    const boost::shared_ptr<RenderTarget> scene = boost::make_shared<RenderTarget>(*headless.context,
        RenderTarget::Spec{.width = size, .height = size, .colour = colourFormat});
    const boost::shared_ptr<RenderTarget> output = boost::make_shared<RenderTarget>(*headless.context,
        RenderTarget::Spec{.width = size, .height = size, .colour = colourFormat});
    const TextureHandle uploaded = headless.context->textures()->texture(texels.data(), size, size, 4);

    Grade graded(*headless.context, Grade::Spec{.colour = colourFormat});
    const MaterialHandle source = graded.source(*scene);
    BOOST_CHECK(!graded.replace(std::vector<uint8_t>(16)));
    BOOST_REQUIRE(graded.replace(Grade::table(invertingStrip())));

    Capture capture(headless.device, headless.context->ring(), headless.logger);
    headless.submitAndWait(recordGrade(&headless, graded, source, scene, output, uploaded, size, &capture));
    BOOST_CHECK(graded.release(source));
    BOOST_CHECK(headless.silent());

    BOOST_REQUIRE(capture.write("data_out/grade_replaced.png"));
    v3d::image::reader::Png png(headless.logger);
    const boost::shared_ptr<v3d::image::Image> picture = png.read("data_out/grade_replaced.png");
    BOOST_REQUIRE(picture);
    const int most = largest(picture, complement(texels));
    BOOST_TEST_MESSAGE("the replaced grade missed the complement by at most " << most);
    BOOST_CHECK_EQUAL(most, 0);
}

/**
 * A frame grades with the old table and is submitted, and the table is replaced without waiting
 * for that frame. The validation layer reports nothing for it, and the frame after the swap
 * grades with the new table. replace() uploads the new table and waits for the queue to go
 * idle, so the submitted frame has finished before the old table and material are released.
 * The case checks that the validation layer reports no errors and that the second frame gives
 * the complement.
 **/
BOOST_AUTO_TEST_CASE(a_table_replaced_after_a_submit_keeps_the_frame_silent) {
    const uint32_t size = 64;
    v3d::test::Headless headless(colourFormat, size, size);
    const std::vector<unsigned char> texels = sweep(size);
    const boost::shared_ptr<RenderTarget> scene = boost::make_shared<RenderTarget>(*headless.context,
        RenderTarget::Spec{.width = size, .height = size, .colour = colourFormat});
    const boost::shared_ptr<RenderTarget> output = boost::make_shared<RenderTarget>(*headless.context,
        RenderTarget::Spec{.width = size, .height = size, .colour = colourFormat});
    const TextureHandle uploaded = headless.context->textures()->texture(texels.data(), size, size, 4);

    Grade graded(*headless.context, Grade::Spec{.colour = colourFormat});
    const MaterialHandle source = graded.source(*scene);

    Capture before(headless.device, headless.context->ring(), headless.logger);
    headless.submit(recordGrade(&headless, graded, source, scene, output, uploaded, size, &before));

    BOOST_REQUIRE(graded.replace(Grade::table(invertingStrip())));

    Capture after(headless.device, headless.context->ring(), headless.logger);
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

/**
 * A grade registers its table, and each source registers its scene. Destroying the grade
 * releases its table and any source still held. Once the frames in flight have finished, the
 * context holds the same number of textures as before the grade was made.
 **/
BOOST_AUTO_TEST_CASE(a_destroyed_grade_releases_its_table_and_sources) {
    const uint32_t size = 16;
    v3d::test::Headless headless(colourFormat, size, size);
    const boost::shared_ptr<RenderTarget> scene = boost::make_shared<RenderTarget>(*headless.context,
        RenderTarget::Spec{.width = size, .height = size, .colour = colourFormat});
    const std::size_t before = headless.context->resources()->textureCount();

    {
        Grade graded(*headless.context, Grade::Spec{.colour = colourFormat});
        BOOST_CHECK_EQUAL(headless.context->resources()->textureCount(), before + 1);
        BOOST_CHECK(graded.source(*scene).valid());
        BOOST_CHECK_EQUAL(headless.context->resources()->textureCount(), before + 2);
    }
    // released, and held until the frame that may still be queued against them has finished
    BOOST_CHECK_EQUAL(headless.context->resources()->textureCount(), before + 2);

    for (uint32_t frame = 0; frame <= headless.context->ring()->framesInFlight(); frame++) {
        headless.submit(headless.context->ring()->begin());
    }
    headless.context->ring()->waitIdle();
    BOOST_CHECK_EQUAL(headless.context->resources()->textureCount(), before);
    BOOST_CHECK(headless.silent());
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
