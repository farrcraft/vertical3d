/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/event/kind/MouseButton.h>
#include <api/event/kind/MouseMotion.h>
#include <api/input/Mouse.h>

#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

/**
 * The device this replaces was v3D::MouseDevice. Mouse::handleEvent was an empty stub that
 * returned true, so every mouse event in every app was swallowed - this is the test that
 * would have caught it.
 **/
namespace {
struct Recorder {
    void button(const v3d::event::kind::MouseButton& event) {
        buttons_.push_back(event);
    }

    void motion(const v3d::event::kind::MouseMotion& event) {
        motion_.push_back(event);
    }

    void sourceEvent(const v3d::event::Event& event) {
        if (event.type() == v3d::event::Type::Source) {
            source_.push_back(event);
        }
    }

    std::vector<v3d::event::kind::MouseButton> buttons_;
    std::vector<v3d::event::kind::MouseMotion> motion_;
    std::vector<v3d::event::Event> source_;
};

SDL_Event buttonEvent(uint32_t type, uint8_t button, float x, float y) {
    SDL_Event event{};
    event.type = type;
    event.button.button = button;
    event.button.x = x;
    event.button.y = y;
    return event;
}

SDL_Event motionEvent(float x, float y, float dx, float dy) {
    SDL_Event event{};
    event.type = SDL_EVENT_MOUSE_MOTION;
    event.motion.x = x;
    event.motion.y = y;
    event.motion.xrel = dx;
    event.motion.yrel = dy;
    return event;
}
};  // namespace

BOOST_AUTO_TEST_CASE(mouse_button_test) {
    boost::shared_ptr<entt::dispatcher> dispatcher = boost::make_shared<entt::dispatcher>();
    boost::shared_ptr<v3d::event::Context> context = boost::make_shared<v3d::event::Context>("mouse");
    v3d::input::Mouse mouse(context, dispatcher);

    Recorder recorder;
    dispatcher->sink<v3d::event::kind::MouseButton>().connect<&Recorder::button>(recorder);
    dispatcher->sink<v3d::event::Event>().connect<&Recorder::sourceEvent>(recorder);

    BOOST_CHECK_EQUAL(mouse.handleEvent(buttonEvent(SDL_EVENT_MOUSE_BUTTON_DOWN, SDL_BUTTON_LEFT, 10.0f, 20.0f)), true);
    BOOST_REQUIRE_EQUAL(recorder.buttons_.size(), 1u);
    BOOST_CHECK_EQUAL(recorder.buttons_[0].button(), static_cast<unsigned int>(SDL_BUTTON_LEFT));
    BOOST_CHECK_EQUAL(recorder.buttons_[0].pressed(), true);

    // the event carries the point SDL put on it, so a cursor can be driven from the press
    // alone rather than from a motion the consumer tracked itself
    BOOST_CHECK_EQUAL(recorder.buttons_[0].position()[0], 10.0f);
    BOOST_CHECK_EQUAL(recorder.buttons_[0].position()[1], 20.0f);

    // the button is held, and the click moved the cursor with it
    BOOST_CHECK_EQUAL(mouse.state().pressed(SDL_BUTTON_LEFT), true);
    BOOST_CHECK_EQUAL(mouse.state().position()[0], 10.0f);
    BOOST_CHECK_EQUAL(mouse.state().position()[1], 20.0f);

    // and a named source event went out, so a binding config can reach the button
    BOOST_REQUIRE_EQUAL(recorder.source_.size(), 1u);
    BOOST_CHECK_EQUAL(recorder.source_[0].name(), "left");
    BOOST_CHECK(recorder.source_[0].state() == v3d::event::State::Pressed);

    // releasing is the other edge, and drops the hold
    BOOST_CHECK_EQUAL(mouse.handleEvent(buttonEvent(SDL_EVENT_MOUSE_BUTTON_UP, SDL_BUTTON_LEFT, 12.0f, 22.0f)), true);
    BOOST_CHECK_EQUAL(mouse.state().pressed(SDL_BUTTON_LEFT), false);
    BOOST_REQUIRE_EQUAL(recorder.buttons_.size(), 2u);
    BOOST_CHECK_EQUAL(recorder.buttons_[1].position()[0], 12.0f);
    BOOST_CHECK_EQUAL(recorder.buttons_[1].position()[1], 22.0f);
    BOOST_REQUIRE_EQUAL(recorder.source_.size(), 2u);
    BOOST_CHECK(recorder.source_[1].state() == v3d::event::State::Released);

    // the other buttons have names too
    mouse.handleEvent(buttonEvent(SDL_EVENT_MOUSE_BUTTON_DOWN, SDL_BUTTON_RIGHT, 0.0f, 0.0f));
    mouse.handleEvent(buttonEvent(SDL_EVENT_MOUSE_BUTTON_DOWN, SDL_BUTTON_MIDDLE, 0.0f, 0.0f));
    BOOST_REQUIRE_EQUAL(recorder.source_.size(), 4u);
    BOOST_CHECK_EQUAL(recorder.source_[2].name(), "right");
    BOOST_CHECK_EQUAL(recorder.source_[3].name(), "middle");

    // a button with no name is still the mouse's event, but nothing can bind to it
    BOOST_CHECK_EQUAL(mouse.handleEvent(buttonEvent(SDL_EVENT_MOUSE_BUTTON_DOWN, 99, 0.0f, 0.0f)), true);
    BOOST_CHECK_EQUAL(recorder.source_.size(), 4u);
}

