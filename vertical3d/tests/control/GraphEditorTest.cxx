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
#include <vertical3d/src/control/GraphEditor.h>
#include <vertical3d/src/document/Document.h>
#include <vertical3d/src/scene/Mesh.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/view/ViewOverlay.h>

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

using v3d::editor::control::GraphEditor;

/**
 * What the editor wrote, and where.
 **/
using Written = v3d::ui::typeset::Written;

/**
 * A graph editor over a document holding a cube with translation.x keyed 0 on frame 20 and 1 on
 * frame 40, drawn into a 600 by 400 region at the canvas's corner. The layer measures ten pixels
 * a character and records what it writes.
 **/
struct Panel {
    Panel() :
        document(boost::make_shared<v3d::editor::document::Document>(boost::make_shared<v3d::log::Logger>(),
            boost::filesystem::temp_directory_path() / "v3d_graph_editor")),
        cube(boost::make_shared<v3d::editor::scene::Mesh>(boost::make_shared<v3d::brep::BRep>())),
        layer(v3d::ui::typeset::mono(&written)),
        editor(document, [this](const v3d::editor::document::Outcome& outcome) { reported.push_back(outcome); }) {
        cube->name("Cube");
        document->scene()->add(cube);
        v3d::animation::Curve curve;
        curve.set(20.0F, 0.0F);
        curve.set(40.0F, 1.0F);
        document->scene()->curve(cube->id(), "translation.x", curve);
        cube->selected(true);
        canvas.resize(glm::uvec2(700, 500));
        frame(hover(away));
    }

    /**
     * Draw one frame of the editor under some input.
     **/
    void frame(const v3d::ui::Immediate::Input& input, GraphEditor::Navigate navigate = GraphEditor::Navigate::None) {
        written.clear();
        layer.begin(&canvas, input);
        editor.draw(&layer, 1, glm::vec4(0.0F, 0.0F, 600.0F, 400.0F), navigate);
        layer.end();
    }

    static v3d::ui::Immediate::Input hover(const glm::vec2& cursor) {
        v3d::ui::Immediate::Input made;
        made.cursor = cursor;
        return made;
    }

    /**
     * Press at a point, drag to another, and let go there.
     **/
    void drag(const glm::vec2& from, const glm::vec2& to, GraphEditor::Navigate navigate = GraphEditor::Navigate::None) {
        frame(hover(from), navigate);
        v3d::ui::Immediate::Input input = hover(from);
        input.down = true;
        input.pressed = true;
        frame(input, navigate);
        input = hover(to);
        input.down = true;
        frame(input, navigate);
        input = hover(to);
        input.released = true;
        frame(input, navigate);
        frame(hover(to), navigate);
    }

    /**
     * @return the view the editor last drew
     **/
    GraphEditor::View view() const {
        const std::optional<GraphEditor::View> drawn = editor.view(1);
        BOOST_REQUIRE(drawn);
        return drawn.value_or(GraphEditor::View());
    }

    /**
     * @return where a frame, in x, and a value, in y, are drawn
     **/
    glm::vec2 at(const glm::vec2& place) const {
        return view().at(place);
    }

    /**
     * @return a point on the ruler at a frame, found from a frame number written above the graph
     **/
    glm::vec2 ruler(float frame) const {
        for (const Written& each : written) {
            if (each.pen.y < view().min.y && each.text != "Frame All") {
                return glm::vec2(at(glm::vec2(frame, 0.0F)).x, each.pen.y + 2.0F);
            }
        }
        BOOST_FAIL("no frame is numbered on the ruler");
        return glm::vec2(0.0F);
    }

    /**
     * @return a point inside the Frame All button
     **/
    glm::vec2 frameAll() const {
        for (const Written& each : written) {
            if (each.text == "Frame All") {
                return each.pen + glm::vec2(2.0F, 2.0F);
            }
        }
        BOOST_FAIL("Frame All is not drawn");
        return glm::vec2(0.0F);
    }

    v3d::animation::Curve curve() const {
        return document->scene()->curves(cube->id()).at("translation.x");
    }

    static constexpr glm::vec2 away{ 690.0F, 490.0F };

