/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/font/Bidi.h>
#include <api/font/Face.h>
#include <api/font/Family.h>
#include <api/font/FallbackChain.h>
#include <api/font/Library.h>
#include <api/font/Paragraph.h>
#include <api/font/ShapingCache.h>
#include <api/font/TextStyle.h>
#include <api/log/Logger.h>

#include <cstddef>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace {

const float kSize = 20.0F;
const float kTolerance = 0.001F;

/**
 * Paragraphs in Inter, falling back to Noto Sans, through one cache.
 **/
struct Typesetting {
    Typesetting() :
        logger(boost::make_shared<v3d::log::Logger>()),
        library(boost::make_shared<v3d::font::Library>()),
        chain(boost::make_shared<v3d::font::FallbackChain>()),
        cache(boost::make_shared<v3d::font::ShapingCache>("", logger)) {
        const boost::shared_ptr<v3d::font::Face> inter = v3d::font::Face::open(library, "data/fonts/Inter-Regular.ttf", logger);
        const boost::shared_ptr<v3d::font::Face> noto = v3d::font::Face::open(library, "data/fonts/NotoSans-Regular.ttf", logger);
        BOOST_REQUIRE(inter && noto);
        const boost::shared_ptr<v3d::font::Family> primary = boost::make_shared<v3d::font::Family>("Inter");
        primary->add(inter);
        const boost::shared_ptr<v3d::font::Family> fallback = boost::make_shared<v3d::font::Family>("Noto Sans");
        fallback->add(noto);
        chain->add(primary);
        chain->add(fallback);
        style.fonts = chain;
        style.size = kSize;
    }

    /**
     * @return a paragraph of a text in the test style
     **/
    v3d::font::Paragraph paragraph(const std::string& text, const v3d::font::ParagraphStyle& paragraphStyle = {}) const {
        return v3d::font::Paragraph(cache, text, style, paragraphStyle);
    }

    /**
     * @return how wide a text is on one line
     **/
    float width(const std::string& text) const {
        return paragraph(text).size().x;
    }

    boost::shared_ptr<v3d::log::Logger> logger;
    boost::shared_ptr<v3d::font::Library> library;
    boost::shared_ptr<v3d::font::FallbackChain> chain;
    boost::shared_ptr<v3d::font::ShapingCache> cache;
    v3d::font::TextStyle style;
};

BOOST_FIXTURE_TEST_SUITE(paragraph_test, Typesetting)

/**
 * Lines wrap after the last word that fits, with the space ending a line hanging past it, and
 * a word wider than the line breaks between its clusters.
 **/
BOOST_AUTO_TEST_CASE(lines_wrap_at_words_then_clusters) {
    const v3d::font::Paragraph wrapped = paragraph("one two three", { .width = width("one two") + 1.0F });
    BOOST_REQUIRE_EQUAL(wrapped.lines().size(), 2U);
    BOOST_CHECK_EQUAL(wrapped.lines()[0].end, 8U);
    BOOST_CHECK_EQUAL(wrapped.lines()[1].start, 8U);
    BOOST_CHECK_CLOSE(wrapped.lines()[0].width, width("one two"), kTolerance);
    BOOST_CHECK_CLOSE(wrapped.lines()[1].top, wrapped.lines()[0].height, kTolerance);

    const v3d::font::Paragraph word = paragraph("WWWW", { .width = width("W") * 1.5F });
    BOOST_REQUIRE_EQUAL(word.lines().size(), 4U);
    BOOST_CHECK_EQUAL(word.lines()[1].start, 1U);
}

/**
 * A line sits at the left, the right, or the centre of the width, and the start and the end
 * follow the paragraph's direction.
 **/
BOOST_AUTO_TEST_CASE(lines_align_across_the_width) {
    const float across = 300.0F;
    const auto offset = [this, across](v3d::font::Alignment alignment) {
        return paragraph("one", { .alignment = alignment, .width = across }).lines()[0].x;
    };
    const float room = across - width("one");
    BOOST_CHECK_SMALL(offset(v3d::font::Alignment::Left), kTolerance);
    BOOST_CHECK_CLOSE(offset(v3d::font::Alignment::Right), room, kTolerance);
    BOOST_CHECK_CLOSE(offset(v3d::font::Alignment::Center), room / 2.0F, kTolerance);
    BOOST_CHECK_SMALL(offset(v3d::font::Alignment::Start), kTolerance);
    BOOST_CHECK_CLOSE(offset(v3d::font::Alignment::End), room, kTolerance);
    const v3d::font::Paragraph rightToLeft = paragraph("one", { .direction = v3d::font::BaseDirection::RightToLeft, .width = across });
    BOOST_CHECK_CLOSE(rightToLeft.lines()[0].x, room, kTolerance);
}

/**
 * A paragraph with more lines than it shows ends its last line in an ellipsis, within the width.
 **/
