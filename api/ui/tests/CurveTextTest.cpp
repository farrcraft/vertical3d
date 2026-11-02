/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/font/Face.h>
#include <api/font/Family.h>
#include <api/font/FallbackChain.h>
#include <api/font/Library.h>
#include <api/font/Paragraph.h>
#include <api/font/ShapingCache.h>
#include <api/font/TextStyle.h>
#include <api/log/Logger.h>
#include <api/render/realtime/Handle.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/ui/paint/CurvePages.h>
#include <api/ui/paint/CurveText.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>
#include <glm/mat3x3.hpp>
#include <glm/vec2.hpp>

namespace {

using v3d::render::realtime::TextureHandle;

/**
 * What the curve pages asked of the device.
 **/
struct Device {
    std::size_t created = 0;
    std::size_t updated = 0;
};

/**
 * Text in Inter, drawn from its curves through pages that tell a device what they do.
 **/
struct Drawing {
    Drawing() :
        logger(boost::make_shared<v3d::log::Logger>()),
        library(boost::make_shared<v3d::font::Library>()),
        cache(boost::make_shared<v3d::font::ShapingCache>("", logger)),
        text(v3d::ui::paint::CurvePages(
            [this](const float*, std::uint32_t, std::uint32_t) {
                device.created++;
                return TextureHandle(static_cast<std::uint32_t>(device.created));
            },
            [this](const TextureHandle&, const glm::uvec2&, const float*, const glm::uvec2&) { device.updated++; },
            [](const TextureHandle&) {})) {
        const boost::shared_ptr<v3d::font::Family> family = boost::make_shared<v3d::font::Family>("Inter");
        family->add(v3d::font::Face::open(library, "data/fonts/Inter-Regular.ttf", logger));
        BOOST_REQUIRE(!family->empty());
        style.fonts = boost::make_shared<v3d::font::FallbackChain>();
        style.fonts->add(family);
        style.size = 40.0F;
        canvas.resize(glm::uvec2(640, 480));
    }

    /**
     * Draw a string through a transform.
     **/
    void draw(const std::string& string, const glm::mat3& transform = glm::mat3(1.0F)) {
        const v3d::font::Paragraph paragraph(cache, string, style);
        text.draw(&canvas, paragraph, transform);
    }

    Device device;
    boost::shared_ptr<v3d::log::Logger> logger;
    boost::shared_ptr<v3d::font::Library> library;
    boost::shared_ptr<v3d::font::ShapingCache> cache;
    v3d::font::TextStyle style;
    v3d::ui::paint::CurveText text;
    v3d::render::realtime::canvas::Canvas canvas;
};

BOOST_FIXTURE_TEST_SUITE(curve_text_test, Drawing)

/**
 * Each glyph is one quad in a run of curves, whose corners carry their place in the em square
 * and where the glyph's bands start, and its curves reach the device as two textures.
 **/
BOOST_AUTO_TEST_CASE(each_glyph_is_a_quad_over_its_curves) {
    draw("Hi");
    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 8U);
    BOOST_CHECK_EQUAL(canvas.indices().size(), 12U);
    BOOST_REQUIRE_EQUAL(canvas.batches().size(), 1U);
    BOOST_CHECK(canvas.batches()[0].bands.valid());
    BOOST_CHECK_NE(canvas.vertices()[0].glyph, canvas.vertices()[4].glyph);
    // em y runs upwards, so the quad's top corner is the outline's top
    BOOST_CHECK_GT(canvas.vertices()[0].uv.y, canvas.vertices()[3].uv.y);
    BOOST_CHECK_LT(canvas.vertices()[0].position.y, canvas.vertices()[3].position.y);
    BOOST_CHECK_EQUAL(device.created, 2U);
}

/**
 * Glyphs drawn before are not sent again, and a new glyph is copied into the textures it fits.
 **/
BOOST_AUTO_TEST_CASE(curves_reach_the_device_once) {
    draw("Hi");
    draw("Hi");
    BOOST_CHECK_EQUAL(device.created, 2U);
    BOOST_CHECK_EQUAL(device.updated, 0U);
    BOOST_CHECK_EQUAL(text.store().read(), 2U);
    draw("Ho");
    BOOST_CHECK_EQUAL(device.created, 2U);
    BOOST_CHECK_EQUAL(device.updated, 2U);
}

/**
 * A transform places the quads: twice the scale makes a glyph twice as wide, but for the pixel
 * of margin on each side, and a quarter turn stands it on its side.
 **/
BOOST_AUTO_TEST_CASE(a_transform_places_the_quads) {
    draw("H");
    const float plain = canvas.vertices()[1].position.x - canvas.vertices()[0].position.x;
    canvas.clear();
    draw("H", glm::mat3(glm::vec3(2.0F, 0.0F, 0.0F), glm::vec3(0.0F, 2.0F, 0.0F), glm::vec3(0.0F, 0.0F, 1.0F)));
    const float doubled = canvas.vertices()[1].position.x - canvas.vertices()[0].position.x;
    // each side's margin is a pixel of the canvas, so two pixels of the plain width do not double
    BOOST_CHECK_CLOSE(doubled, ((plain - 2.0F) * 2.0F) + 2.0F, 0.5F);

    canvas.clear();
    draw("H", glm::mat3(glm::vec3(0.0F, 1.0F, 0.0F), glm::vec3(-1.0F, 0.0F, 0.0F), glm::vec3(0.0F, 0.0F, 1.0F)));
    // the quad's top edge, along x before the turn, now runs along y
    BOOST_CHECK_SMALL(canvas.vertices()[1].position.x - canvas.vertices()[0].position.x, 0.001F);
    BOOST_CHECK_CLOSE(std::abs(canvas.vertices()[1].position.y - canvas.vertices()[0].position.y), plain, 0.01F);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
