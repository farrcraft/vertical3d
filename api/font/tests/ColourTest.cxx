/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/font/glyph/ColourGlyph.h>
#include <api/font/glyph/CurveStore.h>
#include <api/font/face/Face.h>
#include <api/font/face/FallbackChain.h>
#include <api/font/face/Family.h>
#include <api/font/glyph/Store.h>
#include <api/font/face/Library.h>
#include <api/font/glyph/Paint.h>
#include <api/font/text/ShapedRun.h>
#include <api/font/text/Shaper.h>
#include <api/font/internal/Composite.h>
#include <api/font/internal/FaceHandle.h>
#include <api/font/internal/SvgDocuments.h>
#include <api/log/Logger.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace {

/**
 * The first colour glyph's code point in each fixture.
 **/
constexpr char32_t FIRST = 0xE000;

/**
 * The size the fixtures are painted at: their em square is 20 pixels, from 4 below the baseline
 * to 16 above it.
 **/
constexpr float SIZE = 20.0F;

/**
 * The fixtures, one for each colour table, opened once for the suite's cases. tests/fonts holds
 * them and the script that writes them, which says what each glyph draws.
 **/
struct Fonts {
    Fonts() :
        logger(boost::make_shared<v3d::log::Logger>()),
        library(boost::make_shared<v3d::font::face::Library>()),
        colr0(open("fonts/colr0.ttf")),
        colr1(open("fonts/colr1.ttf")),
        svg(open("fonts/svg.ttf")),
        cbdt(open("fonts/cbdt.ttf")),
        sbix(open("fonts/sbix.ttf")),
        inter(open("data/fonts/Inter-Regular.ttf")) {
    }

    boost::shared_ptr<v3d::font::face::Face> open(const std::string& filename) const {
        boost::shared_ptr<v3d::font::face::Face> face = v3d::font::face::Face::open(library, filename, logger);
        BOOST_REQUIRE(face);
        return face;
    }

    /**
     * @return the key of a fixture's colour glyph by its place after the first
     **/
    static v3d::font::glyph::Key key(const boost::shared_ptr<v3d::font::face::Face>& face, unsigned int glyph, float size = SIZE) {
        return v3d::font::glyph::Key{ .face = face.get(), .index = face->glyphIndex(FIRST + glyph), .size = size, .mode = v3d::font::glyph::Mode::Colour,
            .foreground = v3d::font::glyph::packColour(glm::vec4(1.0F)) };
    }

    boost::shared_ptr<v3d::log::Logger> logger;
    boost::shared_ptr<v3d::font::face::Library> library;
    boost::shared_ptr<v3d::font::face::Face> colr0;
    boost::shared_ptr<v3d::font::face::Face> colr1;
    boost::shared_ptr<v3d::font::face::Face> svg;
    boost::shared_ptr<v3d::font::face::Face> cbdt;
    boost::shared_ptr<v3d::font::face::Face> sbix;
    boost::shared_ptr<v3d::font::face::Face> inter;
};

/**
 * @return a pixel of a colour bitmap, as red, green, blue and alpha bytes
 **/
glm::ivec4 at(const v3d::font::glyph::Bitmap& bitmap, unsigned int x, unsigned int y) {
    const std::size_t pixel = ((static_cast<std::size_t>(y) * bitmap.size.x) + x) * 4;
    return glm::ivec4(bitmap.pixels[pixel], bitmap.pixels[pixel + 1], bitmap.pixels[pixel + 2], bitmap.pixels[pixel + 3]);
}

/**
 * @return whether a pixel is within a step of each byte of a colour
 **/
bool near(const glm::ivec4& pixel, const glm::ivec4& colour, int step = 2) {
    const glm::ivec4 apart = glm::abs(pixel - colour);
    return apart.x <= step && apart.y <= step && apart.z <= step && apart.w <= step;
}

/**
 * The fixtures' colours, as bytes.
 **/
glm::ivec4 red() {
    return glm::ivec4(255, 0, 0, 255);
}

glm::ivec4 blue() {
    return glm::ivec4(0, 0, 255, 255);
}

glm::ivec4 green() {
    return glm::ivec4(0, 255, 0, 255);
}

BOOST_FIXTURE_TEST_SUITE(colour, Fonts)