BOOST_AUTO_TEST_CASE(a_cut_line_ends_in_an_ellipsis) {
    const float across = width("one two") + 1.0F;
    const v3d::font::Paragraph cut = paragraph("one two three", { .width = across, .maxLines = 1 });
    BOOST_REQUIRE_EQUAL(cut.lines().size(), 1U);
    const v3d::font::Line& line = cut.lines()[0];
    BOOST_REQUIRE(!line.runs.empty());
    BOOST_CHECK(line.runs.back().ellipsis);
    BOOST_CHECK(line.end < 8U);
    BOOST_CHECK(line.width <= across);
}

/**
 * A caret between a kerned pair is where the kerned second glyph starts, and one at the end is
 * at the text's width.
 **/
BOOST_AUTO_TEST_CASE(a_caret_follows_kerning) {
    const v3d::font::Paragraph kerned = paragraph("AV");
    const float first = kerned.lines()[0].runs[0].glyphs[0].advance;
    BOOST_CHECK(first < width("A"));
    BOOST_CHECK_CLOSE(kerned.caret(1, v3d::font::Affinity::Downstream).x, first, kTolerance);
    BOOST_CHECK_CLOSE(kerned.caret(2, v3d::font::Affinity::Upstream).x, kerned.size().x, kTolerance);
}

/**
 * At each edge of Hebrew in an English sentence, a caret going with the character before it
 * and one going with the character after it are drawn at the Hebrew run's two edges.
 **/
BOOST_AUTO_TEST_CASE(a_caret_follows_reordered_text) {
    // "I said ", shalom, " today": the Hebrew is bytes 7 to 15
    const v3d::font::Paragraph mixed = paragraph("I said \xD7\xA9\xD7\x9C\xD7\x95\xD7\x9D today");
    const std::vector<v3d::font::LineRun>& runs = mixed.lines()[0].runs;
    BOOST_REQUIRE_EQUAL(runs.size(), 3U);
    BOOST_CHECK(runs[1].rightToLeft);
    const float left = runs[1].x;
    const float right = runs[1].x + runs[1].advance;
    BOOST_CHECK_CLOSE(mixed.caret(7, v3d::font::Affinity::Upstream).x, left, kTolerance);
    BOOST_CHECK_CLOSE(mixed.caret(7, v3d::font::Affinity::Downstream).x, right, kTolerance);
    BOOST_CHECK_CLOSE(mixed.caret(15, v3d::font::Affinity::Upstream).x, left, kTolerance);
    BOOST_CHECK_CLOSE(mixed.caret(15, v3d::font::Affinity::Downstream).x, right, kTolerance);
}

/**
 * A point over a glyph's left half is the caret before it, and over its right half the caret
 * after it. Past the end of the text it is the end, and below the last line it is that line.
 **/
BOOST_AUTO_TEST_CASE(a_hit_takes_the_nearer_edge) {
    const v3d::font::Paragraph text = paragraph("AB");
    const float first = text.lines()[0].runs[0].glyphs[0].advance;
    const v3d::font::Hit left = text.hitTest(glm::vec2(first * 0.25F, 1.0F));
    BOOST_CHECK_EQUAL(left.index, 0U);
    BOOST_CHECK(left.affinity == v3d::font::Affinity::Downstream);
    const v3d::font::Hit right = text.hitTest(glm::vec2(first * 0.75F, 1.0F));
    BOOST_CHECK_EQUAL(right.index, 1U);
    BOOST_CHECK(right.affinity == v3d::font::Affinity::Upstream);
    BOOST_CHECK_EQUAL(text.hitTest(glm::vec2(1000.0F, 1000.0F)).index, 2U);
}

/**
 * A selection across a wrap is a rectangle on each line it covers, and a caret at the wrap is
 * on the line above upstream and on the line below downstream.
 **/
BOOST_AUTO_TEST_CASE(a_selection_and_a_caret_cross_a_wrap) {
    const v3d::font::Paragraph wrapped = paragraph("one two three", { .width = width("one two") + 1.0F });
    const std::vector<v3d::font::SelectionRect> rects = wrapped.selection(4, 11);
    BOOST_REQUIRE_EQUAL(rects.size(), 2U);
    BOOST_CHECK_SMALL(rects[0].position.y, kTolerance);
    BOOST_CHECK_CLOSE(rects[1].position.y, wrapped.lines()[1].top, kTolerance);
    BOOST_CHECK_SMALL(wrapped.caret(8, v3d::font::Affinity::Upstream).top, kTolerance);
    BOOST_CHECK_CLOSE(wrapped.caret(8, v3d::font::Affinity::Downstream).top, wrapped.lines()[1].top, kTolerance);
}

/**
 * Empty text is one empty line as tall as its font, and a newline at the end starts another.
 **/
BOOST_AUTO_TEST_CASE(an_empty_line_has_height) {
    const v3d::font::Paragraph empty = paragraph("");
    BOOST_REQUIRE_EQUAL(empty.lines().size(), 1U);
    BOOST_CHECK(empty.lines()[0].height > 0.0F);
    const v3d::font::Paragraph ended = paragraph("a\n");
    BOOST_REQUIRE_EQUAL(ended.lines().size(), 2U);
    BOOST_CHECK_EQUAL(ended.lines()[1].start, 2U);
    BOOST_CHECK_CLOSE(ended.caret(2, v3d::font::Affinity::Upstream).top, ended.lines()[1].top, kTolerance);
}

