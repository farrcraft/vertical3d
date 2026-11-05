/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/animation/Curve.h>
#include <api/brep/BRep.h>
#include <api/log/Logger.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/ui/Immediate.h>
#include <api/ui/paint/Dressing.h>
#include <api/ui/tests/Typeset.h>
#include <vertical3d/src/control/Playback.h>
#include <vertical3d/src/control/TimelineEditor.h>
#include <vertical3d/src/document/Document.h>
#include <vertical3d/src/scene/item/Mesh.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/scene/Timeline.h>

#include <cmath>
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
        cube(boost::make_shared<v3d::editor::scene::item::Mesh>(boost::make_shared<v3d::brep::BRep>())),
        other(boost::make_shared<v3d::editor::scene::item::Mesh>(boost::make_shared<v3d::brep::BRep>())),
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
     * @return the middle of one of the transport's five buttons, which come before the frame's
     *         field, a bar square and spaced as the row spaces its widgets
     **/
    glm::vec2 button(int index) {
        const v3d::ui::paint::Dressing& dress = layer.dressing();
        const float step = dress.barHeight + dress.spacing;
        const float left = point("Frame").x - dress.padding - (5.0F * step);
        return glm::vec2(left + (static_cast<float>(index) * step) + (dress.barHeight * 0.5F), point("Frame").y + 2.0F);
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
    boost::shared_ptr<v3d::editor::scene::item::Mesh> cube;
    boost::shared_ptr<v3d::editor::scene::item::Mesh> other;
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
    panel.drag(panel.button(4), panel.button(4));
    BOOST_CHECK_EQUAL(panel.document->scene()->timeline().frame, v3d::editor::scene::Timeline::DEFAULT_END);
    panel.drag(panel.button(1), panel.button(1));
    BOOST_CHECK_EQUAL(panel.document->scene()->timeline().frame, 40);

    panel.drag(panel.on("Auto Key"), panel.on("Auto Key"));
    BOOST_CHECK(panel.document->scene()->timeline().autoKey);
    BOOST_CHECK_EQUAL(panel.document->history()->undoDepth(), 1U);
}

/**
 * The transport is five buttons, and play is stop while the scene plays. Each button's tooltip
 * names what it does, and the key bound to its command when there is one.
 **/
BOOST_AUTO_TEST_CASE(the_transport_names_what_each_button_does) {
    const std::vector<TimelineEditor::Control> still = TimelineEditor::controls(false);
    BOOST_REQUIRE_EQUAL(still.size(), 5U);
    BOOST_CHECK_EQUAL(std::string(still[0].tooltip), "First frame");
    BOOST_CHECK_EQUAL(std::string(still[2].tooltip), "Play");
    BOOST_CHECK_EQUAL(std::string(still[4].command), "timeline::playback::last");
    const std::vector<TimelineEditor::Control> playing = TimelineEditor::controls(true);
    BOOST_CHECK(playing[2].action == TimelineEditor::Action::Stop);
    BOOST_CHECK_EQUAL(std::string(playing[2].command), "timeline::playback::toggle");

    // a layer with no clock shows a tooltip as soon as the cursor is on its button
    Panel panel;
    panel.editor.shortcuts([](const std::string& command) { return command == "timeline::playback::toggle" ? std::string("Space") : std::string(); });
    panel.frame(Panel::hover(panel.button(2)));
    panel.frame(Panel::hover(panel.button(2)));
    BOOST_CHECK(panel.pen("Play"));
    BOOST_CHECK(panel.pen("Space"));
    panel.drag(panel.button(2), panel.button(2));
    BOOST_CHECK(panel.playback->playing());
    // the press put the tooltip away, and a move brings it back
    panel.frame(Panel::hover(panel.button(2) + glm::vec2(1.0F, 0.0F)));
    BOOST_CHECK(panel.pen("Stop"));
    panel.playback->stop();
}

/**
 * The rate is chosen from a menu that names each rate in frames a second, and the choice is a
 * change. A rate that is none of the common ones is offered among them.
 **/
BOOST_AUTO_TEST_CASE(the_rate_is_chosen_from_a_menu) {
    BOOST_CHECK_EQUAL(TimelineEditor::rateName(24.0F), "24 fps");
    BOOST_CHECK_EQUAL(TimelineEditor::rateName(29.97F), "29.97 fps");

    Panel panel;
    BOOST_CHECK(panel.pen("24 fps"));
    panel.drag(panel.on("Rate"), panel.on("Rate"));
    panel.frame(Panel::hover(panel.on("Rate")));
    const std::optional<glm::vec2> thirty = panel.pen("30 fps");
    BOOST_REQUIRE(thirty);
    const glm::vec2 choice = thirty.value_or(glm::vec2(0.0F)) + glm::vec2(2.0F, 2.0F);
    panel.drag(choice, choice);
    BOOST_CHECK_EQUAL(panel.document->scene()->timeline().rate, 30.0F);
    BOOST_REQUIRE(!panel.reported.empty());
    BOOST_CHECK(panel.reported.back().done);

    panel.document->timing(17.0F, 1, 120);
    panel.frame(Panel::hover(glm::vec2(690.0F, 490.0F)));
    BOOST_CHECK(panel.pen("17 fps"));
}

/**
 * The controls are one row in a wide area. In a narrow one, a control that would pass the right
 * edge starts a new row rather than running over the last, and the range's ends stay together.
 **/
BOOST_AUTO_TEST_CASE(the_controls_wrap_in_a_narrow_area) {
    Panel panel;
    const auto drawn = [&panel](float width) {
        panel.written.clear();
        panel.layer.begin(&panel.canvas, Panel::hover(glm::vec2(1290.0F, 490.0F)));
        panel.editor.draw(&panel.layer, 1, glm::vec4(0.0F, 0.0F, width, 400.0F));
        panel.layer.end();
    };
    panel.canvas.resize(glm::uvec2(1300, 500));
    // a check is a line high and a field a bar high, so their text sits a little apart on one row
    const float line = panel.layer.dressing().lineHeight;
    drawn(1200.0F);
    BOOST_CHECK_LT(std::abs(panel.point("Auto Key").y - panel.point("Frame").y), line);

    drawn(300.0F);
    BOOST_CHECK_GT(panel.point("Auto Key").y, panel.point("Frame").y + line);
    // the range's two ends stay on one row
    BOOST_CHECK_EQUAL(panel.point("Range").y, panel.point("to").y);
    BOOST_CHECK_LT(panel.point("Rate").x + panel.layer.measure("Rate"), 300.0F);
}

/**
 * A drag of the range's first frame sets it when it is let go, as one change.
 **/
BOOST_AUTO_TEST_CASE(the_range_is_set_when_its_drag_ends) {
    Panel panel;
    const int start = panel.document->scene()->timeline().start;
    panel.drag(panel.on("Range"), panel.on("Range") + glm::vec2(30.0F, 0.0F));
    BOOST_CHECK_GT(panel.document->scene()->timeline().start, start);
    BOOST_REQUIRE(!panel.reported.empty());
    BOOST_CHECK(panel.reported.back().done);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
