/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/font/glyph/Curves.h>
#include <api/font/face/Face.h>
#include <api/font/face/Library.h>
#include <api/log/Logger.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace {

/**
 * Inter, opened once for the suite's cases.
 **/
struct Outlines {
    Outlines() :
        logger(boost::make_shared<v3d::log::Logger>()),
        library(boost::make_shared<v3d::font::face::Library>()),
        inter(v3d::font::face::Face::open(library, "data/fonts/Inter-Regular.ttf", logger)) {
        BOOST_REQUIRE(inter);
    }

    /**
     * @return the outline of a character's glyph
     **/
    v3d::font::glyph::CurveGlyph of(char32_t charcode) const {
        return v3d::font::glyph::outline(*inter, inter->glyphIndex(charcode));
    }

    boost::shared_ptr<v3d::log::Logger> logger;
    boost::shared_ptr<v3d::font::face::Library> library;
    boost::shared_ptr<v3d::font::face::Face> inter;
};

/**
 * @return the winding number of an outline about a point: the curves a ray cast to the right
 *         crosses, each counted by the way it crosses
 **/
int winding(const v3d::font::glyph::CurveGlyph& glyph, const glm::vec2& point) {
    int count = 0;
    for (const v3d::font::glyph::QuadraticCurve& curve : glyph.curves) {
        // y(t) - point.y = a t^2 + b t + c
        const float square = curve.p1.y - (2.0F * curve.p2.y) + curve.p3.y;
        const float linear = 2.0F * (curve.p2.y - curve.p1.y);
        const float constant = curve.p1.y - point.y;
        std::vector<float> roots;
        if (std::abs(square) < 1e-7F) {
            if (std::abs(linear) > 1e-7F) {
                roots.push_back(-constant / linear);
            }
        } else {
            const float discriminant = (linear * linear) - (4.0F * square * constant);
            if (discriminant >= 0.0F) {
                roots.push_back((-linear - std::sqrt(discriminant)) / (2.0F * square));
                roots.push_back((-linear + std::sqrt(discriminant)) / (2.0F * square));
            }
        }
        for (const float along : roots) {
            const float rest = 1.0F - along;
            const float across = (rest * rest * curve.p1.x) + (2.0F * rest * along * curve.p2.x) + (along * along * curve.p3.x);
            const float slope = (2.0F * square * along) + linear;
            // half open, so a point where two curves meet is counted once
            if (along >= 0.0F && along < 1.0F && across > point.x && slope != 0.0F) {
                count += slope > 0.0F ? 1 : -1;
            }
        }
    }
    return count;
}

BOOST_FIXTURE_TEST_SUITE(curves_test, Outlines)

/**
 * The curves of an o close around its ring: a point in the stroke is inside, and the counter
 * and a point beyond the glyph are outside.
 **/
BOOST_AUTO_TEST_CASE(an_outline_encloses_its_strokes) {
    const v3d::font::glyph::CurveGlyph ring = of(U'o');
    BOOST_REQUIRE(!ring.curves.empty());
    const glm::vec2 centre = (ring.min + ring.max) / 2.0F;
    BOOST_CHECK_EQUAL(winding(ring, centre), 0);
    BOOST_CHECK_NE(winding(ring, glm::vec2(ring.min.x + 0.02F, centre.y)), 0);
    BOOST_CHECK_EQUAL(winding(ring, glm::vec2(ring.max.x + 0.1F, centre.y)), 0);
    // a glyph sits within an em or so of its origin
    BOOST_CHECK(ring.min.x >= -0.5F && ring.max.x <= 1.5F && ring.min.y >= -0.5F && ring.max.y <= 1.5F);
}

/**
 * Every curve ends where another starts, so each contour is closed.
 **/
BOOST_AUTO_TEST_CASE(contours_are_closed) {
    const v3d::font::glyph::CurveGlyph letter = of(U'a');
    BOOST_REQUIRE(!letter.curves.empty());
    for (const v3d::font::glyph::QuadraticCurve& curve : letter.curves) {
        bool joined = false;
        for (const v3d::font::glyph::QuadraticCurve& next : letter.curves) {
            joined = joined || next.p1 == curve.p3;
        }
        BOOST_CHECK(joined);
    }
}

/**
 * A horizontal band holds no horizontal line and lists its curves from the largest x down, and a
 * vertical band holds no vertical line and lists its curves from the largest y down. Every curve
 * that is not a line along both axes is in some band of each.
 **/
BOOST_AUTO_TEST_CASE(bands_hold_their_curves_in_order) {
    const v3d::font::glyph::CurveGlyph aitch = of(U'H');
    BOOST_REQUIRE(!aitch.horizontal.empty());
    BOOST_REQUIRE_EQUAL(aitch.horizontal.size(), aitch.vertical.size());
    const auto most = [&aitch](std::uint32_t index, int axis) {
        const v3d::font::glyph::QuadraticCurve& curve = aitch.curves[index];
        return std::max({ curve.p1[axis], curve.p2[axis], curve.p3[axis] });
    };
    for (const std::vector<std::uint32_t>& held : aitch.horizontal) {
        for (std::size_t at = 0; at < held.size(); at++) {
            const v3d::font::glyph::QuadraticCurve& curve = aitch.curves[held[at]];
            BOOST_CHECK(curve.p1.y != curve.p2.y || curve.p2.y != curve.p3.y);
            BOOST_CHECK(at == 0 || most(held[at - 1], 0) >= most(held[at], 0));
        }
    }
    for (const std::vector<std::uint32_t>& held : aitch.vertical) {
        for (std::size_t at = 0; at < held.size(); at++) {
            const v3d::font::glyph::QuadraticCurve& curve = aitch.curves[held[at]];
            BOOST_CHECK(curve.p1.x != curve.p2.x || curve.p2.x != curve.p3.x);
            BOOST_CHECK(at == 0 || most(held[at - 1], 1) >= most(held[at], 1));
        }
    }
}

/**
 * The band transform takes the outline's lower left to band 0 and its upper right to the
 * number of bands.
 **/
BOOST_AUTO_TEST_CASE(the_band_transform_spans_the_outline) {
    const v3d::font::glyph::CurveGlyph aitch = of(U'H');
    BOOST_REQUIRE(!aitch.horizontal.empty());
    const auto count = static_cast<float>(aitch.horizontal.size());
    const glm::vec4 transform = aitch.bandTransform();
    const glm::vec2 low = (aitch.min * glm::vec2(transform)) + glm::vec2(transform.z, transform.w);
    const glm::vec2 high = (aitch.max * glm::vec2(transform)) + glm::vec2(transform.z, transform.w);
    BOOST_CHECK_SMALL(low.x, 1e-4F);
    BOOST_CHECK_SMALL(low.y, 1e-4F);
    BOOST_CHECK_CLOSE(high.x, count, 1e-3F);
    BOOST_CHECK_CLOSE(high.y, count, 1e-3F);
}

/**
 * A space has no outline, and so no curves and no bands.
 **/
BOOST_AUTO_TEST_CASE(a_space_has_no_curves) {
    const v3d::font::glyph::CurveGlyph space = of(U' ');
    BOOST_CHECK(space.curves.empty());
    BOOST_CHECK(space.horizontal.empty());
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
