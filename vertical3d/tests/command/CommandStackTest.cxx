/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/brep/BRep.h>
#include <vertical3d/src/command/Command.h>
#include <vertical3d/src/command/CommandStack.h>
#include <vertical3d/src/command/CreateCommand.h>
#include <vertical3d/src/command/Placement.h>
#include <vertical3d/src/command/TransformCommand.h>
#include <vertical3d/src/scene/CreatePoly.h>
#include <vertical3d/src/scene/Scene.h>

#include <string>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/vec3.hpp>

namespace {

/**
 * A command that counts how often it has been done and undone. The tests measure the
 * stack's behaviour by these counts.
 **/
class CountingCommand final : public v3d::editor::Command {
 public:
    explicit CountingCommand(const std::string& name) :
        name_(name),
        undone(0),
        redone(0) {
    }

    void undo() override {
        undone++;
    }

    void redo() override {
        redone++;
    }

    std::string name() const override {
        return name_;
    }

    std::string name_;
    int undone;
    int redone;
};

/**
 **/
boost::shared_ptr<CountingCommand> counter(const std::string& name) {
    return boost::make_shared<CountingCommand>(name);
}

BOOST_AUTO_TEST_CASE(command_stack_empty_test) {
    v3d::editor::CommandStack stack;

    BOOST_CHECK(!stack.canUndo());
    BOOST_CHECK(!stack.canRedo());
    BOOST_CHECK(!stack.undo());
    BOOST_CHECK(!stack.redo());

    // a stack that is given nothing has recorded nothing
    stack.push(boost::shared_ptr<v3d::editor::Command>());
    BOOST_CHECK_EQUAL(stack.undoDepth(), 0U);
}

BOOST_AUTO_TEST_CASE(command_stack_undo_redo_test) {
    v3d::editor::CommandStack stack;
    const boost::shared_ptr<CountingCommand> first = counter("first");
    const boost::shared_ptr<CountingCommand> second = counter("second");

    // a command arrives already done, so pushing must not do it again
    stack.push(first);
    stack.push(second);
    BOOST_CHECK_EQUAL(first->redone, 0);
    BOOST_CHECK_EQUAL(second->redone, 0);
    BOOST_CHECK_EQUAL(stack.undoDepth(), 2U);

    const boost::shared_ptr<v3d::editor::Command> undone = stack.undo();
    BOOST_CHECK(undone == second);
    BOOST_CHECK_EQUAL(second->undone, 1);
    BOOST_CHECK_EQUAL(first->undone, 0);
    BOOST_CHECK_EQUAL(stack.undoDepth(), 1U);
    BOOST_CHECK_EQUAL(stack.redoDepth(), 1U);

    const boost::shared_ptr<v3d::editor::Command> redone = stack.redo();
    BOOST_CHECK(redone == second);
    BOOST_CHECK_EQUAL(second->redone, 1);
    BOOST_CHECK_EQUAL(stack.undoDepth(), 2U);
    BOOST_CHECK(!stack.canRedo());

    // the whole history unwinds in order
    BOOST_CHECK(stack.undo() == second);
    BOOST_CHECK(stack.undo() == first);
    BOOST_CHECK(!stack.canUndo());
}

BOOST_AUTO_TEST_CASE(command_stack_branch_test) {
    v3d::editor::CommandStack stack;
    const boost::shared_ptr<CountingCommand> first = counter("first");
    const boost::shared_ptr<CountingCommand> second = counter("second");

    stack.push(first);
    stack.undo();
    BOOST_CHECK(stack.canRedo());

    // a new change starts a new branch of the history, so the undone commands cannot be
    // redone
    stack.push(second);
    BOOST_CHECK(!stack.canRedo());
    BOOST_CHECK_EQUAL(stack.undoDepth(), 1U);
    BOOST_CHECK(stack.undo() == second);
}

BOOST_AUTO_TEST_CASE(command_stack_capacity_test) {
    v3d::editor::CommandStack stack(2);
    const boost::shared_ptr<CountingCommand> first = counter("first");
    const boost::shared_ptr<CountingCommand> second = counter("second");
    const boost::shared_ptr<CountingCommand> third = counter("third");

    stack.push(first);
    stack.push(second);
    stack.push(third);

    // the oldest is dropped rather than the newest, and dropping it does not undo it
    BOOST_CHECK_EQUAL(stack.undoDepth(), 2U);
    BOOST_CHECK_EQUAL(first->undone, 0);
    BOOST_CHECK(stack.undo() == third);
    BOOST_CHECK(stack.undo() == second);
    BOOST_CHECK(!stack.canUndo());

    // a capacity of none would be a stack that cannot record anything
    v3d::editor::CommandStack smallest(0);
    smallest.push(first);
    BOOST_CHECK_EQUAL(smallest.undoDepth(), 1U);
}

BOOST_AUTO_TEST_CASE(command_stack_clear_test) {
    v3d::editor::CommandStack stack;
    const boost::shared_ptr<CountingCommand> first = counter("first");

    stack.push(first);
    stack.clear();

    // the scene the commands describe is being replaced, so nothing is undone on the way out
    BOOST_CHECK(!stack.canUndo());
    BOOST_CHECK(!stack.canRedo());
    BOOST_CHECK_EQUAL(first->undone, 0);
}

BOOST_AUTO_TEST_CASE(command_stack_saved_position_test) {
    v3d::editor::CommandStack stack;

    // a new document has nothing to lose
    BOOST_CHECK(!stack.dirty());

    stack.push(counter("first"));
    BOOST_CHECK(stack.dirty());
    stack.markSaved();
    BOOST_CHECK(!stack.dirty());

    // the document is dirty on either side of the saved position, and clean back on it
    stack.push(counter("second"));
    BOOST_CHECK(stack.dirty());
    stack.undo();
    BOOST_CHECK(!stack.dirty());
    stack.undo();
    BOOST_CHECK(stack.dirty());
    stack.redo();
    BOOST_CHECK(!stack.dirty());
}

BOOST_AUTO_TEST_CASE(command_stack_saved_position_discarded_test) {
    v3d::editor::CommandStack stack;

    stack.push(counter("first"));
    stack.push(counter("second"));
    stack.markSaved();

    // the saved position is among the undone commands, and a push discards them
    stack.undo();
    stack.push(counter("third"));
    BOOST_CHECK(stack.dirty());

    // undoing back to the same depth reaches a different document
    stack.undo();
    stack.undo();
    BOOST_CHECK(stack.dirty());
    stack.redo();
    stack.redo();
    BOOST_CHECK(stack.dirty());

    stack.markSaved();
    BOOST_CHECK(!stack.dirty());
}

BOOST_AUTO_TEST_CASE(command_stack_saved_position_dropped_test) {
    v3d::editor::CommandStack stack(2);

    // saved after the oldest command: dropping that command keeps the position reachable,
    // by undoing everything left
    stack.push(counter("first"));
    stack.markSaved();
    stack.push(counter("second"));
    stack.push(counter("third"));
    BOOST_CHECK(stack.dirty());
    stack.undo();
    stack.undo();
    BOOST_CHECK(!stack.dirty());

    // saved before the oldest command: once it is dropped, no undo reaches the position
    v3d::editor::CommandStack lost(2);
    lost.push(counter("first"));
    lost.push(counter("second"));
    lost.push(counter("third"));
    lost.undo();
    lost.undo();
    BOOST_CHECK(lost.dirty());
}

BOOST_AUTO_TEST_CASE(command_stack_clear_is_saved_test) {
    v3d::editor::CommandStack stack;

    stack.push(counter("first"));
    BOOST_CHECK(stack.dirty());

    // clearing is what opening a document does, and an opened document is saved
    stack.clear();
    BOOST_CHECK(!stack.dirty());
}

BOOST_AUTO_TEST_CASE(create_command_test) {
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    const boost::shared_ptr<v3d::brep::BRep> cube = v3d::editor::create_poly_cube();
    const unsigned int id = cube->id();

    const boost::shared_ptr<v3d::editor::CreateCommand> command =
        boost::make_shared<v3d::editor::CreateCommand>(scene, cube, "cube");
    BOOST_CHECK_EQUAL(command->name(), "create cube");

    // the command does the creating, so the first do and a redo run the same code
    command->redo();
    BOOST_CHECK_EQUAL(scene->count(), 1U);
    BOOST_CHECK(scene->selection() == v3d::editor::Meshes{cube});

    command->undo();
    BOOST_CHECK_EQUAL(scene->count(), 0U);
    BOOST_CHECK(scene->selection().empty());
    BOOST_CHECK(!cube->selected());

    // the mesh outlives its removal, so it comes back with the id that a command deeper in
    // the history still names
    command->redo();
    BOOST_CHECK_EQUAL(scene->count(), 1U);
    BOOST_CHECK(scene->mesh(id) == cube);
}

BOOST_AUTO_TEST_CASE(create_command_selection_test) {
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    const boost::shared_ptr<v3d::brep::BRep> plane = v3d::editor::create_poly_plane();
    const boost::shared_ptr<v3d::brep::BRep> cube = v3d::editor::create_poly_cube();

    plane->selected(true);
    scene->add(plane);

    v3d::editor::CreateCommand command(scene, cube, "cube");
    command.redo();
    BOOST_CHECK_EQUAL(scene->selection().size(), 1U);
    BOOST_CHECK(scene->selection() == v3d::editor::Meshes{cube});

    // the flag lives on the mesh, so undoing the create clears it
    command.undo();
    BOOST_CHECK(scene->selection().empty());

    command.redo();
    BOOST_CHECK_EQUAL(scene->selection().size(), 1U);
    BOOST_CHECK(scene->selection() == v3d::editor::Meshes{cube});
    BOOST_CHECK(!plane->selected());

    // with the plane added to the selection, undoing the create takes the cube out of it
    // and leaves the plane selected
    plane->selected(true);
    command.undo();
    BOOST_CHECK_EQUAL(scene->selection().size(), 1U);
    BOOST_CHECK(scene->selection() == v3d::editor::Meshes{plane});
    BOOST_CHECK(!cube->selected());
}

BOOST_AUTO_TEST_CASE(transform_command_test) {
    const boost::shared_ptr<v3d::brep::BRep> cube = v3d::editor::create_poly_cube();

    const v3d::editor::Placement before = v3d::editor::Placement::of(*cube);

    cube->translation(glm::vec3(1.0F, 2.0F, 3.0F));
    cube->rotation(glm::angleAxis(1.0F, glm::vec3(0.0F, 1.0F, 0.0F)));
    cube->scale(glm::vec3(2.0F, 2.0F, 2.0F));
    const v3d::editor::Placement after = v3d::editor::Placement::of(*cube);

    BOOST_CHECK(!before.same(after));

    v3d::editor::TransformCommand command({{.mesh = cube, .before = before, .after = after}}, "translate");
    BOOST_CHECK_EQUAL(command.name(), "translate");

    // a gesture is undone by restoring the whole placement, not only the part that moved
    command.undo();
    BOOST_CHECK_SMALL(cube->translation().x, 0.001F);
    BOOST_CHECK_CLOSE(cube->scale().x, 1.0F, 0.1F);
    BOOST_CHECK(v3d::editor::Placement::of(*cube).same(before));

    command.redo();
    BOOST_CHECK(v3d::editor::Placement::of(*cube).same(after));
    BOOST_CHECK_CLOSE(cube->translation().y, 2.0F, 0.1F);
}

BOOST_AUTO_TEST_CASE(placement_tolerance_test) {
    const v3d::editor::Placement one;
    v3d::editor::Placement two;

    // the default placement is the one a mesh is built with
    BOOST_CHECK(one.same(two));

    // a gesture that measured nothing still composes a rotation, so the two ends of it
    // differ in the last bits without the object having moved
    two.translation.x = 1e-9F;
    BOOST_CHECK(one.same(two));

    two.translation.x = 0.001F;
    BOOST_CHECK(!one.same(two));
}

};  // namespace