    boost::shared_ptr<v3d::editor::document::Document> document;
    boost::shared_ptr<v3d::editor::scene::Mesh> cube;
    v3d::render::realtime::canvas::Canvas canvas;
    std::vector<Written> written;
    std::vector<v3d::editor::document::Outcome> reported;
    v3d::ui::Immediate layer;
    GraphEditor editor;
};

BOOST_AUTO_TEST_SUITE(graph_editor_test)

/**
 * A view puts its first frame at the left and its least value at the bottom, and a point gives
 * the frame and the value back.
 **/
BOOST_AUTO_TEST_CASE(a_view_lays_out_frames_and_values) {
    const GraphEditor::View view{ .min = glm::vec2(100.0F, 50.0F), .max = glm::vec2(300.0F, 150.0F), .frame = 10.0F, .value = -1.0F,
        .frames = 20.0F, .values = 2.0F };
    BOOST_CHECK(view.at(glm::vec2(10.0F, -1.0F)) == glm::vec2(100.0F, 150.0F));
    BOOST_CHECK(view.at(glm::vec2(30.0F, 1.0F)) == glm::vec2(300.0F, 50.0F));
    const glm::vec2 back = view.point(view.at(glm::vec2(15.0F, 0.5F)));
    BOOST_CHECK_CLOSE(back.x, 15.0F, 0.001F);
    BOOST_CHECK_CLOSE(back.y, 0.5F, 0.001F);
}

/**
 * A curve takes its axis's colour, and a path with no axis the colour given.
 **/
BOOST_AUTO_TEST_CASE(a_curve_is_coloured_by_its_axis) {
    const glm::vec4 text(1.0F);
    BOOST_CHECK(GraphEditor::colour("translation.x", text) == v3d::editor::view::OverlayLook::X_AXIS);
    BOOST_CHECK(GraphEditor::colour("rotation.y", text) == v3d::editor::view::OverlayLook::Y_AXIS);
    BOOST_CHECK(GraphEditor::colour("scale.z", text) == v3d::editor::view::OverlayLook::Z_AXIS);
    BOOST_CHECK(GraphEditor::colour("lens.fov", text) == text);
}

/**
 * A new editor fits its view to the selected items' curves, and Frame All fits it again after
 * the view has moved.
 **/
BOOST_AUTO_TEST_CASE(the_view_fits_the_curves) {
    Panel panel;
    const GraphEditor::View fitted = panel.view();
    BOOST_CHECK(fitted.frame < 20.0F);
    BOOST_CHECK(fitted.frame + fitted.frames > 40.0F);
    BOOST_CHECK(fitted.value < 0.0F);
    BOOST_CHECK(fitted.value + fitted.values > 1.0F);

    const glm::vec2 middle = (fitted.min + fitted.max) * 0.5F;
    panel.drag(middle, middle + glm::vec2(100.0F, 0.0F), GraphEditor::Navigate::Pan);
    BOOST_CHECK(panel.view().frame < fitted.frame - 1.0F);

    panel.drag(panel.frameAll(), panel.frameAll());
    BOOST_CHECK_CLOSE(panel.view().frame, fitted.frame, 0.001F);
}

/**
 * The truck move drags the view, the zoom move shows fewer frames for a drag right and fewer
 * values for a drag up, and the wheel shows less about the cursor.
 **/
BOOST_AUTO_TEST_CASE(the_view_pans_and_zooms) {
    Panel panel;
    const GraphEditor::View before = panel.view();
    const glm::vec2 middle = (before.min + before.max) * 0.5F;

    // a drag left by a quarter of the width moves the view on by a quarter of its frames
    panel.drag(middle, middle - glm::vec2((before.max.x - before.min.x) * 0.25F, 0.0F), GraphEditor::Navigate::Pan);
    BOOST_CHECK_CLOSE(panel.view().frame, before.frame + (before.frames * 0.25F), 0.1F);

    panel.drag(middle, middle + glm::vec2(50.0F, -50.0F), GraphEditor::Navigate::Zoom);
    BOOST_CHECK(panel.view().frames < before.frames);
    BOOST_CHECK(panel.view().values < before.values);
    BOOST_CHECK_EQUAL(panel.document->history()->undoDepth(), 0U);

    // the point under the cursor stays under it
    const GraphEditor::View zoomed = panel.view();
    const glm::vec2 under = zoomed.point(middle);
    v3d::ui::Immediate::Input turned = Panel::hover(middle);
    turned.wheel = 1.0F;
    panel.frame(turned);
    BOOST_CHECK(panel.view().frames < zoomed.frames);
    const glm::vec2 still = panel.view().point(middle);
    BOOST_CHECK_CLOSE(still.x, under.x, 0.01F);
    BOOST_CHECK_CLOSE(still.y, under.y, 0.01F);
}

