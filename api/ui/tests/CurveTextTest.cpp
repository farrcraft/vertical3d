/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/font/Curves.h>
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
#include <api/render/realtime/canvas/DepthOrder.h>
#include <api/render/realtime/canvas/World.h>
#include <api/ui/paint/CurvePages.h>
#include <api/ui/paint/CurveText.h>
#include <api/ui/paint/GlyphPages.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>

namespace {

using v3d::render::realtime::TextureHandle;

/**
 * The texture every page of distance fields is given.
 **/
constexpr std::uint32_t FIELD_PAGE = 100;

/**
 * An outline two pixels wide.
 **/
constexpr v3d::font::TextEffect kOutline{ .colour = glm::vec4(0.0F, 0.0F, 0.0F, 1.0F), .width = 2.0F };

/**
 * A soft shadow three pixels below and to the right.
 **/
constexpr v3d::font::TextEffect kShadow{ .colour = glm::vec4(0.0F, 0.0F, 0.0F, 0.5F), .softness = 2.0F, .offset = glm::vec2(3.0F) };

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
            [](const TextureHandle&) {}),
            v3d::ui::paint::GlyphPages(
            [](const unsigned char*, std::uint32_t) { return TextureHandle(FIELD_PAGE); },
            [](const TextureHandle&, const glm::uvec2&, const unsigned char*, const glm::uvec2&) {},
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

/**
 * A paragraph stands in the world as glyph quads on the plane its transform gives, with the
 * world margin around each outline, and among other quads by its key.
 **/
BOOST_AUTO_TEST_CASE(a_paragraph_stands_in_the_world) {
    const v3d::font::Paragraph paragraph(cache, "H", style);
    v3d::render::realtime::canvas::World world;
    // a paragraph pixel is a hundredth of a world unit, laid on the ground plane
    glm::mat4 ground(1.0F);
    ground[0] = glm::vec4(0.01F, 0.0F, 0.0F, 0.0F);
    ground[1] = glm::vec4(0.0F, 0.0F, 0.01F, 0.0F);
    text.draw(&world, paragraph, ground);
    BOOST_REQUIRE_EQUAL(world.vertices().size(), 4U);
    BOOST_REQUIRE_EQUAL(world.batches().size(), 1U);
    BOOST_CHECK(world.batches()[0].bands.valid());
    for (const v3d::render::realtime::canvas::World::Vertex& corner : world.vertices()) {
        BOOST_CHECK_EQUAL(corner.position.y, 0.0F);
    }
    // the quad is as wide as the outline, and the world margin on either side
    const boost::shared_ptr<v3d::font::Face> face = style.fonts->faces({}).front();
    const v3d::font::CurveGlyph outlined = v3d::font::outline(*face, face->glyphIndex(U'H'));
    BOOST_CHECK_CLOSE(world.vertices()[1].uv.x - world.vertices()[0].uv.x,
        (outlined.max.x - outlined.min.x) + (2.0F * v3d::ui::paint::CurveText::WORLD_MARGIN), 0.01F);

    v3d::render::realtime::canvas::DepthOrder order;
    text.draw(&order, 1.0F, paragraph, ground);
    text.draw(&order, 2.0F, v3d::font::Paragraph(cache, "Hi", style), ground);
    v3d::render::realtime::canvas::World sorted;
    order.into(&sorted);
    // the further paragraph, of two glyphs, comes out first
    BOOST_REQUIRE_EQUAL(sorted.vertices().size(), 12U);
    BOOST_CHECK_NE(sorted.vertices()[0].glyph, sorted.vertices()[4].glyph);
    BOOST_CHECK_EQUAL(sorted.vertices()[8].glyph, world.vertices()[0].glyph);
}

/**
 * A style's effects are quads over each glyph's distance field, drawn before the glyphs. Each
 * glyph's quad lies inside its effect's, and every glyph's first effect is drawn before any
 * glyph's second. A shadow's quad is moved by its offset.
 **/
BOOST_AUTO_TEST_CASE(effects_are_drawn_beneath_the_glyphs) {
    draw("Hi");
    const std::size_t plain = canvas.vertices().size();
    const glm::vec2 glyphTopLeft = canvas.vertices()[0].position;
    const glm::vec2 glyphBottomRight = canvas.vertices()[2].position;
    canvas.clear();

    style.effects = { kShadow, kOutline };
    draw("Hi");
    // two effects for each of two glyphs, then the glyphs
    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), plain + 16U);
    BOOST_REQUIRE_EQUAL(canvas.batches().size(), 2U);
    BOOST_CHECK(canvas.batches()[0].text);
    BOOST_CHECK(canvas.batches()[0].texture == TextureHandle(FIELD_PAGE));
    BOOST_CHECK(canvas.batches()[1].bands.valid());

    const glm::vec2 shadowTopLeft = canvas.vertices()[0].position;
    const glm::vec2 outlineTopLeft = canvas.vertices()[8].position;
    const glm::vec2 outlineBottomRight = canvas.vertices()[10].position;
    BOOST_CHECK_EQUAL(canvas.vertices()[0].colour.a, 0.5F);
    BOOST_CHECK_EQUAL(canvas.vertices()[8].colour.a, 1.0F);
    BOOST_CHECK_NE(canvas.vertices()[0].glyph, 0U);
    BOOST_CHECK_NE(canvas.vertices()[0].glyph, canvas.vertices()[8].glyph);
    BOOST_CHECK_CLOSE(shadowTopLeft.x - 3.0F, outlineTopLeft.x, 0.01F);
    BOOST_CHECK_CLOSE(shadowTopLeft.y - 3.0F, outlineTopLeft.y, 0.01F);
    BOOST_CHECK_LT(outlineTopLeft.x, glyphTopLeft.x);
    BOOST_CHECK_LT(outlineTopLeft.y, glyphTopLeft.y);
    BOOST_CHECK_GT(outlineBottomRight.x, glyphBottomRight.x);
    BOOST_CHECK_GT(outlineBottomRight.y, glyphBottomRight.y);
}

