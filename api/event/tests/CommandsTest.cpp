/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/event/Commands.h>
#include <api/event/Context.h>
#include <api/event/Event.h>
#include <api/event/State.h>

#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

namespace {

/**
 * The event a binding or a menu item would deliver.
 **/
v3d::event::Event command(const std::string& context, const std::string& name, v3d::event::State state) {
    v3d::event::Event event(name, boost::make_shared<v3d::event::Context>(context));
    event.state(state);
    return event;
}

BOOST_AUTO_TEST_SUITE(commands_test)

/**
 * A handler is found by the event's identity, which is its context and name together.
 **/
BOOST_AUTO_TEST_CASE(commands_invokes_by_identity) {
    v3d::event::Commands directory;
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
BOOST_AUTO_TEST_CASE(commands_separates_contexts) {
    v3d::event::Commands directory;
    int edit = 0;
    int view = 0;

    directory.add("edit::undo", [&edit](const v3d::event::Event&) { edit++; });
    directory.add("view::undo", [&view](const v3d::event::Event&) { view++; });

    BOOST_CHECK(directory.invoke(command("view", "undo", v3d::event::State::Pressed)));
    BOOST_CHECK_EQUAL(edit, 0);
    BOOST_CHECK_EQUAL(view, 1);
}

/**
 * An unknown command runs nothing and invoke() returns false, so a menu item naming an
 * unregistered command can be reported rather than failing silently.
 **/
BOOST_AUTO_TEST_CASE(commands_reports_an_unknown_command) {
    v3d::event::Commands directory;
    directory.add("create::poly::cube", [](const v3d::event::Event&) { });

    BOOST_CHECK(!directory.invoke(command("timeline", "playback::play", v3d::event::State::Pressed)));
    BOOST_CHECK(!directory.has("timeline::playback::play"));
}

/**
 * A second registration of the same name is refused rather than replacing the first, so
 * two handlers for one command is a failure at startup rather than one of them silently
 * never running.
 **/
BOOST_AUTO_TEST_CASE(commands_refuses_a_duplicate) {
    v3d::event::Commands directory;
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
BOOST_AUTO_TEST_CASE(commands_refuses_nothing_to_register) {
    v3d::event::Commands directory;

    BOOST_CHECK(!directory.add("", [](const v3d::event::Event&) { }));
    BOOST_CHECK(!directory.add("ui::quit", v3d::event::Commands::Handler()));
    BOOST_CHECK(!directory.addPress("ui::quit", v3d::event::Commands::PressHandler()));
    BOOST_CHECK_EQUAL(directory.size(), 0U);
}

/**
 * A handler sees the state, so a command bound to both edges of a key can tell them apart.
 **/
BOOST_AUTO_TEST_CASE(commands_passes_the_state_through) {
    v3d::event::Commands directory;
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
 * A press handler runs on the press and on a binding that names no edge. It does not run on
 * the release or on a held key's repeat. Those are still handled, so the command does not
 * report as unknown.
 **/
BOOST_AUTO_TEST_CASE(commands_ignores_a_release_of_a_press_command) {
    v3d::event::Commands directory;
    int ran = 0;

    directory.addPress("create::poly::cube", [&ran]() { ran++; });

    BOOST_CHECK(directory.invoke(command("create", "poly::cube", v3d::event::State::Pressed)));
    BOOST_CHECK_EQUAL(ran, 1);

    BOOST_CHECK(directory.invoke(command("create", "poly::cube", v3d::event::State::Released)));
    BOOST_CHECK_EQUAL(ran, 1);

    BOOST_CHECK(directory.invoke(command("create", "poly::cube", v3d::event::State::Any)));
    BOOST_CHECK_EQUAL(ran, 2);

    // a held key's repeat is handled and runs nothing
    v3d::event::Event repeated = command("create", "poly::cube", v3d::event::State::Pressed);
    repeated.repeat(true);
    BOOST_CHECK(directory.invoke(repeated));
    BOOST_CHECK_EQUAL(ran, 2);
}

/**
 * The directory lists every registered command, so a menu definition can be checked
 * against it.
 **/
BOOST_AUTO_TEST_CASE(commands_lists_its_names_in_order) {
    v3d::event::Commands directory;
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
 * An event with no context is named by its leaf alone. Nothing a binding sends arrives that
 * way, but the registry still keys on the event's identity.
 **/
BOOST_AUTO_TEST_CASE(commands_invokes_a_contextless_event) {
    v3d::event::Commands directory;
    int ran = 0;
    directory.addPress("quit", [&ran]() { ran++; });

    v3d::event::Event event("quit");
    event.state(v3d::event::State::Pressed);
    BOOST_CHECK(directory.invoke(event));
    BOOST_CHECK_EQUAL(ran, 1);
}

/**
 * A command read by asking whether its key is held is registered with no handler. Its event
 * is handled and runs nothing, so it does not report as unknown.
 **/
BOOST_AUTO_TEST_CASE(commands_handles_a_held_command) {
    v3d::event::Commands directory;

    BOOST_CHECK(directory.addHeld("pong::leftPaddleUp"));
    BOOST_CHECK(directory.has("pong::leftPaddleUp"));
    BOOST_CHECK(directory.invoke(command("pong", "leftPaddleUp", v3d::event::State::Pressed)));
    BOOST_CHECK(!directory.addHeld("pong::leftPaddleUp"));
}

/**
 * A removed command is no longer handled, and its handler does not run.
 **/
BOOST_AUTO_TEST_CASE(commands_forgets_a_removed_command) {
    v3d::event::Commands directory;
    int ran = 0;
    directory.addPress("ui::menuNext", [&ran]() { ran++; });

    BOOST_CHECK(directory.remove("ui::menuNext"));
    BOOST_CHECK(!directory.invoke(command("ui", "menuNext", v3d::event::State::Pressed)));
    BOOST_CHECK_EQUAL(ran, 0);
    BOOST_CHECK(!directory.remove("ui::menuNext"));
}

/**
 * The commands nothing handles are the ones in the list that are not registered, in the
 * list's order.
 **/
BOOST_AUTO_TEST_CASE(commands_lists_what_nothing_handles) {
    v3d::event::Commands directory;
    directory.addPress("ui::quit", []() { });
    directory.addHeld("odyssey::moveForward");

    const std::vector<std::string> missing =
        directory.unhandled({ "odyssey::moveUp", "ui::quit", "pong::toggleFS", "odyssey::moveForward" });
    BOOST_REQUIRE_EQUAL(missing.size(), 2U);
    BOOST_CHECK_EQUAL(missing[0], "odyssey::moveUp");
    BOOST_CHECK_EQUAL(missing[1], "pong::toggleFS");
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