/**
 * A press on the ruler moves the frame to where it lands.
 **/
BOOST_AUTO_TEST_CASE(the_ruler_scrubs) {
    Panel panel;
    panel.drag(panel.ruler(25.0F), panel.ruler(30.0F));
    BOOST_CHECK_EQUAL(panel.document->scene()->timeline().frame, 30);
    BOOST_CHECK_EQUAL(panel.document->history()->undoDepth(), 0U);
}

/**
 * A press on a key chooses it, and a drag moves it by whole frames and any value, as one
 * change. A press away from every key chooses none.
 **/
BOOST_AUTO_TEST_CASE(a_key_is_chosen_and_moved) {
    Panel panel;
    panel.drag(panel.at(glm::vec2(40.0F, 1.0F)), panel.at(glm::vec2(35.2F, 0.5F)));
    BOOST_REQUIRE(panel.editor.chosen());
    const GraphEditor::CurveKey chosen = panel.editor.chosen().value_or(GraphEditor::CurveKey());
    BOOST_CHECK_EQUAL(chosen.path, "translation.x");
    BOOST_CHECK_EQUAL(chosen.frame, 35.0F);
    const v3d::animation::Curve moved = panel.curve();
    BOOST_REQUIRE_EQUAL(moved.keys().size(), 2U);
    BOOST_CHECK_EQUAL(moved.keys()[1].frame, 35.0F);
    BOOST_CHECK_CLOSE(moved.keys()[1].value, 0.5F, 0.5F);
    BOOST_CHECK_EQUAL(panel.document->history()->undoDepth(), 1U);
    BOOST_REQUIRE(!panel.reported.empty());
    BOOST_CHECK_EQUAL(panel.reported.back().message, "edited translation.x");

    panel.document->undo();
    BOOST_CHECK_EQUAL(panel.curve().keys()[1].frame, 40.0F);

    panel.drag(panel.at(glm::vec2(30.0F, 0.9F)), panel.at(glm::vec2(30.0F, 0.9F)));
    BOOST_CHECK(!panel.editor.chosen());
}

/**
 * A drag on the chosen key's handle turns both its slopes toward the handle, and it stops being
 * automatic.
 **/
BOOST_AUTO_TEST_CASE(a_handle_turns_the_slopes) {
    Panel panel;
    panel.drag(panel.at(glm::vec2(20.0F, 0.0F)), panel.at(glm::vec2(20.0F, 0.0F)));
    BOOST_REQUIRE(panel.editor.chosen());
    BOOST_CHECK_EQUAL(panel.document->history()->undoDepth(), 0U);

    // the first key's handle reaches a third of the way to the key after it, along its flat slope
    const float reach = 20.0F / 3.0F;
    panel.drag(panel.at(glm::vec2(20.0F + reach, 0.0F)), panel.at(glm::vec2(20.0F + reach, 0.5F)));
    const v3d::animation::Curve::Key key = panel.curve().keys()[0];
    BOOST_CHECK(!key.automatic);
    BOOST_CHECK_CLOSE(key.out, 0.5F / reach, 1.0F);
    BOOST_CHECK_EQUAL(key.in, key.out);
    BOOST_CHECK_EQUAL(panel.document->history()->undoDepth(), 1U);
}

/**
 * A key whose item stops being selected stops being chosen.
 **/
BOOST_AUTO_TEST_CASE(a_choice_follows_the_selection) {
    Panel panel;
    panel.drag(panel.at(glm::vec2(40.0F, 1.0F)), panel.at(glm::vec2(40.0F, 1.0F)));
    BOOST_REQUIRE(panel.editor.chosen());
    panel.cube->selected(false);
    panel.frame(Panel::hover(Panel::away));
    BOOST_CHECK(!panel.editor.chosen());
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
