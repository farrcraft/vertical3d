/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

#include "../../src/command/CommandDirectory.h"

#include "../../../api/event/Context.h"
#include "../../../api/event/Event.h"
#include "../../../api/event/State.h"

namespace {

    /**
     * The event a binding or a menu item would deliver.
     **/
    v3d::event::Event command(const std::string& context, const std::string& name, v3d::event::State state) {
        v3d::event::Event event(name, boost::make_shared<v3d::event::Context>(context));
        event.state(state);
        return event;
    }

};  // namespace

BOOST_AUTO_TEST_SUITE(commanddirectory_test)

/**
 * A handler is found by the event's identity, which is its context and name together.
 **/
BOOST_AUTO_TEST_CASE(commanddirectory_invokes_by_identity) {
    v3d::editor::CommandDirectory directory;
    int ran = 0;

    BOOST_CHECK(directory.add("create::poly::cube", [&ran](const v3d::event::Event&) { ran++; }));
    BOOST_CHECK_EQUAL(directory.size(), 1U);
    BOOST_CHECK(directory.has("create::poly::cube"));

    BOOST_CHECK(directory.invoke(command("create", "poly::cube", v3d::event::State::Pressed)));
    BOOST_CHECK_EQUAL(ran, 1);
}

/**
 * The context is part of the name, so the same leaf in two contexts is two commands.
 **/
BOOST_AUTO_TEST_CASE(commanddirectory_separates_contexts) {
    v3d::editor::CommandDirectory directory;
    int edit = 0;
    int view = 0;

    directory.add("edit::undo", [&edit](const v3d::event::Event&) { edit++; });
    directory.add("view::undo", [&view](const v3d::event::Event&) { view++; });

    BOOST_CHECK(directory.invoke(command("view", "undo", v3d::event::State::Pressed)));
    BOOST_CHECK_EQUAL(edit, 0);
    BOOST_CHECK_EQUAL(view, 1);
}

/**
 * An unknown command runs nothing and says so, which is how an untranslated menu item
 * reports itself rather than failing silently.
 **/
BOOST_AUTO_TEST_CASE(commanddirectory_reports_an_unknown_command) {
    v3d::editor::CommandDirectory directory;
    directory.add("create::poly::cube", [](const v3d::event::Event&) { });

    BOOST_CHECK(!directory.invoke(command("timeline", "playback::play", v3d::event::State::Pressed)));
    BOOST_CHECK(!directory.has("timeline::playback::play"));
}

/**
 * A second registration of the same name is refused rather than replacing the first, so
 * two handlers for one command is a failure at startup rather than one of them silently
 * never running.
 **/
BOOST_AUTO_TEST_CASE(commanddirectory_refuses_a_duplicate) {
    v3d::editor::CommandDirectory directory;
    int first = 0;
    int second = 0;

    BOOST_CHECK(directory.add("transform::translate", [&first](const v3d::event::Event&) { first++; }));
    BOOST_CHECK(!directory.add("transform::translate", [&second](const v3d::event::Event&) { second++; }));
    BOOST_CHECK(!directory.addPress("transform::translate", [&second]() { second++; }));
    BOOST_CHECK_EQUAL(directory.size(), 1U);

    directory.invoke(command("transform", "translate", v3d::event::State::Pressed));
    BOOST_CHECK_EQUAL(first, 1);
    BOOST_CHECK_EQUAL(second, 0);
}

/**
 * An empty name and an empty handler are both refused.
 **/
BOOST_AUTO_TEST_CASE(commanddirectory_refuses_nothing_to_register) {
    v3d::editor::CommandDirectory directory;

    BOOST_CHECK(!directory.add("", [](const v3d::event::Event&) { }));
    BOOST_CHECK(!directory.add("ui::quit", v3d::editor::CommandDirectory::Handler()));
    BOOST_CHECK(!directory.addPress("ui::quit", v3d::editor::CommandDirectory::PressHandler()));
    BOOST_CHECK_EQUAL(directory.size(), 0U);
}

/**
 * A handler sees the state, so a command bound to both edges of a key can tell them apart.
 **/
BOOST_AUTO_TEST_CASE(commanddirectory_passes_the_state_through) {
    v3d::editor::CommandDirectory directory;
    int held = 0;

    directory.add("view::camera::zoom", [&held](const v3d::event::Event& event) {
        held += event.state() == v3d::event::State::Released ? -1 : 1;
    });

    directory.invoke(command("view", "camera::zoom", v3d::event::State::Pressed));
    BOOST_CHECK_EQUAL(held, 1);
    directory.invoke(command("view", "camera::zoom", v3d::event::State::Released));
    BOOST_CHECK_EQUAL(held, 0);
}

/**
 * A press handler runs on the press and on a binding that names no edge, but not on the
 * release - which is still handled, so the command does not report as unknown.
 **/
BOOST_AUTO_TEST_CASE(commanddirectory_ignores_a_release_of_a_press_command) {
    v3d::editor::CommandDirectory directory;
    int ran = 0;

    directory.addPress("create::poly::cube", [&ran]() { ran++; });

    BOOST_CHECK(directory.invoke(command("create", "poly::cube", v3d::event::State::Pressed)));
    BOOST_CHECK_EQUAL(ran, 1);

    BOOST_CHECK(directory.invoke(command("create", "poly::cube", v3d::event::State::Released)));
    BOOST_CHECK_EQUAL(ran, 1);

    BOOST_CHECK(directory.invoke(command("create", "poly::cube", v3d::event::State::Any)));
    BOOST_CHECK_EQUAL(ran, 2);
}

/**
 * The directory can say what the editor can do, which is what a menu translated from
 * another tree has to be checked against.
 **/
BOOST_AUTO_TEST_CASE(commanddirectory_lists_its_names_in_order) {
    v3d::editor::CommandDirectory directory;
    directory.addPress("view::show::grid", []() { });
    directory.addPress("create::poly::cube", []() { });
    directory.addPress("edit::undo", []() { });

    const std::vector<std::string> names = directory.names();
    BOOST_REQUIRE_EQUAL(names.size(), 3U);
    BOOST_CHECK_EQUAL(names[0], "create::poly::cube");
    BOOST_CHECK_EQUAL(names[1], "edit::undo");
    BOOST_CHECK_EQUAL(names[2], "view::show::grid");
}

/**
 * An event with no context is named by its leaf alone. Nothing the editor binds arrives
 * that way, but the identity is what the directory keys on either way.
 **/
BOOST_AUTO_TEST_CASE(commanddirectory_invokes_a_contextless_event) {
    v3d::editor::CommandDirectory directory;
    int ran = 0;
    directory.addPress("quit", [&ran]() { ran++; });

    v3d::event::Event event("quit");
    event.state(v3d::event::State::Pressed);
    BOOST_CHECK(directory.invoke(event));
    BOOST_CHECK_EQUAL(ran, 1);
}

BOOST_AUTO_TEST_SUITE_END()
