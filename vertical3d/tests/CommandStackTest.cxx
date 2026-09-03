/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <string>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/vec3.hpp>

#include "../src/CommandStack.h"
#include "../src/CreateCommand.h"
#include "../src/CreatePoly.h"
#include "../src/Scene.h"
#include "../src/TransformCommand.h"

namespace {

    /**
     * A command that records how often it has been asked to do and undo itself, which is
     * what the stack's own behaviour is measured by.
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

};  // namespace

BOOST_AUTO_TEST_CASE(command_stack_empty_test) {
    v3d::editor::CommandStack stack;

    BOOST_CHECK(!stack.canUndo());
    BOOST_CHECK(!stack.canRedo());
    BOOST_CHECK(!stack.undo());
    BOOST_CHECK(!stack.redo());

    // a stack that is given nothing has recorded nothing
    stack.push(boost::shared_ptr<v3d::editor::Command>());
    BOOST_CHECK_EQUAL(stack.undoDepth(), 0u);
}

BOOST_AUTO_TEST_CASE(command_stack_undo_redo_test) {
    v3d::editor::CommandStack stack;
    boost::shared_ptr<CountingCommand> first = counter("first");
    boost::shared_ptr<CountingCommand> second = counter("second");

    // a command arrives already done, so pushing must not do it again
    stack.push(first);
    stack.push(second);
    BOOST_CHECK_EQUAL(first->redone, 0);
    BOOST_CHECK_EQUAL(second->redone, 0);
    BOOST_CHECK_EQUAL(stack.undoDepth(), 2u);

    boost::shared_ptr<v3d::editor::Command> undone = stack.undo();
    BOOST_CHECK(undone == second);
    BOOST_CHECK_EQUAL(second->undone, 1);
    BOOST_CHECK_EQUAL(first->undone, 0);
    BOOST_CHECK_EQUAL(stack.undoDepth(), 1u);
    BOOST_CHECK_EQUAL(stack.redoDepth(), 1u);

    boost::shared_ptr<v3d::editor::Command> redone = stack.redo();
    BOOST_CHECK(redone == second);
    BOOST_CHECK_EQUAL(second->redone, 1);
    BOOST_CHECK_EQUAL(stack.undoDepth(), 2u);
    BOOST_CHECK(!stack.canRedo());

    // and the whole history unwinds in order
    BOOST_CHECK(stack.undo() == second);
    BOOST_CHECK(stack.undo() == first);
    BOOST_CHECK(!stack.canUndo());
}

BOOST_AUTO_TEST_CASE(command_stack_branch_test) {
    v3d::editor::CommandStack stack;
    boost::shared_ptr<CountingCommand> first = counter("first");
    boost::shared_ptr<CountingCommand> second = counter("second");

    stack.push(first);
    stack.undo();
    BOOST_CHECK(stack.canRedo());

    // a new change is a branch of the history that was never taken, so what had been undone
    // is not reachable any more
    stack.push(second);
    BOOST_CHECK(!stack.canRedo());
    BOOST_CHECK_EQUAL(stack.undoDepth(), 1u);
    BOOST_CHECK(stack.undo() == second);
}

BOOST_AUTO_TEST_CASE(command_stack_capacity_test) {
    v3d::editor::CommandStack stack(2);
    boost::shared_ptr<CountingCommand> first = counter("first");
    boost::shared_ptr<CountingCommand> second = counter("second");
    boost::shared_ptr<CountingCommand> third = counter("third");

    stack.push(first);
    stack.push(second);
    stack.push(third);

    // the oldest is dropped rather than the newest, and dropping it does not undo it
    BOOST_CHECK_EQUAL(stack.undoDepth(), 2u);
    BOOST_CHECK_EQUAL(first->undone, 0);
    BOOST_CHECK(stack.undo() == third);
    BOOST_CHECK(stack.undo() == second);
    BOOST_CHECK(!stack.canUndo());

    // a capacity of none would be a stack that cannot record anything
    v3d::editor::CommandStack smallest(0);
    smallest.push(first);
    BOOST_CHECK_EQUAL(smallest.undoDepth(), 1u);
}

BOOST_AUTO_TEST_CASE(command_stack_clear_test) {
    v3d::editor::CommandStack stack;
    boost::shared_ptr<CountingCommand> first = counter("first");

    stack.push(first);
    stack.clear();

    // the scene the commands describe is being replaced, so nothing is undone on the way out
    BOOST_CHECK(!stack.canUndo());
    BOOST_CHECK(!stack.canRedo());
    BOOST_CHECK_EQUAL(first->undone, 0);
}

BOOST_AUTO_TEST_CASE(create_command_test) {
    boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    boost::shared_ptr<v3d::brep::BRep> cube = v3d::editor::create_poly_cube();
    const unsigned int id = cube->id();

    boost::shared_ptr<v3d::editor::CreateCommand> command =
        boost::make_shared<v3d::editor::CreateCommand>(scene, cube, "cube");
    BOOST_CHECK_EQUAL(command->name(), "create cube");

    // the command is what creates, so the first do and a redo are the same code
    command->redo();
    BOOST_CHECK_EQUAL(scene->count(), 1u);
    BOOST_CHECK(scene->selection() == cube);

    command->undo();
    BOOST_CHECK_EQUAL(scene->count(), 0u);
    BOOST_CHECK(!scene->selection());
    BOOST_CHECK(!cube->selected());

    // the mesh outlives its removal, which is what lets it come back with the id a deeper
    // command still names
    command->redo();
    BOOST_CHECK_EQUAL(scene->count(), 1u);
    BOOST_CHECK(scene->mesh(id) == cube);
}

BOOST_AUTO_TEST_CASE(create_command_selection_test) {
    boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    boost::shared_ptr<v3d::brep::BRep> plane = v3d::editor::create_poly_plane();
    boost::shared_ptr<v3d::brep::BRep> cube = v3d::editor::create_poly_cube();

    plane->selected(true);
    scene->add(plane);

    v3d::editor::CreateCommand command(scene, cube, "cube");
    command.redo();
    BOOST_CHECK(scene->selection() == cube);

    // undoing the create must leave one selection at most, and the flag lives on the mesh
    command.undo();
    BOOST_CHECK(!scene->selection());

    command.redo();
    BOOST_CHECK(scene->selection() == cube);
    BOOST_CHECK(!plane->selected());
}

BOOST_AUTO_TEST_CASE(transform_command_test) {
    boost::shared_ptr<v3d::brep::BRep> cube = v3d::editor::create_poly_cube();

    const v3d::editor::Placement before = v3d::editor::Placement::of(*cube);

    cube->translation(glm::vec3(1.0f, 2.0f, 3.0f));
    cube->rotation(glm::angleAxis(1.0f, glm::vec3(0.0f, 1.0f, 0.0f)));
    cube->scale(glm::vec3(2.0f, 2.0f, 2.0f));
    const v3d::editor::Placement after = v3d::editor::Placement::of(*cube);

    BOOST_CHECK(!before.same(after));

    v3d::editor::TransformCommand command(cube, before, after, "translate");
    BOOST_CHECK_EQUAL(command.name(), "translate");

    // a gesture is undone by putting the whole placement back, not the part that moved
    command.undo();
    BOOST_CHECK_SMALL(cube->translation().x, 0.001f);
    BOOST_CHECK_CLOSE(cube->scale().x, 1.0f, 0.1f);
    BOOST_CHECK(v3d::editor::Placement::of(*cube).same(before));

    command.redo();
    BOOST_CHECK(v3d::editor::Placement::of(*cube).same(after));
    BOOST_CHECK_CLOSE(cube->translation().y, 2.0f, 0.1f);
}

BOOST_AUTO_TEST_CASE(placement_tolerance_test) {
    v3d::editor::Placement one;
    v3d::editor::Placement two;

    // the default placement is the one a mesh is built with
    BOOST_CHECK(one.same(two));

    // a gesture that measured nothing still composes a rotation, so the two ends of it
    // differ in the last bits without the object having moved
    two.translation.x = 1e-9f;
    BOOST_CHECK(one.same(two));

    two.translation.x = 0.001f;
    BOOST_CHECK(!one.same(two));
}