BOOST_AUTO_TEST_CASE(a_face_reports_its_colour_tables) {
    BOOST_CHECK(colr0->colour().layers && !colr0->colour().paint);
    BOOST_CHECK(colr1->colour().paint);
    BOOST_CHECK(svg->colour().svg);
    BOOST_CHECK(cbdt->colour().cbdt && !cbdt->colour().sbix);
    BOOST_CHECK(sbix->colour().sbix && !sbix->colour().cbdt);
    BOOST_CHECK(!inter->colour().any());
}

/**
 * A glyph is coloured when any table draws it. An outline the colour glyphs are made of is not.
 **/
BOOST_AUTO_TEST_CASE(a_glyph_is_coloured_by_any_table) {
    for (const auto& face : { colr0, colr1, svg, cbdt, sbix }) {
        BOOST_CHECK(face->coloured(face->glyphIndex(FIRST)));
        BOOST_CHECK(!face->coloured(1));
    }
    BOOST_CHECK(!inter->coloured(inter->glyphIndex(U'A')));
}

/**
 * COLR version 0 reads as a solid fill for each layer, clipped by its outline, in the palette
 * asked for.
 **/
BOOST_AUTO_TEST_CASE(layers_read_in_a_palette) {
    const unsigned int index = colr0->glyphIndex(FIRST);
    const v3d::font::glyph::ColourGlyph first = v3d::font::glyph::colour(*colr0, index, 0, SIZE);
    BOOST_REQUIRE_EQUAL(first.layers.size(), 2U);
    BOOST_REQUIRE_EQUAL(first.layers[0].fill.stops.size(), 1U);
    BOOST_CHECK(first.layers[0].fill.stops[0].colour == glm::vec4(1.0F, 0.0F, 0.0F, 1.0F));
    BOOST_CHECK_EQUAL(first.layers[0].clips.size(), 1U);
    BOOST_CHECK_CLOSE(first.min.y, -0.2F, 0.01F);
    BOOST_CHECK_CLOSE(first.max.x, 1.0F, 0.01F);
    BOOST_CHECK(!first.foreground());

    const v3d::font::glyph::ColourGlyph second = v3d::font::glyph::colour(*colr0, index, 1, SIZE);
    BOOST_REQUIRE_EQUAL(second.layers.size(), 2U);
    BOOST_CHECK(second.layers[0].fill.stops[0].colour == glm::vec4(0.0F, 0.0F, 1.0F, 1.0F));
    // a palette the face does not have is its first
    BOOST_CHECK(v3d::font::glyph::colour(*colr0, index, 9, SIZE).layers[0].fill.stops[0].colour == glm::vec4(1.0F, 0.0F, 0.0F, 1.0F));
}

BOOST_AUTO_TEST_CASE(a_glyph_without_colour_reads_empty) {
    BOOST_CHECK(v3d::font::glyph::colour(*inter, inter->glyphIndex(U'A'), 0, SIZE).empty());
    BOOST_CHECK(v3d::font::glyph::colour(*colr0, 1, 0, SIZE).empty());
    BOOST_CHECK(v3d::font::glyph::paint(v3d::font::glyph::Key{ .face = inter.get(), .index = 1, .size = SIZE, .mode = v3d::font::glyph::Mode::Colour }).pixels.empty());
}

BOOST_AUTO_TEST_CASE(layers_are_painted_on_whole_pixels) {
    const v3d::font::glyph::Bitmap painted = v3d::font::glyph::paint(key(colr0, 0));
    BOOST_REQUIRE_EQUAL(painted.channels, 4U);
    BOOST_REQUIRE(painted.size == glm::uvec2(20));
    BOOST_CHECK(painted.bearing == glm::ivec2(0, -16));
    BOOST_CHECK(near(at(painted, 2, 10), red()));
    BOOST_CHECK(near(at(painted, 17, 10), blue()));
}

/**
 * A layer in the text's colour takes the colour the key holds, and keeps its alpha.
 **/
BOOST_AUTO_TEST_CASE(a_layer_takes_the_text_colour) {
    BOOST_CHECK(v3d::font::glyph::colour(*colr0, colr0->glyphIndex(FIRST + 1), 0, SIZE).foreground());
    v3d::font::glyph::Key tinted = key(colr0, 1);
    tinted.foreground = v3d::font::glyph::packColour(glm::vec4(0.0F, 1.0F, 0.0F, 0.5F));
    const v3d::font::glyph::Bitmap painted = v3d::font::glyph::paint(tinted);
    BOOST_REQUIRE(painted.size == glm::uvec2(20));
    BOOST_CHECK(near(at(painted, 2, 10), red()));
    BOOST_CHECK(near(at(painted, 17, 10), glm::ivec4(0, 255, 0, 128)));
}

