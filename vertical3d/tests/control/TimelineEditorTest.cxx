/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/animation/Curve.h>
#include <api/brep/BRep.h>
#include <api/log/Logger.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/ui/Immediate.h>
#include <api/ui/tests/Typeset.h>
#include <vertical3d/src/control/Playback.h>
#include <vertical3d/src/control/TimelineEditor.h>
#include <vertical3d/src/document/Document.h>
#include <vertical3d/src/scene/Mesh.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/scene/Timeline.h>

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <boost/filesystem/operations.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace {

using v3d::editor::control::TimelineEditor;

/**
 * What the editor wrote, and where.
 **/
using Written = v3d::ui::typeset::Written;

/**
 * A timeline editor over a document holding a cube keyed on frames 20 and 40 and an empty mesh,
 * drawn into a 600 by 400 region at the canvas's corner. The layer measures ten pixels a
 * character and records what it writes.
 **/
struct Panel {
    Panel() :
        document(boost::make_shared<v3d::editor::document::Document>(boost::make_shared<v3d::log::Logger>(),
            boost::filesystem::temp_directory_path() / "v3d_timeline_editor")),
        playback(boost::make_shared<v3d::editor::control::Playback>(document)),
        cube(boost::make_shared<v3d::editor::scene::Mesh>(boost::make_shared<v3d::brep::BRep>())),
        other(boost::make_shared<v3d::editor::scene::Mesh>(boost::make_shared<v3d::brep::BRep>())),
        layer(v3d::ui::typeset::mono(&written)),
        editor(document, playback, [this](const v3d::editor::document::Outcome& outcome) { reported.push_back(outcome); }) {
        cube->name("Cube");
        other->name("Other");
        document->scene()->add(cube);
        document->scene()->add(other);
        v3d::animation::Curve curve;
        curve.set(20.0F, 0.0F);
        curve.set(40.0F, 1.0F);
        document->scene()->curve(cube->id(), "translation.x", curve);
        cube->selected(true);
        canvas.resize(glm::uvec2(700, 500));
        frame(hover(glm::vec2(690.0F, 490.0F)));
    }

    /**
     * Draw one frame of the editor under some input.
     **/
    void frame(const v3d::ui::Immediate::Input& input) {
        written.clear();
        layer.begin(&canvas, input);
        editor.draw(&layer, 1, glm::vec4(0.0F, 0.0F, 600.0F, 400.0F));
        layer.end();
    }

    /**
     * @return where the first text written as this was put
     **/
    std::optional<glm::vec2> pen(const std::string& text) const {
        for (const Written& each : written) {
            if (each.text == text) {
                return each.pen;
            }
        }
        return std::nullopt;
    }

    /**
     * @return where text that has to have been written was put
     **/
    glm::vec2 point(const std::string& text) const {
        const std::optional<glm::vec2> found = pen(text);
        BOOST_REQUIRE(found);
        return found.value_or(glm::vec2(-1.0F));
    }

    /**
     * @return a point just inside the control whose label was written
     **/
    glm::vec2 on(const std::string& label) const {
        return point(label) + glm::vec2(2.0F, 2.0F);
    }

    /**
     * @return the point along a row's middle at a frame, found from the ruler's numbers for 20
     *         and 40, which are written two pixels past their frames
     **/
    glm::vec2 at(const std::string& row, float frame) const {
        const glm::vec2 twenty = point("20") - glm::vec2(2.0F, 0.0F);
        const glm::vec2 forty = point("40") - glm::vec2(2.0F, 0.0F);
        const float x = twenty.x + ((frame - 20.0F) / 20.0F * (forty.x - twenty.x));
        const float y = row.empty() ? twenty.y : point(row).y;
        return glm::vec2(x, y + 2.0F);
    }

    static v3d::ui::Immediate::Input hover(const glm::vec2& cursor) {
        v3d::ui::Immediate::Input made;
        made.cursor = cursor;
        return made;
    }

    /**
     * Press at a point, drag to another, and let go there.
     **/
    void drag(const glm::vec2& from, const glm::vec2& to) {
        frame(hover(from));
        v3d::ui::Immediate::Input input = hover(from);
        input.down = true;
        input.pressed = true;
        frame(input);
        input = hover(to);
        input.down = true;
        frame(input);
        input = hover(to);
        input.released = true;
        frame(input);
        frame(hover(to));
    }

    std::vector<float> keyed() const {
        return document->scene()->keyedFrames({ cube });
    }

    boost::shared_ptr<v3d::editor::document::Document> document;
    boost::shared_ptr<v3d::editor::control::Playback> playback;
    boost::shared_ptr<v3d::editor::scene::Mesh> cube;
    boost::shared_ptr<v3d::editor::scene::Mesh> other;
    v3d::render::realtime::canvas::Canvas canvas;
    std::vector<Written> written;
    std::vector<v3d::editor::document::Outcome> reported;
    v3d::ui::Immediate layer;
    TimelineEditor editor;
};

BOOST_AUTO_TEST_SUITE(timeline_editor_test)

/**
 * A frame is as far along a track as its place in the range, with half a frame's room at each
 * end, and a point along the track gives the frame back.
 **/
BOOST_AUTO_TEST_CASE(a_span_lays_out_the_range) {
    const TimelineEditor::Span span{ .left = 100.0F, .right = 300.0F, .start = 1, .end = 10 };
    BOOST_CHECK_CLOSE(span.at(1.0F), 110.0F, 0.001F);
    BOOST_CHECK_CLOSE(span.at(10.0F), 290.0F, 0.001F);
    BOOST_CHECK_CLOSE(span.frame(span.at(4.0F)), 4.0F, 0.001F);
}

/**
 * Each selected item has a row, and an item that is not selected has none.
 **/
BOOST_AUTO_TEST_CASE(a_row_is_drawn_for_each_selected_item) {
    Panel panel;
    BOOST_CHECK(panel.pen("Cube"));
    BOOST_CHECK(!panel.pen("Other"));
    BOOST_CHECK(panel.pen("20") && panel.pen("40"));

    panel.other->selected(true);
    panel.frame(Panel::hover(glm::vec2(690.0F, 490.0F)));
    BOOST_CHECK(panel.pen("Other"));
}

/**
 * A press on the ruler moves the frame to where it lands, and a drag scrubs.
 **/
BOOST_AUTO_TEST_CASE(the_ruler_scrubs) {
    Panel panel;
    panel.drag(panel.at("", 30.0F), panel.at("", 60.0F));
    BOOST_CHECK_EQUAL(panel.document->scene()->timeline().frame, 60);
    BOOST_CHECK_EQUAL(panel.document->history()->undoDepth(), 0U);
}

/**
 * A press on a key chooses it, and a drag moves it along by whole frames when it is let go, as
 * one change. A press on a row away from its keys chooses none and moves the frame.
 **/
BOOST_AUTO_TEST_CASE(a_key_is_chosen_and_moved) {
    Panel panel;
    panel.drag(panel.at("Cube", 20.0F), panel.at("Cube", 30.0F));
    BOOST_CHECK(panel.keyed() == std::vector<float>({ 30.0F, 40.0F }));
    BOOST_CHECK_EQUAL(panel.document->history()->undoDepth(), 1U);
    BOOST_REQUIRE_EQUAL(panel.editor.chosen().size(), 1U);
    BOOST_CHECK_EQUAL(panel.editor.chosen()[0].frame, 30.0F);
    BOOST_REQUIRE(!panel.reported.empty());
    BOOST_CHECK_EQUAL(panel.reported.back().message, "moved 1 key 10 frames later");

    panel.drag(panel.at("Cube", 80.0F), panel.at("Cube", 80.0F));
    BOOST_CHECK(panel.editor.chosen().empty());
    BOOST_CHECK_EQUAL(panel.document->scene()->timeline().frame, 80);
    BOOST_CHECK_EQUAL(panel.document->history()->undoDepth(), 1U);
}

/**
 * The chosen keys are deleted as one change, and none are chosen after.
 **/
BOOST_AUTO_TEST_CASE(the_chosen_keys_are_deleted) {
    Panel panel;
    panel.drag(panel.at("Cube", 40.0F), panel.at("Cube", 40.0F));
    BOOST_REQUIRE_EQUAL(panel.editor.chosen().size(), 1U);
    BOOST_CHECK_EQUAL(panel.document->history()->undoDepth(), 0U);

    panel.editor.deleteChosen();
    BOOST_CHECK(panel.keyed() == std::vector<float>({ 20.0F }));
    BOOST_CHECK(panel.editor.chosen().empty());
    BOOST_CHECK_EQUAL(panel.reported.back().message, "deleted 1 key");

    // with none chosen there is nothing to delete
    panel.editor.deleteChosen();
    BOOST_CHECK(!panel.reported.back().done);
}

/**
 * A key whose item stops being selected stops being chosen.
 **/
BOOST_AUTO_TEST_CASE(a_choice_follows_the_selection) {
    Panel panel;
    panel.drag(panel.at("Cube", 40.0F), panel.at("Cube", 40.0F));
    BOOST_REQUIRE_EQUAL(panel.editor.chosen().size(), 1U);
    panel.cube->selected(false);
    panel.frame(Panel::hover(glm::vec2(690.0F, 490.0F)));
    BOOST_CHECK(panel.editor.chosen().empty());
}

/**
 * The transport goes to the ends of the range and steps between keys, and the check turns auto
 * key on as a change.
 **/
BOOST_AUTO_TEST_CASE(the_controls_step_and_set) {
    Panel panel;
    panel.drag(panel.on("Last"), panel.on("Last"));
    BOOST_CHECK_EQUAL(panel.document->scene()->timeline().frame, v3d::editor::scene::Timeline::DEFAULT_END);
    panel.drag(panel.on("Previous Key"), panel.on("Previous Key"));
    BOOST_CHECK_EQUAL(panel.document->scene()->timeline().frame, 40);

    panel.drag(panel.on("Auto Key"), panel.on("Auto Key"));
    BOOST_CHECK(panel.document->scene()->timeline().autoKey);
    BOOST_CHECK_EQUAL(panel.document->history()->undoDepth(), 1U);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