/**
 * A new width or a new colour shapes nothing again, the same text in another paragraph is
 * found, and a new size shapes it again.
 **/
BOOST_AUTO_TEST_CASE(shaping_is_kept_and_shared) {
    v3d::font::Paragraph text = paragraph("one two three");
    text.lines();
    const float narrow = width("one");
    const std::size_t shaped = cache->shaped();
    text.setWidth(narrow);
    BOOST_CHECK(text.lines().size() > 1U);
    v3d::font::TextStyle red = style;
    red.colour = glm::vec4(1.0F, 0.0F, 0.0F, 1.0F);
    text.setStyle(0, 3, red);
    text.lines();
    BOOST_CHECK_EQUAL(text.styles().size(), 2U);
    BOOST_CHECK(text.styleAt(1).colour == red.colour);
    paragraph("one two three").lines();
    BOOST_CHECK_EQUAL(cache->shaped(), shaped);

    v3d::font::TextStyle larger = style;
    larger.size = kSize * 2.0F;
    text.setStyle(4, 7, larger);
    text.lines();
    BOOST_CHECK_EQUAL(cache->shaped(), shaped + 1);
}

/**
 * Styling the middle of a range splits it in three, and styling it back joins it again.
 **/
BOOST_AUTO_TEST_CASE(styles_split_and_join) {
    v3d::font::Paragraph text = paragraph("one two three");
    v3d::font::TextStyle red = style;
    red.colour = glm::vec4(1.0F, 0.0F, 0.0F, 1.0F);
    text.setStyle(4, 7, red);
    BOOST_REQUIRE_EQUAL(text.styles().size(), 3U);
    BOOST_CHECK_EQUAL(text.styles()[1].start, 4U);
    BOOST_CHECK_EQUAL(text.styles()[1].end, 7U);
    BOOST_CHECK(text.styleAt(7).colour == style.colour);
    text.setStyle(4, 7, style);
    BOOST_CHECK_EQUAL(text.styles().size(), 1U);
}

/**
 * In a paragraph read right to left, the space ending a line hangs past its left edge.
 **/
BOOST_AUTO_TEST_CASE(trailing_space_hangs_left_in_right_to_left_text) {
    const float across = 300.0F;
    const v3d::font::Paragraph text = paragraph("abc ", { .direction = v3d::font::BaseDirection::RightToLeft, .width = across });
    const v3d::font::Line& line = text.lines()[0];
    BOOST_CHECK_CLOSE(line.width, width("abc"), kTolerance);
    BOOST_CHECK_CLOSE(line.x, across - width("abc"), kTolerance);
    BOOST_REQUIRE_EQUAL(line.runs.size(), 2U);
    BOOST_CHECK_EQUAL(line.runs[0].start, 3U);
    BOOST_CHECK_CLOSE(line.runs[0].x + line.runs[0].advance, line.x, kTolerance);
}

/**
 * A line limit with no ellipsis cuts at the last line shown, and the empty line after a closing
 * newline is dropped without an ellipsis.
 **/
BOOST_AUTO_TEST_CASE(a_line_limit_cuts_only_hidden_text) {
    const v3d::font::Paragraph plain = paragraph("one two three", { .width = width("one two") + 1.0F, .maxLines = 1, .ellipsis = false });
    BOOST_REQUIRE_EQUAL(plain.lines().size(), 1U);
    BOOST_CHECK(!plain.lines()[0].runs.back().ellipsis);
    BOOST_CHECK_EQUAL(plain.lines()[0].end, 7U);

    const v3d::font::Paragraph ended = paragraph("a\n", { .maxLines = 1 });
    BOOST_REQUIRE_EQUAL(ended.lines().size(), 1U);
    BOOST_CHECK(!ended.lines()[0].runs.back().ellipsis);
}

/**
 * A new direction shapes the text again and a new alignment does not, and a cache past its
 * capacity lets the text used longest ago go.
 **/
BOOST_AUTO_TEST_CASE(a_direction_reshapes_and_the_cache_forgets) {
    v3d::font::Paragraph text = paragraph("one");
    text.lines();
    const std::size_t shaped = cache->shaped();
    text.setParagraphStyle({ .alignment = v3d::font::Alignment::Center });
    text.lines();
    BOOST_CHECK_EQUAL(cache->shaped(), shaped);
    text.setParagraphStyle({ .direction = v3d::font::BaseDirection::RightToLeft });
    text.lines();
    BOOST_CHECK_EQUAL(cache->shaped(), shaped + 1);

    v3d::font::ShapingCache small("", logger, 1);
    const std::vector<v3d::font::StyledRange> ranges{ { .start = 0, .end = 1, .style = style } };
    small.shape("a", ranges, v3d::font::BaseDirection::Auto);
    small.shape("b", ranges, v3d::font::BaseDirection::Auto);
    small.shape("a", ranges, v3d::font::BaseDirection::Auto);
    BOOST_CHECK_EQUAL(small.shaped(), 3U);
    BOOST_CHECK_EQUAL(small.size(), 1U);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