/**
 * A step of a pixel moves the glyph right, so its last column is only partly covered.
 **/
BOOST_AUTO_TEST_CASE(a_step_moves_the_glyph_right) {
    v3d::font::glyph::Key moved = key(colr0, 0);
    moved.subpixel = 2;
    const v3d::font::glyph::Bitmap painted = v3d::font::glyph::paint(moved);
    BOOST_REQUIRE(painted.size == glm::uvec2(21, 20));
    BOOST_CHECK_LE(std::abs(at(painted, 0, 10).w - 128), 2);
    BOOST_CHECK_LE(std::abs(at(painted, 20, 10).w - 128), 2);
}

BOOST_AUTO_TEST_CASE(a_linear_gradient_runs_across_its_glyph) {
    const v3d::font::glyph::ColourGlyph read = v3d::font::glyph::colour(*colr1, colr1->glyphIndex(FIRST), 0, SIZE);
    BOOST_REQUIRE_EQUAL(read.layers.size(), 1U);
    BOOST_CHECK(read.layers[0].fill.kind == v3d::font::glyph::ColourFill::Kind::Linear);
    BOOST_CHECK_EQUAL(read.layers[0].fill.stops.size(), 2U);

    const v3d::font::glyph::Bitmap painted = v3d::font::glyph::paint(key(colr1, 0));
    BOOST_REQUIRE(painted.size == glm::uvec2(20));
    // each pixel's centre is a fortieth of the em further across, starting at a fortieth
    BOOST_CHECK(near(at(painted, 0, 10), glm::ivec4(249, 0, 6, 255)));
    BOOST_CHECK(near(at(painted, 10, 10), glm::ivec4(121, 0, 134, 255)));
    BOOST_CHECK(near(at(painted, 19, 10), glm::ivec4(6, 0, 249, 255)));
    // a gradient is the same down its glyph
    BOOST_CHECK(at(painted, 10, 0) == at(painted, 10, 19));
}

BOOST_AUTO_TEST_CASE(layers_take_the_palette_and_the_text_colour) {
    v3d::font::glyph::Key tinted = key(colr1, 1);
    tinted.foreground = v3d::font::glyph::packColour(glm::vec4(0.0F, 1.0F, 0.0F, 1.0F));
    tinted.palette = 1;
    const v3d::font::glyph::Bitmap painted = v3d::font::glyph::paint(tinted);
    BOOST_REQUIRE(painted.size == glm::uvec2(20));
    BOOST_CHECK(near(at(painted, 2, 10), blue()));
    BOOST_CHECK(near(at(painted, 17, 10), green()));
}

/**
 * The radial gradient grows from red at the centre to blue half an em out, then pads.
 **/
BOOST_AUTO_TEST_CASE(a_radial_gradient_grows_from_its_centre) {
    const v3d::font::glyph::Bitmap painted = v3d::font::glyph::paint(key(colr1, 2));
    BOOST_REQUIRE(painted.size == glm::uvec2(20));
    // the pixel beside the centre is a fortieth of the em across and down from it
    const int beside = static_cast<int>(std::lround(255.0 * std::sqrt(2.0) / 40.0 / 0.5));
    BOOST_CHECK(near(at(painted, 9, 10), glm::ivec4(255 - beside, 0, beside, 255)));
    BOOST_CHECK(near(at(painted, 0, 0), blue()));
}

/**
 * The sweep turns from red at the right of its centre, counter-clockwise, to blue all the way
 * round.
 **/
BOOST_AUTO_TEST_CASE(a_sweep_gradient_turns_about_its_centre) {
    const v3d::font::glyph::Bitmap painted = v3d::font::glyph::paint(key(colr1, 3));
    BOOST_REQUIRE(painted.size == glm::uvec2(20));
    // just above the centre's right, and just below it, the sweep's two ends meet
    BOOST_CHECK(near(at(painted, 19, 9), glm::ivec4(253, 0, 2, 255)));
    BOOST_CHECK(near(at(painted, 19, 10), glm::ivec4(2, 0, 253, 255)));
    // straight up from the centre is a little over a quarter of the way round
    const glm::ivec4 up = at(painted, 9, 0);
    BOOST_CHECK(up.x > 180 && up.x < 200 && up.z > 55 && up.z < 75);
}

