/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/event/Mapper.h>

#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

/**
 * The replacement for the command layer's Bind: a Mapper holds the source-to-destination
 * bindings and answers what a source event maps to.
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
};  // namespace

BOOST_AUTO_TEST_CASE(mapper_test) {
    boost::shared_ptr<v3d::event::Context> keyboard = boost::make_shared<v3d::event::Context>("keyboard");
    boost::shared_ptr<v3d::event::Context> game = boost::make_shared<v3d::event::Context>("pong");

    v3d::event::Mapper mapper("global");
    BOOST_CHECK_EQUAL(mapper.name(), "global");

    // nothing is bound yet
    BOOST_CHECK_EQUAL(mapper.destinations(source(keyboard, "w", v3d::event::State::Pressed)).size(), 0u);

    mapper.map(source(keyboard, "w", v3d::event::State::Any), destination(game, "leftPaddleUp"));

    std::vector<v3d::event::Event> found = mapper.destinations(source(keyboard, "w", v3d::event::State::Pressed));
    BOOST_REQUIRE_EQUAL(found.size(), 1u);
    BOOST_CHECK_EQUAL(found[0].name(), "leftPaddleUp");
    BOOST_CHECK_EQUAL(found[0].context(), game);

    // an unbound key maps to nothing
    BOOST_CHECK_EQUAL(mapper.destinations(source(keyboard, "q", v3d::event::State::Pressed)).size(), 0u);

    // and neither does the same name in another context
    boost::shared_ptr<v3d::event::Context> mouse = boost::make_shared<v3d::event::Context>("mouse");
    BOOST_CHECK_EQUAL(mapper.destinations(source(mouse, "w", v3d::event::State::Pressed)).size(), 0u);
}

BOOST_AUTO_TEST_CASE(mapper_edge_test) {
    boost::shared_ptr<v3d::event::Context> keyboard = boost::make_shared<v3d::event::Context>("keyboard");
    boost::shared_ptr<v3d::event::Context> ui = boost::make_shared<v3d::event::Context>("ui");

    v3d::event::Mapper mapper("global");
    mapper.map(source(keyboard, "escape", v3d::event::State::Pressed), destination(ui, "showGameMenu"));

    // a binding on one edge only answers for that edge
    BOOST_CHECK_EQUAL(mapper.destinations(source(keyboard, "escape", v3d::event::State::Pressed)).size(), 1u);
    BOOST_CHECK_EQUAL(mapper.destinations(source(keyboard, "escape", v3d::event::State::Released)).size(), 0u);

    // a binding with no edge answers for both, which is what a held key needs
    mapper.map(source(keyboard, "w", v3d::event::State::Any), destination(ui, "up"));
    BOOST_CHECK_EQUAL(mapper.destinations(source(keyboard, "w", v3d::event::State::Pressed)).size(), 1u);
    BOOST_CHECK_EQUAL(mapper.destinations(source(keyboard, "w", v3d::event::State::Released)).size(), 1u);
}

BOOST_AUTO_TEST_CASE(mapper_multiple_bindings_test) {
    boost::shared_ptr<v3d::event::Context> keyboard = boost::make_shared<v3d::event::Context>("keyboard");
    boost::shared_ptr<v3d::event::Context> game = boost::make_shared<v3d::event::Context>("pong");
    boost::shared_ptr<v3d::event::Context> ui = boost::make_shared<v3d::event::Context>("ui");

    v3d::event::Mapper mapper("global");

    // one key can drive more than one command, so a mapper keyed by source alone would
    // silently keep only the last binding
    mapper.map(source(keyboard, "arrow_up", v3d::event::State::Any), destination(game, "rightPaddleUp"));
    mapper.map(source(keyboard, "arrow_up", v3d::event::State::Pressed), destination(ui, "menuPrevious"));

    std::vector<v3d::event::Event> pressed = mapper.destinations(source(keyboard, "arrow_up", v3d::event::State::Pressed));
    BOOST_CHECK_EQUAL(pressed.size(), 2u);

    // on release only the edgeless binding answers
    std::vector<v3d::event::Event> released = mapper.destinations(source(keyboard, "arrow_up", v3d::event::State::Released));
    BOOST_REQUIRE_EQUAL(released.size(), 1u);
    BOOST_CHECK_EQUAL(released[0].name(), "rightPaddleUp");

    // a neighbouring key is not caught up in the run
    mapper.map(source(keyboard, "arrow_down", v3d::event::State::Any), destination(game, "rightPaddleDown"));
    std::vector<v3d::event::Event> down = mapper.destinations(source(keyboard, "arrow_down", v3d::event::State::Pressed));
    BOOST_REQUIRE_EQUAL(down.size(), 1u);
    BOOST_CHECK_EQUAL(down[0].name(), "rightPaddleDown");
}

BOOST_AUTO_TEST_CASE(mapper_parameter_test) {
    boost::shared_ptr<v3d::event::Context> keyboard = boost::make_shared<v3d::event::Context>("keyboard");
    boost::shared_ptr<v3d::event::Context> ui = boost::make_shared<v3d::event::Context>("ui");

    v3d::event::Mapper mapper("global");

    // one action, told apart by the parameter each binding carries
    v3d::event::Event slotOne = destination(ui, "selectSlot");
    slotOne.data(1);
    v3d::event::Event slotTwo = destination(ui, "selectSlot");
    slotTwo.data(2);
    mapper.map(source(keyboard, "1", v3d::event::State::Pressed), slotOne);
    mapper.map(source(keyboard, "2", v3d::event::State::Pressed), slotTwo);

    std::vector<v3d::event::Event> first = mapper.destinations(source(keyboard, "1", v3d::event::State::Pressed));
    BOOST_REQUIRE_EQUAL(first.size(), 1u);
    BOOST_REQUIRE(first[0].data());
    BOOST_CHECK_EQUAL(std::get<int>(first[0].data().get()), 1);

    std::vector<v3d::event::Event> second = mapper.destinations(source(keyboard, "2", v3d::event::State::Pressed));
    BOOST_REQUIRE_EQUAL(second.size(), 1u);
    BOOST_REQUIRE(second[0].data());
    BOOST_CHECK_EQUAL(std::get<int>(second[0].data().get()), 2);
}
