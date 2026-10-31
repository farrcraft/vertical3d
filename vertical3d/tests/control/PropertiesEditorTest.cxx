/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/brep/Primitives.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/ui/Immediate.h>
#include <vertical3d/src/command/CommandStack.h>
#include <vertical3d/src/command/EditCommand.h>
#include <vertical3d/src/control/PropertiesEditor.h>
#include <vertical3d/src/scene/Camera.h>
#include <vertical3d/src/scene/Mesh.h>
#include <vertical3d/src/scene/Scene.h>

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace {

using v3d::editor::EditCommand;
using v3d::editor::PropertiesEditor;

/**
 * What the editor wrote, and where.
 **/
struct Written final {
    std::string text;
    glm::vec2 pen;
};

/**
 * A properties editor over a scene of a cube and a camera, drawn into a 300 by 600 region at
 * the canvas's corner. The layer measures ten pixels a character and records what it writes.
 **/
struct Panel {
    Panel() :
        history(boost::make_shared<v3d::editor::CommandStack>()),
        cube(boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube())),
        camera(boost::make_shared<v3d::editor::Camera>()),
        layer([](std::string_view text) { return static_cast<float>(text.size()) * 10.0F; },
            [this](std::string_view text, const glm::vec2& pen, const glm::vec4&) { written.push_back({ std::string(text), pen }); }),
        editor(history) {
        cube->name("Cube");
        scene.add(cube);
        scene.add(camera);
        canvas.resize(glm::uvec2(400, 700));
    }

    /**
     * Draw one frame of the editor under some input.
     **/
    void frame(const v3d::ui::Immediate::Input& input) {
        written.clear();
        layer.begin(&canvas, input);
        editor.draw(&layer, &scene, 1, glm::vec4(0.0F, 0.0F, 300.0F, 600.0F));
        layer.end();
    }

    /**
     * @return a point inside the field whose label was written, the first of that label
     **/
    std::optional<glm::vec2> at(const std::string& label) const {
        for (const Written& each : written) {
            if (each.text == label) {
                return each.pen + glm::vec2(2.0F, -4.0F);
            }
        }
        return std::nullopt;
    }

    bool wrote(const std::string& text) const {
        return at(text).has_value();
    }

    static v3d::ui::Immediate::Input input(const glm::vec2& cursor, bool down, bool pressed, bool released) {
        v3d::ui::Immediate::Input made;
        made.cursor = cursor;
        made.down = down;
        made.pressed = pressed;
        made.released = released;
        return made;
    }

    /**
     * Drag across a number from a point, by a distance in two moves: one within the distance
     * that starts a drag, and the rest.
     **/
    void drag(const glm::vec2& from, float across) {
        frame(input(from, false, false, false));
        frame(input(from, true, true, false));
        frame(input(from + glm::vec2(2.0F, 0.0F), true, false, false));
        frame(input(from + glm::vec2(2.0F + across, 0.0F), true, false, false));
        frame(input(from + glm::vec2(2.0F + across, 0.0F), false, false, true));
    }

    boost::shared_ptr<v3d::editor::CommandStack> history;
    v3d::editor::Scene scene;
    boost::shared_ptr<v3d::editor::Mesh> cube;
    boost::shared_ptr<v3d::editor::Camera> camera;
    v3d::render::realtime::canvas::Canvas canvas;
    std::vector<Written> written;
    v3d::ui::Immediate layer;
    PropertiesEditor editor;
};

BOOST_AUTO_TEST_SUITE(properties_editor_test)

/**
 * Each path is shown under its heading, and an angle moves half a degree a pixel where a
 * distance moves a hundredth of a unit.
 **/
BOOST_AUTO_TEST_CASE(a_path_is_shown_under_its_heading) {
    const PropertiesEditor::Field turn = PropertiesEditor::field("rotation.y");
    BOOST_CHECK_EQUAL(turn.group, "Rotation");
    BOOST_CHECK_EQUAL(turn.label, "Y");
    BOOST_CHECK_EQUAL(turn.speed, 0.5F);
    BOOST_CHECK_EQUAL(PropertiesEditor::field("translation.x").speed, 0.01F);
    BOOST_CHECK_EQUAL(PropertiesEditor::field("lens.fov").label, "Field of View");
    BOOST_CHECK_EQUAL(PropertiesEditor::field("unknown").label, "unknown");
}

/**
 * An edit command writes either end of an edit back whole, and two sets of values that differ
 * only in the last bits of a placement are the same.
 **/