BOOST_AUTO_TEST_CASE(a_transform_moves_a_layer) {
    const v3d::font::glyph::Bitmap painted = v3d::font::glyph::paint(key(colr1, 4));
    BOOST_REQUIRE(painted.size == glm::uvec2(10, 20));
    BOOST_CHECK(painted.bearing == glm::ivec2(10, -16));
    BOOST_CHECK(near(at(painted, 5, 10), green()));
}

/**
 * A bitmap glyph is its strike's picture, placed by the strike's metrics and scaled to the size.
 **/
BOOST_AUTO_TEST_CASE(a_bitmap_glyph_is_its_strike_scaled) {
    for (const auto& face : { cbdt, sbix }) {
        const v3d::font::glyph::ColourGlyph read = v3d::font::glyph::colour(*face, face->glyphIndex(FIRST), 0, 32.0F);
        BOOST_REQUIRE_EQUAL(read.images.size(), 1U);
        BOOST_CHECK(read.images[0].size == glm::uvec2(32));

        const v3d::font::glyph::Bitmap whole = v3d::font::glyph::paint(key(face, 0, 32.0F));
        BOOST_REQUIRE(whole.size == glm::uvec2(32));
        BOOST_CHECK(whole.bearing == glm::ivec2(0, -26));
        BOOST_CHECK(near(at(whole, 0, 0), red()));
        BOOST_CHECK(near(at(whole, 31, 31), blue()));

        const v3d::font::glyph::Bitmap half = v3d::font::glyph::paint(key(face, 0, 16.0F));
        BOOST_REQUIRE(half.size == glm::uvec2(16));
        BOOST_CHECK(near(at(half, 3, 8), red()));
        BOOST_CHECK(near(at(half, 12, 8), blue()));
    }
}

/**
 * An SVG glyph is its element in the document, with the rectangle each half uses drawn twice.
 **/
BOOST_AUTO_TEST_CASE(an_svg_glyph_is_read_from_its_element) {
    const v3d::font::glyph::ColourGlyph read = v3d::font::glyph::colour(*svg, svg->glyphIndex(FIRST), 0, SIZE);
    BOOST_REQUIRE_EQUAL(read.layers.size(), 2U);
    BOOST_CHECK_CLOSE(read.min.y, -0.2F, 0.01F);
    BOOST_CHECK_CLOSE(read.max.y, 0.8F, 0.01F);

    const v3d::font::glyph::Bitmap painted = v3d::font::glyph::paint(key(svg, 0));
    BOOST_REQUIRE(painted.size == glm::uvec2(20));
    BOOST_CHECK(painted.bearing == glm::ivec2(0, -16));
    BOOST_CHECK(near(at(painted, 2, 10), red()));
    BOOST_CHECK(near(at(painted, 17, 10), blue()));
}

/**
 * The fixture's SVG glyphs share one document, which is parsed the first time one is read and
 * kept for the rest.
 **/
BOOST_AUTO_TEST_CASE(an_svg_document_is_parsed_once) {
    const v3d::font::internal::SvgDocuments& documents = *svg->handle().documents;
    const std::size_t before = documents.parsed();
    for (unsigned int glyph = 0; glyph < 4; glyph++) {
        BOOST_CHECK(!v3d::font::glyph::colour(*svg, svg->glyphIndex(FIRST + glyph), 0, SIZE).empty());
    }
    BOOST_CHECK_EQUAL(documents.parsed(), before + 1);
}

BOOST_AUTO_TEST_CASE(an_svg_gradient_runs_across_its_glyph) {
    const v3d::font::glyph::Bitmap painted = v3d::font::glyph::paint(key(svg, 1));
    BOOST_REQUIRE(painted.size == glm::uvec2(20));
    BOOST_CHECK(near(at(painted, 0, 10), glm::ivec4(249, 0, 6, 255)));
    BOOST_CHECK(near(at(painted, 10, 10), glm::ivec4(121, 0, 134, 255)));
}

/**
 * A stroke is the outline drawn around its path, as thick as its width, with butt ends.
 **/
BOOST_AUTO_TEST_CASE(an_svg_stroke_is_drawn_around_its_path) {
    const v3d::font::glyph::Bitmap painted = v3d::font::glyph::paint(key(svg, 2));
    BOOST_REQUIRE(painted.size == glm::uvec2(20, 4));
    BOOST_CHECK(painted.bearing == glm::ivec2(0, -8));
    BOOST_CHECK(near(at(painted, 0, 0), green()));
    BOOST_CHECK(near(at(painted, 19, 3), green()));
}

