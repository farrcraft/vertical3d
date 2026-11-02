/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/asset/Manager.h>
#include <api/log/Logger.h>
#include <api/render/realtime/Handle.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/ui/paint/GlyphPages.h>
#include <api/ui/paint/Text.h>
#include <api/ui/paint/TextRenderer.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace {

// e with an acute accent, U+00E9, as UTF-8
const char* const eAcute = "\xC3\xA9";

/**
 * What the glyph pages asked of the device.
 **/
struct Device {
    std::size_t created = 0;
    std::size_t updated = 0;
};

/**
 * A renderer over the shared font whose pages are handed back without a device, since nothing
 * here draws to one.
 **/
boost::shared_ptr<v3d::ui::paint::TextRenderer> renderer(const v3d::ui::paint::TextRenderer::Faces& faces = v3d::ui::paint::TextRenderer::Faces(),
    Device* device = nullptr) {
    const boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    const boost::shared_ptr<v3d::asset::Manager> assets = boost::make_shared<v3d::asset::Manager>("data", logger);
    v3d::ui::paint::GlyphPages pages(
        [device](const unsigned char*, std::uint32_t) {
            if (device != nullptr) {
                device->created++;
            }
            return v3d::render::realtime::TextureHandle(1);
        },
        [device](const v3d::render::realtime::TextureHandle&, const glm::uvec2&, const unsigned char*, const glm::uvec2&) {
            if (device != nullptr) {
                device->updated++;
            }
        },
        [](const v3d::render::realtime::TextureHandle&) {});
    return boost::make_shared<v3d::ui::paint::TextRenderer>(assets, logger, std::move(pages), v3d::ui::paint::TextRenderer::baseSize, faces);
}

/**
 * @return the top left corner of each quad a canvas holds
 **/
std::vector<glm::vec2> corners(const v3d::render::realtime::canvas::Canvas& canvas) {
    std::vector<glm::vec2> found;
    for (std::size_t vertex = 0; vertex < canvas.vertices().size(); vertex += 4) {
        found.push_back(canvas.vertices()[vertex].position);
    }
    return found;
}

BOOST_AUTO_TEST_SUITE(text_renderer_test)

/**
 * Any character the faces have is measured and drawn, with no set of glyphs named up front, and
 * one written as several UTF-8 bytes is one glyph.
 **/
BOOST_AUTO_TEST_CASE(any_character_is_drawn) {
    Device device;
    const boost::shared_ptr<v3d::ui::paint::TextRenderer> text = renderer(v3d::ui::paint::TextRenderer::Faces(), &device);
    BOOST_REQUIRE(text->loaded());
    const float accent = text->width(eAcute);
    BOOST_REQUIRE(accent > 0.0F);
    BOOST_CHECK_CLOSE(text->width(std::string(eAcute) + eAcute), accent * 2.0F, 0.01F);

    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(640, 480));
    text->draw(&canvas, std::string("a ") + eAcute, glm::vec2(10.0F, 100.0F), glm::vec4(1.0F));
    // a quad for each letter, and none for the space between them
    BOOST_CHECK_EQUAL(canvas.vertices().size(), 8U);
    BOOST_CHECK_EQUAL(device.created, 1U);
    // a space at the end of a string counts towards its width
    BOOST_CHECK_GT(text->width("a "), text->width("a"));
}

/**
 * A glyph lands on whole pixels, with the baseline rounded to one, and a pen a quarter of a
 * pixel on is drawn from another rasterization of the glyph.
 **/
BOOST_AUTO_TEST_CASE(glyphs_land_on_whole_pixels) {
    const boost::shared_ptr<v3d::ui::paint::TextRenderer> text = renderer();
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(640, 480));
    text->draw(&canvas, "l", glm::vec2(10.0F, 100.4F), glm::vec4(1.0F), 12.0F);
    text->draw(&canvas, "l", glm::vec2(10.3F, 100.4F), glm::vec4(1.0F), 12.0F);
    const std::vector<glm::vec2> placed = corners(canvas);
    BOOST_REQUIRE_EQUAL(placed.size(), 2U);
    for (const glm::vec2& corner : placed) {
        BOOST_CHECK_EQUAL(corner.x, std::floor(corner.x));
        BOOST_CHECK_EQUAL(corner.y, std::floor(corner.y));
    }
    // the quarter step may carry the glyph's left edge over into the next pixel
    BOOST_CHECK(placed[1].x - placed[0].x == 0.0F || placed[1].x - placed[0].x == 1.0F);
    // the two steps are separate glyphs, so they sample different places in the page
    BOOST_CHECK(canvas.vertices()[0].uv != canvas.vertices()[4].uv);
}