BOOST_AUTO_TEST_CASE(an_edit_command_puts_either_end_back) {
    const boost::shared_ptr<v3d::editor::Camera> camera = boost::make_shared<v3d::editor::Camera>();
    const EditCommand::Values before = EditCommand::of(*camera);
    EditCommand::Values after = before;
    after.name = "Shot";
    after.pose.placement.position = glm::vec3(1.0F, 2.0F, 3.0F);
    after.pose.lens.fov = 30.0F;
    EditCommand::apply(camera.get(), after);
    BOOST_CHECK(EditCommand::same(EditCommand::of(*camera), after));
    BOOST_CHECK(!EditCommand::same(before, after));

    // the command reads where the edit ended from the item
    EditCommand command(camera, before, "edit lens");
    command.undo();
    BOOST_CHECK(camera->name().empty());
    BOOST_CHECK_EQUAL(camera->lens().fov, before.pose.lens.fov);
    BOOST_CHECK(camera->translation() == glm::vec3(0.0F));
    command.redo();
    BOOST_CHECK_EQUAL(camera->name(), "Shot");
    BOOST_CHECK_EQUAL(command.name(), "edit lens");

    EditCommand::Values nudged = before;
    nudged.pose.placement.position.x += 1e-8F;
    BOOST_CHECK(EditCommand::same(before, nudged));
}

/**
 * With nothing selected, or more than one item, the editor says so and shows no item.
 **/
BOOST_FIXTURE_TEST_CASE(it_shows_one_selected_item, Panel) {
    frame({});
    BOOST_CHECK(wrote("Nothing is selected"));
    cube->selected(true);
    camera->selected(true);
    frame({});
    BOOST_CHECK(wrote("2 items are selected"));
    BOOST_CHECK(!wrote("Translation"));

    camera->selected(false);
    frame({});
    BOOST_CHECK(wrote("Mesh"));
    BOOST_CHECK(wrote("Translation"));
    BOOST_CHECK(wrote("Rotation"));
    BOOST_CHECK(wrote("Scale"));
    BOOST_CHECK(!wrote("Lens"));
}

/**
 * A camera shows its lens, and a drag of its field of view moves it.
 **/
BOOST_FIXTURE_TEST_CASE(a_camera_shows_its_lens, Panel) {
    camera->selected(true);
    frame({});
    BOOST_CHECK(wrote("Lens"));
    const std::optional<glm::vec2> fov = at("Field of View");
    BOOST_REQUIRE(fov);
    const float was = camera->lens().fov;
    drag(*fov, 20.0F);
    BOOST_CHECK_CLOSE(camera->lens().fov, was + 2.0F, 0.01F);
}

/**
 * A drag of a number moves the item as it goes, and is one change in the history when it is let
 * go, which undo takes back whole.
 **/
BOOST_FIXTURE_TEST_CASE(a_drag_is_one_change, Panel) {
    cube->selected(true);
    frame({});
    // the first X is the translation's
    const std::optional<glm::vec2> x = at("X");
    BOOST_REQUIRE(x);
    drag(*x, 50.0F);
    BOOST_CHECK_CLOSE(cube->translation().x, 0.5F, 0.01F);
    BOOST_CHECK_EQUAL(history->undoDepth(), 1U);

    history->undo()->undo();
    BOOST_CHECK(cube->translation() == glm::vec3(0.0F));
}

/**
 * A value that would make a lens invalid is not written.
 **/
BOOST_FIXTURE_TEST_CASE(an_invalid_lens_is_not_written, Panel) {
    camera->selected(true);
    frame({});
    const std::optional<glm::vec2> near = at("Near");
    BOOST_REQUIRE(near);
    const float was = camera->lens().clipping.x;
    drag(*near, -1000.0F);
    BOOST_CHECK_EQUAL(camera->lens().clipping.x, was);
    BOOST_CHECK_EQUAL(history->undoDepth(), 0U);
}

/**
 * Typing a name renames the item as it is typed, and is one change when the field lets go.
 **/
BOOST_FIXTURE_TEST_CASE(a_rename_is_one_change, Panel) {
    cube->selected(true);
    frame({});
    // a press past the end of the name puts the caret there
    const std::optional<glm::vec2> start = at("Cube");
    BOOST_REQUIRE(start);
    const std::optional<glm::vec2> name = *start + glm::vec2(60.0F, 0.0F);
    frame(input(*name, false, false, false));
    frame(input(*name, true, true, false));
    frame(input(*name, false, false, true));
    BOOST_REQUIRE(layer.typing());
    BOOST_CHECK(layer.typed("s"));
    frame(input(*name, false, false, false));
    BOOST_CHECK_EQUAL(cube->name(), "Cubes");
    BOOST_CHECK_EQUAL(history->undoDepth(), 0U);
    BOOST_CHECK(layer.press("return"));
    frame(input(*name, false, false, false));
    BOOST_REQUIRE_EQUAL(history->undoDepth(), 1U);

    const boost::shared_ptr<v3d::editor::Command> undone = history->undo();
    BOOST_CHECK_EQUAL(undone->name(), "rename");
    undone->undo();
    BOOST_CHECK_EQUAL(cube->name(), "Cube");
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
