/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/event/Event.h>
#include <api/event/MouseButton.h>
#include <api/event/MouseMotion.h>

#include <string>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

/**
 * The replacement for EventInfo: a name, the context it belongs to, the edge it happened on
 * and an optional parameter.
 **/
BOOST_AUTO_TEST_CASE(event_test) {
    boost::shared_ptr<v3d::event::Context> keyboard = boost::make_shared<v3d::event::Context>("keyboard");

    v3d::event::Event event("myevent", keyboard);
    BOOST_CHECK_EQUAL(event.name(), "myevent");
    BOOST_CHECK_EQUAL(event.context(), keyboard);
    BOOST_CHECK_EQUAL(event.str(), "keyboard::myevent");

    // an event with no context is just its name
    v3d::event::Event bare("bare");
    BOOST_CHECK_EQUAL(bare.str(), "bare");
    BOOST_CHECK(bare.context() == nullptr);

    // the state is the edge, and defaults to matching either one
    BOOST_CHECK(event.state() == v3d::event::State::Any);
    event.state(v3d::event::State::Pressed);
    BOOST_CHECK(event.state() == v3d::event::State::Pressed);

    // the type says which side of a binding this is
    BOOST_CHECK(event.type() == v3d::event::Type::Unknown);
    event.type(v3d::event::Type::Source);
    BOOST_CHECK(event.type() == v3d::event::Type::Source);
}

BOOST_AUTO_TEST_CASE(event_data_test) {
    boost::shared_ptr<v3d::event::Context> ui = boost::make_shared<v3d::event::Context>("ui");

    // an event carries no parameter unless one is given to it
    v3d::event::Event plain("setMaxScore", ui);
    BOOST_CHECK(!plain.data());

    v3d::event::Event numeric("setMaxScore", ui);
    numeric.data(7);
    BOOST_REQUIRE(numeric.data());
    BOOST_CHECK_EQUAL(std::get<int>(numeric.data().get()), 7);

    v3d::event::Event flag("setCoopMode", ui);
    flag.data(true);
    BOOST_REQUIRE(flag.data());
    BOOST_CHECK_EQUAL(std::get<bool>(flag.data().get()), true);

    v3d::event::Event named("selectLevel", ui);
    named.data(std::string("hard"));
    BOOST_REQUIRE(named.data());
    BOOST_CHECK_EQUAL(std::get<std::string>(named.data().get()), "hard");
}

BOOST_AUTO_TEST_CASE(event_ordering_test) {
    boost::shared_ptr<v3d::event::Context> keyboard = boost::make_shared<v3d::event::Context>("keyboard");
    boost::shared_ptr<v3d::event::Context> mouse = boost::make_shared<v3d::event::Context>("mouse");

    // events order by context and name first, which is what keeps every binding on one key
    // together in the mapper's multimap
    v3d::event::Event escape("escape", keyboard);
    v3d::event::Event ret("return", keyboard);
    BOOST_CHECK(escape < ret);
    BOOST_CHECK(!(ret < escape));

    // the same name in two contexts is two different events
    v3d::event::Event left("left", keyboard);
    v3d::event::Event mouseLeft("left", mouse);
    BOOST_CHECK((left < mouseLeft) || (mouseLeft < left));

    // and the edge orders within one identity, so a run of bindings on one key is contiguous
    v3d::event::Event any("escape", keyboard);
    v3d::event::Event pressed("escape", keyboard);
    pressed.state(v3d::event::State::Pressed);
    v3d::event::Event released("escape", keyboard);
    released.state(v3d::event::State::Released);
    BOOST_CHECK(any < pressed);
    BOOST_CHECK(pressed < released);
    BOOST_CHECK(!(released < any));

    // a parameter is not part of the identity - two bindings on one key differ only by it
    v3d::event::Event withData("escape", keyboard);
    withData.data(3);
    BOOST_CHECK(!(any < withData));
    BOOST_CHECK(!(withData < any));
}

BOOST_AUTO_TEST_CASE(event_state_name_test) {
    // the names a binding config uses for the two edges
    BOOST_CHECK(v3d::event::stringToState("pressed") == v3d::event::State::Pressed);
    BOOST_CHECK(v3d::event::stringToState("down") == v3d::event::State::Pressed);
    BOOST_CHECK(v3d::event::stringToState("released") == v3d::event::State::Released);
    BOOST_CHECK(v3d::event::stringToState("up") == v3d::event::State::Released);

    // anything else, including nothing at all, binds both edges
    BOOST_CHECK(v3d::event::stringToState("") == v3d::event::State::Any);
    BOOST_CHECK(v3d::event::stringToState("sideways") == v3d::event::State::Any);
}

/**
 * Both mouse events carry a position. The assertion is thin on purpose: what it prevents is a
 * field going back to being dropped between the SDL event and the one dispatched from it.
 **/
BOOST_AUTO_TEST_CASE(mouse_event_position_test) {
    boost::shared_ptr<v3d::event::Context> mouse = boost::make_shared<v3d::event::Context>("mouse");

    const v3d::event::MouseButton press(1, glm::vec2(4.0f, 9.0f), mouse, true);
    BOOST_CHECK_EQUAL(press.button(), 1u);
    BOOST_CHECK_EQUAL(press.position()[0], 4.0f);
    BOOST_CHECK_EQUAL(press.position()[1], 9.0f);
    BOOST_CHECK_EQUAL(press.pressed(), true);
    BOOST_CHECK(press.state() == v3d::event::State::Pressed);

    const v3d::event::MouseButton release(1, glm::vec2(4.0f, 9.0f), mouse, false);
    BOOST_CHECK(release.state() == v3d::event::State::Released);

    const v3d::event::MouseMotion moved(glm::vec2(4.0f, 9.0f), glm::vec2(1.0f, -1.0f), mouse);
    BOOST_CHECK_EQUAL(moved.position()[0], press.position()[0]);
    BOOST_CHECK_EQUAL(moved.motion()[1], -1.0f);
}
