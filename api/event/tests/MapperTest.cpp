/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/event/Chord.h>
#include <api/event/Context.h>
#include <api/event/Event.h>
#include <api/event/Mapper.h>
#include <api/event/State.h>
#include <api/event/Type.h>

#include <algorithm>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

/**
 * A Mapper holds the source-to-destination bindings and returns what a source event maps to.
 **/
namespace {
v3d::event::Event source(const boost::shared_ptr<v3d::event::Context>& context,
    const std::string& name, v3d::event::State state) {
    v3d::event::Event event(name, context);
    event.type(v3d::event::Type::Source);
    event.state(state);
    return event;
}

v3d::event::Event destination(const boost::shared_ptr<v3d::event::Context>& context, const std::string& name) {
    v3d::event::Event event(name, context);
    event.type(v3d::event::Type::Destination);
    return event;
}

/**
 * A chord claims its key from the plain binding while its modifiers are held, and the plain
 * binding fires while they are not. Of two chords on one key, the one naming more of the
 * modifiers held wins.
 **/
BOOST_AUTO_TEST_CASE(mapper_the_most_specific_chord_wins_test) {
    const boost::shared_ptr<v3d::event::Context> keyboard = boost::make_shared<v3d::event::Context>("keyboard");
    const boost::shared_ptr<v3d::event::Context> edit = boost::make_shared<v3d::event::Context>("edit");
    v3d::event::Mapper mapper("global");
    mapper.map(source(keyboard, "z", v3d::event::State::Any), destination(edit, "type"));
    mapper.map(source(keyboard, "ctrl+z", v3d::event::State::Any), destination(edit, "undo"));
    mapper.map(source(keyboard, "ctrl+shift+z", v3d::event::State::Any), destination(edit, "redo"));

    const v3d::event::Event z = source(keyboard, "z", v3d::event::State::Pressed);
    const auto fired = [&](const v3d::event::Modifiers& held) {
        const std::vector<v3d::event::Event> found = mapper.destinations(z, held);
        BOOST_REQUIRE_EQUAL(found.size(), 1U);
        return std::string(found[0].name());
    };
    BOOST_TEST(fired(v3d::event::Modifiers()) == "type");
    BOOST_TEST(fired({.ctrl = true, .shift = false, .alt = false}) == "undo");
    BOOST_TEST(fired({.ctrl = true, .shift = true, .alt = false}) == "redo");
    // Alt is claimed by nothing, so it falls through to what Ctrl alone binds
    BOOST_TEST(fired({.ctrl = true, .shift = false, .alt = true}) == "undo");
}

/**
 * A plain binding fires under a modifier no chord on its key claims, so a drag still works
 * while a modifier held for a camera move is down.
 **/
BOOST_AUTO_TEST_CASE(mapper_a_plain_binding_fires_under_an_unclaimed_modifier_test) {
    const boost::shared_ptr<v3d::event::Context> mouse = boost::make_shared<v3d::event::Context>("mouse");
    const boost::shared_ptr<v3d::event::Context> view = boost::make_shared<v3d::event::Context>("view");
    v3d::event::Mapper mapper("global");
    mapper.map(source(mouse, "left", v3d::event::State::Any), destination(view, "drag"));

    const std::vector<v3d::event::Event> found = mapper.destinations(source(mouse, "left", v3d::event::State::Pressed),
        {.ctrl = true, .shift = false, .alt = false});
    BOOST_REQUIRE_EQUAL(found.size(), 1U);
    BOOST_TEST(found[0].name() == "drag");
}

/**
 * A chord is reported as it was bound, modifiers and all.
 **/
BOOST_AUTO_TEST_CASE(mapper_reports_a_chord_as_bound_test) {
    const boost::shared_ptr<v3d::event::Context> keyboard = boost::make_shared<v3d::event::Context>("keyboard");
    const boost::shared_ptr<v3d::event::Context> edit = boost::make_shared<v3d::event::Context>("edit");
    v3d::event::Mapper mapper("global");
    mapper.map(source(keyboard, "ctrl+z", v3d::event::State::Pressed), destination(edit, "undo"));

    const std::vector<v3d::event::Event> found = mapper.sources("edit::undo");
    BOOST_REQUIRE_EQUAL(found.size(), 1U);
    BOOST_TEST(found[0].name() == "ctrl+z");
    BOOST_CHECK(found[0].state() == v3d::event::State::Pressed);
}

BOOST_AUTO_TEST_CASE(mapper_test) {
    const boost::shared_ptr<v3d::event::Context> keyboard = boost::make_shared<v3d::event::Context>("keyboard");
    const boost::shared_ptr<v3d::event::Context> game = boost::make_shared<v3d::event::Context>("pong");

    v3d::event::Mapper mapper("global");
    BOOST_CHECK_EQUAL(mapper.name(), "global");

    // nothing is bound yet
    BOOST_CHECK_EQUAL(mapper.destinations(source(keyboard, "w", v3d::event::State::Pressed)).size(), 0U);

    mapper.map(source(keyboard, "w", v3d::event::State::Any), destination(game, "leftPaddleUp"));

    std::vector<v3d::event::Event> found = mapper.destinations(source(keyboard, "w", v3d::event::State::Pressed));
    BOOST_REQUIRE_EQUAL(found.size(), 1U);
    BOOST_CHECK_EQUAL(found[0].name(), "leftPaddleUp");
    BOOST_CHECK_EQUAL(found[0].context(), game);

    // an unbound key maps to nothing
    BOOST_CHECK_EQUAL(mapper.destinations(source(keyboard, "q", v3d::event::State::Pressed)).size(), 0U);

    // and neither does the same name in another context
    const boost::shared_ptr<v3d::event::Context> mouse = boost::make_shared<v3d::event::Context>("mouse");
    BOOST_CHECK_EQUAL(mapper.destinations(source(mouse, "w", v3d::event::State::Pressed)).size(), 0U);
}

BOOST_AUTO_TEST_CASE(mapper_edge_test) {
    const boost::shared_ptr<v3d::event::Context> keyboard = boost::make_shared<v3d::event::Context>("keyboard");
    const boost::shared_ptr<v3d::event::Context> ui = boost::make_shared<v3d::event::Context>("ui");

    v3d::event::Mapper mapper("global");
    mapper.map(source(keyboard, "escape", v3d::event::State::Pressed), destination(ui, "showGameMenu"));

    // a binding on one edge only answers for that edge
    BOOST_CHECK_EQUAL(mapper.destinations(source(keyboard, "escape", v3d::event::State::Pressed)).size(), 1U);
    BOOST_CHECK_EQUAL(mapper.destinations(source(keyboard, "escape", v3d::event::State::Released)).size(), 0U);

    // a binding with no edge matches both, as a held key needs
    mapper.map(source(keyboard, "w", v3d::event::State::Any), destination(ui, "up"));
    BOOST_CHECK_EQUAL(mapper.destinations(source(keyboard, "w", v3d::event::State::Pressed)).size(), 1U);
    BOOST_CHECK_EQUAL(mapper.destinations(source(keyboard, "w", v3d::event::State::Released)).size(), 1U);
}

BOOST_AUTO_TEST_CASE(mapper_multiple_bindings_test) {
    const boost::shared_ptr<v3d::event::Context> keyboard = boost::make_shared<v3d::event::Context>("keyboard");
    const boost::shared_ptr<v3d::event::Context> game = boost::make_shared<v3d::event::Context>("pong");
    const boost::shared_ptr<v3d::event::Context> ui = boost::make_shared<v3d::event::Context>("ui");

    v3d::event::Mapper mapper("global");

    // one key can drive more than one command, and the mapper keeps every binding
    mapper.map(source(keyboard, "arrow_up", v3d::event::State::Any), destination(game, "rightPaddleUp"));
    mapper.map(source(keyboard, "arrow_up", v3d::event::State::Pressed), destination(ui, "menuPrevious"));

    const std::vector<v3d::event::Event> pressed = mapper.destinations(source(keyboard, "arrow_up", v3d::event::State::Pressed));
    BOOST_CHECK_EQUAL(pressed.size(), 2U);

    // on release only the edgeless binding answers
    std::vector<v3d::event::Event> released = mapper.destinations(source(keyboard, "arrow_up", v3d::event::State::Released));
    BOOST_REQUIRE_EQUAL(released.size(), 1U);
    BOOST_CHECK_EQUAL(released[0].name(), "rightPaddleUp");

    // a neighbouring key is not caught up in the run
    mapper.map(source(keyboard, "arrow_down", v3d::event::State::Any), destination(game, "rightPaddleDown"));
    std::vector<v3d::event::Event> down = mapper.destinations(source(keyboard, "arrow_down", v3d::event::State::Pressed));
    BOOST_REQUIRE_EQUAL(down.size(), 1U);
    BOOST_CHECK_EQUAL(down[0].name(), "rightPaddleDown");
}

BOOST_AUTO_TEST_CASE(mapper_parameter_test) {
    const boost::shared_ptr<v3d::event::Context> keyboard = boost::make_shared<v3d::event::Context>("keyboard");
    const boost::shared_ptr<v3d::event::Context> ui = boost::make_shared<v3d::event::Context>("ui");

    v3d::event::Mapper mapper("global");

    // one action, told apart by the parameter each binding carries
    v3d::event::Event slotOne = destination(ui, "selectSlot");
    slotOne.data(1);
    v3d::event::Event slotTwo = destination(ui, "selectSlot");
    slotTwo.data(2);
    mapper.map(source(keyboard, "1", v3d::event::State::Pressed), slotOne);
    mapper.map(source(keyboard, "2", v3d::event::State::Pressed), slotTwo);

    std::vector<v3d::event::Event> first = mapper.destinations(source(keyboard, "1", v3d::event::State::Pressed));
    BOOST_REQUIRE_EQUAL(first.size(), 1U);
    BOOST_REQUIRE(first[0].data());
    BOOST_CHECK_EQUAL(std::get<int>(first[0].data().get()), 1);

    std::vector<v3d::event::Event> second = mapper.destinations(source(keyboard, "2", v3d::event::State::Pressed));
    BOOST_REQUIRE_EQUAL(second.size(), 1U);
    BOOST_REQUIRE(second[0].data());
    BOOST_CHECK_EQUAL(std::get<int>(second[0].data().get()), 2);
}

/**
 * A destination's sources are every binding that drives it, on any edge and under any
 * parameter, and nothing for a destination nothing drives.
 **/
BOOST_AUTO_TEST_CASE(mapper_sources_test) {
    const boost::shared_ptr<v3d::event::Context> keyboard = boost::make_shared<v3d::event::Context>("keyboard");
    const boost::shared_ptr<v3d::event::Context> game = boost::make_shared<v3d::event::Context>("game");

    v3d::event::Mapper mapper("global");
    v3d::event::Event north = destination(game, "walkNorth");
    north.data(1);
    mapper.map(source(keyboard, "w", v3d::event::State::Any), north);
    mapper.map(source(keyboard, "arrow_up", v3d::event::State::Pressed), destination(game, "walkNorth"));
    mapper.map(source(keyboard, "s", v3d::event::State::Any), destination(game, "walkSouth"));

    const std::vector<v3d::event::Event> found = mapper.sources("game::walkNorth");
    BOOST_REQUIRE_EQUAL(found.size(), 2U);
    std::vector<std::string> names;
    names.reserve(found.size());
    for (const v3d::event::Event& each : found) {
        names.emplace_back(each.name());
    }
    std::ranges::sort(names);
    BOOST_CHECK_EQUAL(names[0], "arrow_up");
    BOOST_CHECK_EQUAL(names[1], "w");

    BOOST_CHECK(mapper.sources("game::walkEast").empty());
    // a name alone is not a command: the context is part of it
    BOOST_CHECK(mapper.sources("walkNorth").empty());
}

};  // namespace
