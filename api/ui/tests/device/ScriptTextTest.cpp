/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/asset/Manager.h>
#include <api/font/Paragraph.h>
#include <api/font/ShapedGlyph.h>
#include <api/image/Image.h>
#include <api/render/realtime/DeviceContext.h>
#include <api/render/realtime/Frame.h>
#include <api/render/realtime/Handle.h>
#include <api/render/realtime/Pass.h>
#include <api/render/realtime/RenderTarget.h>
#include <api/render/realtime/Textures.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/render/tests/device/Headless.h>
#include <api/ui/paint/GlyphPages.h>
#include <api/ui/paint/TextRenderer.h>

#include <vulkan/vulkan.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace {

constexpr std::uint32_t kWidth = 480;
constexpr std::uint32_t kHeight = 360;
constexpr float kSize = 32.0F;
constexpr float kLine = 48.0F;

/**
 * A line in each script the shared faces carry: Arabic, Hebrew, Devanagari, Thai, Chinese,
 * Japanese and Korean.
 **/
constexpr std::array<std::string_view, 7> kLines{
    "\xD8\xA7\xD9\x84\xD8\xB3\xD9\x84\xD8\xA7\xD9\x85 \xD8\xB9\xD9\x84\xD9\x8A\xD9\x83\xD9\x85",
    "\xD7\xA9\xD7\x9C\xD7\x95\xD7\x9D \xD7\xA2\xD7\x95\xD7\x9C\xD7\x9D",
    "\xE0\xA4\xA8\xE0\xA4\xAE\xE0\xA4\xB8\xE0\xA5\x8D\xE0\xA4\xA4\xE0\xA5\x87 \xE0\xA4\x95\xE0\xA4\xBF",
    "\xE0\xB8\xAA\xE0\xB8\xA7\xE0\xB8\xB1\xE0\xB8\xAA\xE0\xB8\x94\xE0\xB8\xB5",
    "\xE4\xBD\xA0\xE5\xA5\xBD\xE4\xB8\x96\xE7\x95\x8C",
    "\xE3\x81\x93\xE3\x82\x93\xE3\x81\xAB\xE3\x81\xA1\xE3\x81\xAF",
    "\xEC\x95\x88\xEB\x85\x95\xED\x95\x98\xEC\x84\xB8\xEC\x9A\x94",
};

/**
 * @return the brightest red in a rectangle of a picture, clamped to the picture
 **/
int brightest(v3d::image::Image& picture, const glm::vec2& low, const glm::vec2& high) {
    const auto clampTo = [](float value, std::uint32_t limit) {
        return static_cast<std::uint32_t>(std::clamp(value, 0.0F, static_cast<float>(limit)));  // checked: clamped to the picture
    };
    int found = 0;
    for (std::uint32_t y = clampTo(std::floor(low.y), kHeight); y < clampTo(std::ceil(high.y), kHeight); y++) {
        for (std::uint32_t x = clampTo(std::floor(low.x), kWidth); x < clampTo(std::ceil(high.x), kWidth); x++) {
            found = std::max<int>(found, picture.data()[((static_cast<std::size_t>(y) * kWidth) + x) * 4]);
        }
    }
    return found;
}

BOOST_AUTO_TEST_SUITE(script_text_test)

/**
 * A line in each script is drawn from the shared faces through the device, with the validation
 * layer silent. Every glyph is one the faces have, not the missing glyph, and its quad holds ink.
 **/
BOOST_AUTO_TEST_CASE(each_script_is_drawn_from_the_shared_faces) {
    v3d::test::Headless headless(VK_FORMAT_R8G8B8A8_UNORM, kWidth, kHeight);
    const boost::shared_ptr<v3d::render::realtime::Textures> textures = headless.context->textures();
    v3d::ui::paint::GlyphPages pages(
        [textures](const unsigned char* pixels, std::uint32_t side, std::uint32_t channels) { return textures->texture(pixels, side, side, channels); },
        [textures](const v3d::render::realtime::TextureHandle& page, const glm::uvec2& at, const unsigned char* pixels, const glm::uvec2& size,
            std::uint32_t channels) {
            textures->update(page, at, { .data = pixels, .width = size.x, .height = size.y, .channels = channels });
        },
        [textures](const v3d::render::realtime::TextureHandle& page) { textures->release(page); });
    const boost::shared_ptr<v3d::asset::Manager> assets = boost::make_shared<v3d::asset::Manager>("data", headless.logger);
    v3d::ui::paint::TextRenderer text(assets, headless.logger, std::move(pages));
    BOOST_REQUIRE(text.loaded());

    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(kWidth, kHeight));
    canvas.clear();
    std::vector<std::size_t> starts;
    for (std::size_t line = 0; line < kLines.size(); line++) {
        const v3d::font::Paragraph paragraph(text.shaping(), std::string(kLines[line]), text.style(kSize));
        BOOST_TEST_CONTEXT("line " << line) {
            for (const v3d::font::Line& shown : paragraph.lines()) {
                for (const v3d::font::LineRun& run : shown.runs) {
                    for (const v3d::font::ShapedGlyph& glyph : run.glyphs) {
                        BOOST_CHECK_NE(glyph.index, 0U);
                    }
                }
            }
        }
        starts.push_back(canvas.vertices().size());
        text.draw(&canvas, paragraph, glm::vec2(16.0F, (kLine * static_cast<float>(line + 1)) - 8.0F));
    }
    starts.push_back(canvas.vertices().size());

    const auto target = boost::make_shared<v3d::render::realtime::RenderTarget>(*headless.context,
        v3d::render::realtime::RenderTarget::Spec{ .width = kWidth, .height = kHeight, .colour = VK_FORMAT_R8G8B8A8_UNORM });
    v3d::render::realtime::Frame frame;
    const boost::shared_ptr<v3d::render::realtime::Pass> pass = frame.pass("text");
    pass->target(target);
    pass->clearColour(glm::vec4(0.0F, 0.0F, 0.0F, 1.0F));
    headless.context->quads()->submit(canvas, pass.get());
    const boost::shared_ptr<v3d::image::Image> picture = headless.picture(frame, *target, "data_out/script_text.png");
    BOOST_REQUIRE(picture);
    BOOST_CHECK(headless.silent());

    const std::vector<v3d::render::realtime::canvas::Canvas::Vertex>& vertices = canvas.vertices();
    for (std::size_t line = 0; line < kLines.size(); line++) {
        BOOST_TEST_CONTEXT("line " << line) {
            // two glyphs at least, each a quad of four corners from its top left
            BOOST_CHECK_GE(starts[line + 1] - starts[line], 8U);
            for (std::size_t quad = starts[line]; quad + 3 < starts[line + 1]; quad += 4) {
                BOOST_CHECK_GT(brightest(*picture, vertices[quad].position, vertices[quad + 2].position), 128);
            }
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
