/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/font/CurveStore.h>
#include <api/font/Curves.h>
#include <api/font/Face.h>
#include <api/font/Library.h>
#include <api/log/Logger.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>
#include <glm/vec4.hpp>

namespace {

/**
 * Inter, and a store of its curves.
 **/
struct Storing {
    Storing() :
        logger(boost::make_shared<v3d::log::Logger>()),
        library(boost::make_shared<v3d::font::Library>()),
        inter(v3d::font::Face::open(library, "data/fonts/Inter-Regular.ttf", logger)) {
        BOOST_REQUIRE(inter);
    }

    boost::shared_ptr<v3d::log::Logger> logger;
    boost::shared_ptr<v3d::font::Library> library;
    boost::shared_ptr<v3d::font::Face> inter;
    v3d::font::CurveStore store;
};

BOOST_FIXTURE_TEST_SUITE(curve_store_test, Storing)

/**
 * A glyph is read the first time it is asked for and found after, wherever it was stored.
 **/
BOOST_AUTO_TEST_CASE(a_glyph_is_read_once) {
    const unsigned int index = inter->glyphIndex(U'H');
    const std::uint32_t first = store.glyph(*inter, index).bands;
    store.glyph(*inter, inter->glyphIndex(U'o'));
    BOOST_CHECK_EQUAL(store.glyph(*inter, index).bands, first);
    BOOST_CHECK_EQUAL(store.read(), 2U);
}

/**
 * A glyph's bands hold its transform and its counts, and each band's list names the curves its
 * outline put in it, whose points are stored two texels a curve.
 **/
BOOST_AUTO_TEST_CASE(a_glyph_is_stored_as_the_shader_reads_it) {
    // a glyph first, so the next is stored after it
    store.glyph(*inter, inter->glyphIndex(U'o'));
    const unsigned int index = inter->glyphIndex(U'H');
    const v3d::font::CurveGlyph outlined = v3d::font::outline(*inter, index);
    const v3d::font::StoredCurves& stored = store.glyph(*inter, index);
    BOOST_REQUIRE(!stored.empty);
    const std::vector<float>& bands = store.bands();
    const std::vector<float>& curves = store.curves();
    const std::uint32_t base = stored.bands;
    const glm::vec4 transform = outlined.bandTransform();
    BOOST_CHECK_EQUAL(bands[base], transform.x);
    BOOST_CHECK_EQUAL(bands[base + 3], transform.w);
    BOOST_CHECK_EQUAL(bands[base + 4], static_cast<float>(outlined.vertical.size()));
    BOOST_CHECK_EQUAL(bands[base + 5], static_cast<float>(outlined.horizontal.size()));

    // the first horizontal band's list, and the first curve on it
    const auto count = static_cast<std::size_t>(bands[base + 8]);
    const auto list = static_cast<std::size_t>(bands[base + 9]);
    BOOST_REQUIRE_EQUAL(count, outlined.horizontal[0].size());
    const auto curve = static_cast<std::size_t>(bands[list]);
    const v3d::font::QuadraticCurve& expected = outlined.curves[outlined.horizontal[0][0]];
    BOOST_CHECK_EQUAL(curves[curve * 8], expected.p1.x);
    BOOST_CHECK_EQUAL(curves[(curve * 8) + 3], expected.p2.y);
    BOOST_CHECK_EQUAL(curves[(curve * 8) + 5], expected.p3.y);
    BOOST_CHECK(stored.min == outlined.min && stored.max == outlined.max);
}

/**
 * The rows written are given once, and a glyph with no outline is stored as empty, writing nothing.
 **/
BOOST_AUTO_TEST_CASE(changed_rows_are_given_once) {
    store.glyph(*inter, inter->glyphIndex(U'H'));
    BOOST_CHECK_EQUAL(store.curveRows(), 1U);
    const v3d::font::CurveStore::Changed changed = store.changedCurves();
    BOOST_CHECK_EQUAL(changed.first, 0U);
    BOOST_CHECK_EQUAL(changed.rows, 1U);
    BOOST_CHECK_EQUAL(store.changedCurves().rows, 0U);
    BOOST_CHECK_EQUAL(store.changedBands().rows, 1U);

    BOOST_CHECK(store.glyph(*inter, inter->glyphIndex(U' ')).empty);
    BOOST_CHECK_EQUAL(store.changedCurves().rows, 0U);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