BOOST_AUTO_TEST_CASE(an_svg_path_is_filled_by_its_rule) {
    const v3d::font::glyph::Bitmap painted = v3d::font::glyph::paint(key(svg, 3));
    BOOST_REQUIRE(painted.size == glm::uvec2(20));
    BOOST_CHECK(near(at(painted, 2, 10), blue()));
    BOOST_CHECK_EQUAL(at(painted, 10, 10).w, 0);
}

/**
 * A woman and a laptop joined by a zero width joiner are one emoji, which the emoji face shapes
 * to one glyph drawn in colour. The letter before it is set in Inter.
 **/
BOOST_AUTO_TEST_CASE(an_emoji_sequence_is_shaped_to_one_glyph) {
    const boost::shared_ptr<v3d::font::face::Face> emoji = open("data/fonts/NotoColorEmoji.ttf");
    BOOST_CHECK(emoji->colour().paint);
    const boost::shared_ptr<v3d::font::face::FallbackChain> chain = boost::make_shared<v3d::font::face::FallbackChain>();
    for (const auto& face : { inter, emoji }) {
        const boost::shared_ptr<v3d::font::face::Family> family = boost::make_shared<v3d::font::face::Family>(face->family());
        family->add(face);
        chain->add(family);
    }
    const v3d::font::text::Shaper shaper(chain);
    const std::vector<v3d::font::text::ShapedRun> runs = shaper.shape("a\U0001F469\u200D\U0001F4BB", {}, SIZE, {});
    BOOST_REQUIRE_EQUAL(runs.size(), 2U);
    BOOST_CHECK(runs[0].face == inter);
    BOOST_REQUIRE(runs[1].face == emoji);
    BOOST_REQUIRE_EQUAL(runs[1].glyphs.size(), 1U);
    const unsigned int index = runs[1].glyphs[0].index;
    BOOST_CHECK(emoji->coloured(index));
    BOOST_CHECK(!v3d::font::glyph::colour(*emoji, index, 0, SIZE).empty());
}

/**
 * A space after an emoji is set in the text's face, not the emoji face, whose space is wider.
 * A digit with the emoji variation selector and a keycap asks for emoji, and is one glyph of the
 * emoji face, though the text face has the digit.
 **/
BOOST_AUTO_TEST_CASE(a_cluster_asking_for_emoji_is_set_in_the_emoji_face) {
    const boost::shared_ptr<v3d::font::face::Face> emoji = open("data/fonts/NotoColorEmoji.ttf");
    const boost::shared_ptr<v3d::font::face::FallbackChain> chain = boost::make_shared<v3d::font::face::FallbackChain>();
    for (const auto& face : { inter, emoji }) {
        const boost::shared_ptr<v3d::font::face::Family> family = boost::make_shared<v3d::font::face::Family>(face->family());
        family->add(face);
        chain->add(family);
    }
    const v3d::font::text::Shaper shaper(chain);
    const std::vector<v3d::font::text::ShapedRun> spaced = shaper.shape("\U0001F600 a", {}, SIZE, {});
    BOOST_REQUIRE_EQUAL(spaced.size(), 2U);
    BOOST_CHECK(spaced[0].face == emoji);
    BOOST_CHECK_EQUAL(spaced[0].end, 4U);
    BOOST_CHECK(spaced[1].face == inter);

    const std::vector<v3d::font::text::ShapedRun> keycap = shaper.shape("1️⃣", {}, SIZE, {});
    BOOST_REQUIRE_EQUAL(keycap.size(), 1U);
    BOOST_CHECK(keycap[0].face == emoji);
    BOOST_REQUIRE_EQUAL(keycap[0].glyphs.size(), 1U);
    BOOST_CHECK(emoji->coloured(keycap[0].glyphs[0].index));

    // the text variation selector keeps a digit in the text face
    const std::vector<v3d::font::text::ShapedRun> text = shaper.shape("1︎", {}, SIZE, {});
    BOOST_REQUIRE_EQUAL(text.size(), 1U);
    BOOST_CHECK(text[0].face == inter);
}

/**
 * A colour glyph is stored as a program of steps among the band values, each fill with its clip
 * stored as a glyph's outline is, its gradient's mapping from em units, and its stops.
 **/