/**
 * A field is made once for a glyph and serves it at every size and for every effect of one
 * reach, and the store lets fields go only across frames.
 **/
BOOST_AUTO_TEST_CASE(a_field_is_made_once) {
    style.effects = { kOutline, kOutline };
    draw("H");
    // twice the size and twice the width reach as far into the em
    style.size = 80.0F;
    style.effects = { v3d::font::TextEffect{ .width = 4.0F } };
    text.beginFrame();
    draw("H");
    BOOST_CHECK_EQUAL(text.fields().made(), 1U);
}

/**
 * In the world, a paragraph's effects are field quads drawn before its glyphs, and in depth
 * order they come before the glyphs at their key, by their layer.
 **/
BOOST_AUTO_TEST_CASE(effects_stand_in_the_world_beneath_the_glyphs) {
    style.effects = { kShadow, kOutline };
    const v3d::font::Paragraph paragraph(cache, "Hi", style);
    v3d::render::realtime::canvas::World world;
    text.draw(&world, paragraph, glm::mat4(1.0F));
    BOOST_REQUIRE_EQUAL(world.vertices().size(), 24U);
    BOOST_REQUIRE_EQUAL(world.batches().size(), 2U);
    BOOST_CHECK(world.batches()[0].field);
    BOOST_CHECK(world.batches()[0].texture == TextureHandle(FIELD_PAGE));
    BOOST_CHECK(world.batches()[1].bands.valid());
    BOOST_CHECK(!world.batches()[1].field);

    v3d::render::realtime::canvas::DepthOrder order;
    text.draw(&order, 1.0F, paragraph, glm::mat4(1.0F));
    v3d::render::realtime::canvas::World sorted;
    order.into(&sorted);
    BOOST_REQUIRE_EQUAL(sorted.vertices().size(), 24U);
    // both shadows, then both outlines, then the glyphs
    BOOST_CHECK_EQUAL(sorted.vertices()[0].colour.a, 0.5F);
    BOOST_CHECK_EQUAL(sorted.vertices()[4].colour.a, 0.5F);
    BOOST_CHECK_EQUAL(sorted.vertices()[8].colour.a, 1.0F);
    BOOST_CHECK_EQUAL(sorted.vertices()[8].glyph, world.vertices()[8].glyph);
    BOOST_CHECK(sorted.batches().back().bands.valid());
}

/**
 * A glyph drawn from its curves carries its style's contrast, on the screen and in the world.
 **/
BOOST_AUTO_TEST_CASE(glyphs_carry_their_style_contrast) {
    style.contrast = 0.6F;
    draw("H");
    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 4U);
    BOOST_CHECK_EQUAL(canvas.vertices()[0].contrast, 0.6F);

    v3d::render::realtime::canvas::World world;
    text.draw(&world, v3d::font::Paragraph(cache, "H", style), glm::mat4(1.0F));
    BOOST_REQUIRE_EQUAL(world.vertices().size(), 4U);
    BOOST_CHECK_EQUAL(world.vertices()[0].contrast, 0.6F);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