BOOST_AUTO_TEST_CASE(mouse_motion_test) {
    boost::shared_ptr<entt::dispatcher> dispatcher = boost::make_shared<entt::dispatcher>();
    boost::shared_ptr<v3d::event::Context> context = boost::make_shared<v3d::event::Context>("mouse");
    v3d::input::Mouse mouse(context, dispatcher);

    Recorder recorder;
    dispatcher->sink<v3d::event::kind::MouseMotion>().connect<&Recorder::motion>(recorder);
    dispatcher->sink<v3d::event::Event>().connect<&Recorder::sourceEvent>(recorder);

    BOOST_CHECK_EQUAL(mouse.handleEvent(motionEvent(3.0f, 0.0f, 3.0f, 0.0f)), true);
    BOOST_REQUIRE_EQUAL(recorder.motion_.size(), 1u);
    BOOST_CHECK_EQUAL(recorder.motion_[0].position()[0], 3.0f);
    BOOST_CHECK_EQUAL(recorder.motion_[0].position()[1], 0.0f);
    BOOST_CHECK_EQUAL(recorder.motion_[0].motion()[0], 3.0f);

    // the cursor position follows
    BOOST_CHECK_EQUAL(mouse.state().position()[0], 3.0f);

    // motion has no discrete name, so it is not a bindable source event
    BOOST_CHECK_EQUAL(recorder.source_.size(), 0u);

    // a key event belongs to the keyboard
    SDL_Event key{};
    key.type = SDL_EVENT_KEY_DOWN;
    BOOST_CHECK_EQUAL(mouse.handleEvent(key), false);
}

BOOST_AUTO_TEST_CASE(mousestate_test) {
    v3d::input::MouseState state;

    // nothing is held, and the cursor starts at the origin
    BOOST_CHECK_EQUAL(state.pressed(SDL_BUTTON_LEFT), false);
    BOOST_CHECK_EQUAL(state.position()[0], 0.0f);
    BOOST_CHECK_EQUAL(state.position()[1], 0.0f);

    // the call operator toggles a button, returning the state it arrived at
    BOOST_CHECK_EQUAL(state(SDL_BUTTON_LEFT), true);
    BOOST_CHECK_EQUAL(state.pressed(SDL_BUTTON_LEFT), true);
    BOOST_CHECK_EQUAL(state.pressed(SDL_BUTTON_RIGHT), false);
    BOOST_CHECK_EQUAL(state(SDL_BUTTON_LEFT), false);
    BOOST_CHECK_EQUAL(state.pressed(SDL_BUTTON_LEFT), false);

    // and moving the cursor hands back where it was
    glm::vec2 previous = state(glm::vec2(5.0f, 7.0f));
    BOOST_CHECK_EQUAL(previous[0], 0.0f);
    BOOST_CHECK_EQUAL(previous[1], 0.0f);
    BOOST_CHECK_EQUAL(state.position()[0], 5.0f);
    BOOST_CHECK_EQUAL(state.position()[1], 7.0f);
    previous = state(glm::vec2(9.0f, 1.0f));
    BOOST_CHECK_EQUAL(previous[0], 5.0f);
    BOOST_CHECK_EQUAL(previous[1], 7.0f);
}