BOOST_AUTO_TEST_CASE(a_colour_glyph_is_stored_as_a_program) {
    v3d::font::glyph::CurveStore store;
    const v3d::font::glyph::StoredColour& linear = store.colour(*colr1, colr1->glyphIndex(FIRST), 0);
    BOOST_REQUIRE(!linear.empty);
    BOOST_CHECK(!linear.pictures);
    BOOST_CHECK_CLOSE(linear.max.y, 0.8F, 0.01F);
    const std::vector<float>& values = store.bands();
    const auto at = [&values](std::uint32_t value) { return values[value]; };
    const std::uint32_t step = linear.program + 4;
    BOOST_CHECK_EQUAL(at(linear.program), 1.0F);
    BOOST_CHECK_EQUAL(at(step), 0.0F);
    BOOST_CHECK_EQUAL(at(step + 3), 1.0F);
    // the clip is the em square, stored as a glyph: two bands each way of its four lines
    const auto clip = static_cast<std::uint32_t>(at(step + 4));
    BOOST_CHECK_EQUAL(at(clip + 4), 2.0F);
    BOOST_CHECK_EQUAL(at(step + 8), 1.0F);
    BOOST_CHECK_EQUAL(at(step + 10), 2.0F);
    // the gradient's space is the font's units, a thousand to the em
    BOOST_CHECK_CLOSE(at(step + 12), 1000.0F, 0.01F);
    BOOST_CHECK_CLOSE(at(step + 20), 1000.0F, 0.01F);
    const auto stops = static_cast<std::uint32_t>(at(step + 11));
    BOOST_CHECK_EQUAL(at(stops + 1), 1.0F);
    BOOST_CHECK_EQUAL(at(stops + 6), 1.0F);
    BOOST_CHECK_EQUAL(at(stops + 9), 1.0F);

    // a layer in the text's colour marks its stop, and a glyph is read once for each palette
    const v3d::font::glyph::StoredColour& layers = store.colour(*colr1, colr1->glyphIndex(FIRST + 1), 0);
    BOOST_CHECK_EQUAL(at(layers.program), 2.0F);
    const auto second = static_cast<std::uint32_t>(at(layers.program + 4 + v3d::font::glyph::CurveStore::STEP + 11));
    BOOST_CHECK_EQUAL(at(second + 5), 1.0F);
    const std::size_t read = store.read();
    store.colour(*colr1, colr1->glyphIndex(FIRST), 0);
    BOOST_CHECK_EQUAL(store.read(), read);
    store.colour(*colr1, colr1->glyphIndex(FIRST), 1);
    BOOST_CHECK_EQUAL(store.read(), read + 1);

    // a bitmap glyph places pictures, which no program draws, and an outline has no colour
    BOOST_CHECK(store.colour(*cbdt, cbdt->glyphIndex(FIRST), 0).pictures);
    BOOST_CHECK(store.colour(*colr1, 1, 0).empty);
}

BOOST_AUTO_TEST_CASE(composites_put_one_colour_on_another) {
    const glm::vec4 red(1.0F, 0.0F, 0.0F, 1.0F);
    const glm::vec4 halfBlue(0.0F, 0.0F, 0.5F, 0.5F);
    using v3d::font::glyph::ColourComposite;
    using v3d::font::internal::composite;
    BOOST_CHECK(composite(halfBlue, red, ColourComposite::SourceOver) == glm::vec4(0.5F, 0.0F, 0.5F, 1.0F));
    BOOST_CHECK(composite(halfBlue, red, ColourComposite::DestinationIn) == glm::vec4(0.5F, 0.0F, 0.0F, 0.5F));
    BOOST_CHECK(composite(halfBlue, red, ColourComposite::Clear) == glm::vec4(0.0F));
    // a full grey multiplied into red halves it
    BOOST_CHECK(composite(glm::vec4(0.5F, 0.5F, 0.5F, 1.0F), red, ColourComposite::Multiply) == glm::vec4(0.5F, 0.0F, 0.0F, 1.0F));
    // red's luminosity with a grey's hue and saturation is a grey as light as red
    const glm::vec4 grey = composite(glm::vec4(0.5F, 0.5F, 0.5F, 1.0F), red, ColourComposite::Hue);
    BOOST_CHECK_CLOSE(grey.r, 0.3F, 0.01F);
    BOOST_CHECK_CLOSE(grey.b, 0.3F, 0.01F);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
