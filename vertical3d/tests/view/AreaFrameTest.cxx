/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/log/Logger.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <vertical3d/src/view/AreaFrame.h>
#include <vertical3d/src/view/AreaLayout.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <optional>

#include <boost/json/parse.hpp>
#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>
#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace {

using v3d::editor::AreaLayout;
using v3d::render::realtime::canvas::Canvas;

/**
 * Two areas side by side over a 200 by 100 window: the first from 3 to 98 across, the second
 * from 101 to 197, both from 3 to 97 down. The gap is drawn red, the hairline green and the
 * accent blue.
 **/
struct Pair {
    Pair() : layout(boost::make_shared<v3d::log::Logger>()) {
        BOOST_REQUIRE(layout.load(boost::json::parse(R"({"root": {"split": "horizontal", "children": [)"
            R"({"editors": [{"kind": "viewport"}]}, {"editors": [{"kind": "viewport"}]}]}})").as_object()));
        layout.resize(glm::vec4(0.0F, 0.0F, 200.0F, 100.0F));
        canvas.resize(glm::uvec2(200, 100));
    }

    void draw(std::optional<std::size_t> active = std::nullopt, const std::optional<AreaLayout::Divider>& lit = std::nullopt) {
        v3d::editor::drawFrame(&canvas, layout, { .gap = gap, .hairline = hairline, .accent = accent }, active, lit);
    }

    /**
     * @return whether a vertex of a colour meets a test
     **/
    bool any(const glm::vec4& colour, const std::function<bool(const glm::vec2&)>& test) const {
        return std::ranges::any_of(canvas.vertices(),
            [&colour, &test](const Canvas::Vertex& vertex) { return vertex.colour == colour && test(vertex.position); });
    }

    const glm::vec4 gap{ 1.0F, 0.0F, 0.0F, 1.0F };
    const glm::vec4 hairline{ 0.0F, 1.0F, 0.0F, 1.0F };
    const glm::vec4 accent{ 0.0F, 0.0F, 1.0F, 1.0F };
    AreaLayout layout;
    Canvas canvas;
};

bool at(const glm::vec2& point, float x, float y) {
    return point == glm::vec2(x, y);
}

BOOST_AUTO_TEST_SUITE(area_frame_test)

/**
 * The divider and the margin around the window are filled with the gap's colour.
 **/
BOOST_FIXTURE_TEST_CASE(the_gaps_are_filled, Pair) {
    draw();
    BOOST_CHECK(any(gap, [](const glm::vec2& point) { return at(point, 98.0F, 3.0F); }));
    BOOST_CHECK(any(gap, [](const glm::vec2& point) { return at(point, 101.0F, 97.0F); }));
    BOOST_CHECK(any(gap, [](const glm::vec2& point) { return at(point, 0.0F, 0.0F); }));
    BOOST_CHECK(any(gap, [](const glm::vec2& point) { return at(point, 200.0F, 100.0F); }));
}

/**
 * Each corner of an area is covered outside its rounding, from the rounding outwards.
 **/
BOOST_FIXTURE_TEST_CASE(the_corners_are_rounded, Pair) {
    draw();
    for (const glm::vec2 centre : { glm::vec2(7.0F, 7.0F), glm::vec2(94.0F, 93.0F), glm::vec2(193.0F, 7.0F) }) {
        BOOST_CHECK(any(gap, [centre](const glm::vec2& point) { return std::abs(glm::distance(point, centre) - 4.0F) < 0.01F; }));
    }
}

/**
 * The active area's line is the accent and the other's is a hairline.
 **/
BOOST_FIXTURE_TEST_CASE(the_active_area_is_outlined_in_the_accent, Pair) {
    draw(1);
    BOOST_CHECK(any(accent, [](const glm::vec2& point) { return point.x > 100.0F; }));
    BOOST_CHECK(!any(accent, [](const glm::vec2& point) { return point.x < 100.0F; }));
    BOOST_CHECK(any(hairline, [](const glm::vec2& point) { return point.x < 100.0F; }));
    BOOST_CHECK(!any(hairline, [](const glm::vec2& point) { return point.x > 100.0F; }));

    canvas.clear();
    draw();
    BOOST_CHECK(!any(accent, [](const glm::vec2&) { return true; }));
}

/**
 * A lit divider is filled with the accent.
 **/
BOOST_FIXTURE_TEST_CASE(a_lit_divider_takes_the_accent, Pair) {
    draw(std::nullopt, layout.divider(99.0F, 50.0F));
    BOOST_CHECK(any(accent, [](const glm::vec2& point) { return at(point, 98.0F, 3.0F); }));
    BOOST_CHECK(any(accent, [](const glm::vec2& point) { return at(point, 101.0F, 97.0F); }));
}

/**
 * A maximised area's line runs around the whole window, and the divider behind it is not drawn.
 **/
BOOST_FIXTURE_TEST_CASE(a_maximised_area_hides_the_divider, Pair) {
    layout.maximise(0);
    draw();
    BOOST_CHECK(!any(gap, [](const glm::vec2& point) { return at(point, 98.0F, 3.0F); }));
    BOOST_CHECK(any(hairline, [](const glm::vec2& point) { return point.x > 190.0F; }));
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