/**
 * A larger size is rasterized at that size rather than scaled, so a letter half as tall again
 * is within a pixel of half as tall again.
 **/
BOOST_AUTO_TEST_CASE(a_new_size_is_rasterized_at_that_size) {
    const boost::shared_ptr<v3d::ui::paint::TextRenderer> text = renderer();
    const auto height = [&text](float size) {
        v3d::render::realtime::canvas::Canvas canvas;
        canvas.resize(glm::uvec2(640, 480));
        text->draw(&canvas, "H", glm::vec2(10.0F, 100.0F), glm::vec4(1.0F), size);
        BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 4U);
        return canvas.vertices()[2].position.y - canvas.vertices()[0].position.y;
    };
    const float small = height(12.0F);
    const float large = height(18.0F);
    BOOST_CHECK(std::abs(large - (small * 1.5F)) <= 1.5F);
    BOOST_CHECK(large > small);
}

/**
 * The renderer draws semibold text in its own face, wider than regular text.
 **/
BOOST_AUTO_TEST_CASE(semibold_text_is_set_in_its_own_face) {
    const boost::shared_ptr<v3d::ui::paint::TextRenderer> text = renderer();
    BOOST_REQUIRE(text->loaded());

    const float regular = text->width("Settings");
    const float semiBold = text->width("Settings", 0.0F, v3d::ui::paint::Weight::SemiBold);
    BOOST_CHECK_GT(semiBold, regular);
    BOOST_CHECK_CLOSE(text->measure(0.0F, v3d::ui::paint::Weight::SemiBold)("Settings"), semiBold, 0.01F);
}

/**
 * A character the regular face lacks is drawn from the fallback, and measures as it does in a
 * renderer of the fallback's face alone.
 **/
BOOST_AUTO_TEST_CASE(a_character_the_face_lacks_comes_from_the_fallback) {
    // U+01C4, which Noto Sans has and Inter does not
    const std::string dzCaron = "\xC7\x84";
    const boost::shared_ptr<v3d::ui::paint::TextRenderer> text = renderer();
    v3d::ui::paint::TextRenderer::Faces alone;
    alone.regular = v3d::ui::paint::TextRenderer::fallbackFont;
    alone.semiBold.clear();
    alone.fallback.clear();
    const boost::shared_ptr<v3d::ui::paint::TextRenderer> noto = renderer(alone);
    BOOST_REQUIRE(text->loaded());
    BOOST_REQUIRE(noto->loaded());

    BOOST_CHECK_GT(text->width(dzCaron), 0.0F);
    BOOST_CHECK_CLOSE(text->width(dzCaron), noto->width(dzCaron), 0.01F);
    BOOST_CHECK_CLOSE(text->width(dzCaron, 0.0F, v3d::ui::paint::Weight::SemiBold), noto->width(dzCaron), 0.01F);
}

/**
 * A semibold face or a fallback that cannot be loaded leaves the renderer working: semibold
 * text is set in the regular face.
 **/
BOOST_AUTO_TEST_CASE(a_missing_semibold_face_sets_semibold_text_regular) {
    v3d::ui::paint::TextRenderer::Faces missing;
    missing.semiBold = "fonts/missing.ttf";
    missing.fallback = "fonts/missing.ttf";
    const boost::shared_ptr<v3d::ui::paint::TextRenderer> text = renderer(missing);
    BOOST_REQUIRE(text->loaded());
    BOOST_CHECK_CLOSE(text->width("Settings", 0.0F, v3d::ui::paint::Weight::SemiBold), text->width("Settings"), 0.01F);
}

/**
 * A theme's weight number is read as CSS reads it: below 500 regular, 500 and above semibold.
 **/
BOOST_AUTO_TEST_CASE(a_weight_number_names_regular_or_semibold) {
    BOOST_CHECK(v3d::ui::paint::weight(400.0F) == v3d::ui::paint::Weight::Regular);
    BOOST_CHECK(v3d::ui::paint::weight(499.0F) == v3d::ui::paint::Weight::Regular);
    BOOST_CHECK(v3d::ui::paint::weight(500.0F) == v3d::ui::paint::Weight::SemiBold);
    BOOST_CHECK(v3d::ui::paint::weight(700.0F) == v3d::ui::paint::Weight::SemiBold);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
