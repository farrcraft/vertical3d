/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/event/Source.h>
#include <api/event/kind/MouseButton.h>
#include <api/event/kind/MouseMotion.h>
#include <api/event/kind/MouseWheel.h>
#include <api/input/Mouse.h>

#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

/**
 * A Mouse turns SDL events into dispatcher events and MouseState, and handleEvent claims
 * only the events that belong to the mouse. A test feeds it an SDL_Event and checks what
 * comes out of the dispatcher.
 **/
namespace {
struct Recorder {
    void button(const v3d::event::kind::MouseButton& event) {
        buttons_.push_back(event);
    }

    void motion(const v3d::event::kind::MouseMotion& event) {
        motion_.push_back(event);
    }

    void wheel(const v3d::event::kind::MouseWheel& event) {
        wheel_.push_back(event);
    }

    void sourceEvent(const v3d::event::Source& event) {
        source_.push_back(event);
    }

    std::vector<v3d::event::kind::MouseButton> buttons_;
    std::vector<v3d::event::kind::MouseMotion> motion_;
    std::vector<v3d::event::kind::MouseWheel> wheel_;
    std::vector<v3d::event::Event> source_;
};

/** An event type, the button, and where the cursor was. **/
struct Click {
    uint32_t type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    uint8_t button = SDL_BUTTON_LEFT;
    float x = 0.0F;
    float y = 0.0F;
};

SDL_Event buttonEvent(const Click& spec) {
    SDL_Event event{};
    event.type = spec.type;
    event.button.button = spec.button;
    event.button.x = spec.x;
    event.button.y = spec.y;
    return event;
}

/** How far the wheel turned, and where the cursor was. **/
struct Wheel {
    float notches = 0.0F;
    float x = 0.0F;
    float y = 0.0F;
};

SDL_Event wheelEvent(const Wheel& spec) {
    SDL_Event event{};
    event.type = SDL_EVENT_MOUSE_WHEEL;
    event.wheel.x = 0.0F;
    event.wheel.y = spec.notches;
    event.wheel.mouse_x = spec.x;
    event.wheel.mouse_y = spec.y;
    return event;
}

/** Where the cursor is, and how far it moved. **/
struct Motion {
    float x = 0.0F;
    float y = 0.0F;
    float dx = 0.0F;
    float dy = 0.0F;
};

SDL_Event motionEvent(const Motion& spec) {
    SDL_Event event{};
    event.type = SDL_EVENT_MOUSE_MOTION;
    event.motion.x = spec.x;
    event.motion.y = spec.y;
    event.motion.xrel = spec.dx;
    event.motion.yrel = spec.dy;
    return event;
}

BOOST_AUTO_TEST_CASE(mouse_button_test) {
    const boost::shared_ptr<entt::dispatcher> dispatcher = boost::make_shared<entt::dispatcher>();
    const boost::shared_ptr<v3d::event::Context> context = boost::make_shared<v3d::event::Context>("mouse");
    v3d::input::Mouse mouse(context, dispatcher);

    Recorder recorder;
    dispatcher->sink<v3d::event::kind::MouseButton>().connect<&Recorder::button>(recorder);
    dispatcher->sink<v3d::event::Source>().connect<&Recorder::sourceEvent>(recorder);

    BOOST_CHECK_EQUAL(mouse.handleEvent(buttonEvent({.type = SDL_EVENT_MOUSE_BUTTON_DOWN, .button = SDL_BUTTON_LEFT, .x = 10.0F, .y = 20.0F})), true);
    BOOST_REQUIRE_EQUAL(recorder.buttons_.size(), 1U);
    BOOST_CHECK_EQUAL(recorder.buttons_[0].button(), "left");
    BOOST_CHECK_EQUAL(recorder.buttons_[0].pressed(), true);

    // the event carries the point SDL put on it, so a cursor can be driven from the press
    // alone rather than from a motion the consumer tracked itself
    BOOST_CHECK_EQUAL(recorder.buttons_[0].position()[0], 10.0F);
    BOOST_CHECK_EQUAL(recorder.buttons_[0].position()[1], 20.0F);

    // the button is held, and the click moved the cursor with it
    BOOST_CHECK_EQUAL(mouse.state().held("left"), true);
    BOOST_CHECK_EQUAL(mouse.state().position()[0], 10.0F);
    BOOST_CHECK_EQUAL(mouse.state().position()[1], 20.0F);

    // and a named source event went out, so a binding config can reach the button
    BOOST_REQUIRE_EQUAL(recorder.source_.size(), 1U);
    BOOST_CHECK_EQUAL(recorder.source_[0].name(), "left");
    BOOST_CHECK(recorder.source_[0].state() == v3d::event::State::Pressed);

    // releasing is the other edge, and drops the hold
    BOOST_CHECK_EQUAL(mouse.handleEvent(buttonEvent({.type = SDL_EVENT_MOUSE_BUTTON_UP, .button = SDL_BUTTON_LEFT, .x = 12.0F, .y = 22.0F})), true);
    BOOST_CHECK_EQUAL(mouse.state().held("left"), false);
    BOOST_REQUIRE_EQUAL(recorder.buttons_.size(), 2U);
    BOOST_CHECK_EQUAL(recorder.buttons_[1].position()[0], 12.0F);
    BOOST_CHECK_EQUAL(recorder.buttons_[1].position()[1], 22.0F);
    BOOST_REQUIRE_EQUAL(recorder.source_.size(), 2U);
    BOOST_CHECK(recorder.source_[1].state() == v3d::event::State::Released);

    // the other buttons have names too
    mouse.handleEvent(buttonEvent({.type = SDL_EVENT_MOUSE_BUTTON_DOWN, .button = SDL_BUTTON_RIGHT, .x = 0.0F, .y = 0.0F}));
    mouse.handleEvent(buttonEvent({.type = SDL_EVENT_MOUSE_BUTTON_DOWN, .button = SDL_BUTTON_MIDDLE, .x = 0.0F, .y = 0.0F}));
    BOOST_REQUIRE_EQUAL(recorder.source_.size(), 4U);
    BOOST_CHECK_EQUAL(recorder.source_[2].name(), "right");
    BOOST_CHECK_EQUAL(recorder.source_[3].name(), "middle");

    // a button with no name is still the mouse's event, but nothing can bind to it
    BOOST_CHECK_EQUAL(mouse.handleEvent(buttonEvent({.type = SDL_EVENT_MOUSE_BUTTON_DOWN, .button = 99, .x = 0.0F, .y = 0.0F})), true);
    BOOST_CHECK_EQUAL(recorder.source_.size(), 4U);
}

BOOST_AUTO_TEST_CASE(mouse_motion_test) {
    const boost::shared_ptr<entt::dispatcher> dispatcher = boost::make_shared<entt::dispatcher>();
    const boost::shared_ptr<v3d::event::Context> context = boost::make_shared<v3d::event::Context>("mouse");
    v3d::input::Mouse mouse(context, dispatcher);

    Recorder recorder;
    dispatcher->sink<v3d::event::kind::MouseMotion>().connect<&Recorder::motion>(recorder);
    dispatcher->sink<v3d::event::Source>().connect<&Recorder::sourceEvent>(recorder);

    BOOST_CHECK_EQUAL(mouse.handleEvent(motionEvent({.x = 3.0F, .y = 0.0F, .dx = 3.0F, .dy = 0.0F})), true);
    BOOST_REQUIRE_EQUAL(recorder.motion_.size(), 1U);
    BOOST_CHECK_EQUAL(recorder.motion_[0].position()[0], 3.0F);
    BOOST_CHECK_EQUAL(recorder.motion_[0].position()[1], 0.0F);
    BOOST_CHECK_EQUAL(recorder.motion_[0].motion()[0], 3.0F);

    // the cursor position follows
    BOOST_CHECK_EQUAL(mouse.state().position()[0], 3.0F);

    // motion has no discrete name, so it is not a bindable source event
    BOOST_CHECK_EQUAL(recorder.source_.size(), 0U);

    // a key event belongs to the keyboard
    SDL_Event key{};
    key.type = SDL_EVENT_KEY_DOWN;
    BOOST_CHECK_EQUAL(mouse.handleEvent(key), false);
}

/**
 * A click and its release inside one frame are both edges of one button, and a flush ends
 * the frame without releasing what is still down or moving the cursor.
 **/
BOOST_AUTO_TEST_CASE(mousestate_edge_test) {
    v3d::input::MouseState state;

    BOOST_CHECK_EQUAL(state.pressed("left"), false);
    BOOST_CHECK_EQUAL(state.released("left"), false);

    state("left");
    BOOST_CHECK_EQUAL(state.pressed("left"), true);
    BOOST_CHECK_EQUAL(state.held("left"), true);

    state("left");
    BOOST_CHECK_EQUAL(state.pressed("left"), true);
    BOOST_CHECK_EQUAL(state.released("left"), true);
    BOOST_CHECK_EQUAL(state.held("left"), false);

    state("right");
    state(glm::vec2(4.0F, 2.0F));
    state.flush();
    BOOST_CHECK_EQUAL(state.pressed("left"), false);
    BOOST_CHECK_EQUAL(state.released("left"), false);
    BOOST_CHECK_EQUAL(state.pressed("right"), false);
    // what is held and where the cursor is are current state, not edges of the frame
    BOOST_CHECK_EQUAL(state.held("right"), true);
    BOOST_CHECK_EQUAL(state.position()[0], 4.0F);
    BOOST_CHECK_EQUAL(state.position()[1], 2.0F);
}

BOOST_AUTO_TEST_CASE(mousestate_test) {
    v3d::input::MouseState state;

    // nothing is held, and the cursor starts at the origin
    BOOST_CHECK_EQUAL(state.held("left"), false);
    BOOST_CHECK_EQUAL(state.position()[0], 0.0F);
    BOOST_CHECK_EQUAL(state.position()[1], 0.0F);

    // the call operator toggles a button, returning the state it arrived at
    BOOST_CHECK_EQUAL(state("left"), true);
    BOOST_CHECK_EQUAL(state.held("left"), true);
    BOOST_CHECK_EQUAL(state.held("right"), false);
    BOOST_CHECK_EQUAL(state("left"), false);
    BOOST_CHECK_EQUAL(state.held("left"), false);

    // and moving the cursor returns where it was
    glm::vec2 previous = state(glm::vec2(5.0F, 7.0F));
    BOOST_CHECK_EQUAL(previous[0], 0.0F);
    BOOST_CHECK_EQUAL(previous[1], 0.0F);
    BOOST_CHECK_EQUAL(state.position()[0], 5.0F);
    BOOST_CHECK_EQUAL(state.position()[1], 7.0F);
    previous = state(glm::vec2(9.0F, 1.0F));
    BOOST_CHECK_EQUAL(previous[0], 5.0F);
    BOOST_CHECK_EQUAL(previous[1], 7.0F);
}

/**
 * A wheel is an edge and not a position, because a wheel has no position. The notches a
 * frame saw are accumulated and cleared with the button edges.
 **/
BOOST_AUTO_TEST_CASE(mouse_wheel_test) {
    const boost::shared_ptr<entt::dispatcher> dispatcher = boost::make_shared<entt::dispatcher>();
    const boost::shared_ptr<v3d::event::Context> context = boost::make_shared<v3d::event::Context>("mouse");
    v3d::input::Mouse mouse(context, dispatcher);

    Recorder recorder;
    dispatcher->sink<v3d::event::kind::MouseWheel>().connect<&Recorder::wheel>(recorder);
    dispatcher->sink<v3d::event::Source>().connect<&Recorder::sourceEvent>(recorder);

    BOOST_CHECK_EQUAL(mouse.handleEvent(wheelEvent({.notches = 1.0F, .x = 40.0F, .y = 50.0F})), true);
    BOOST_REQUIRE_EQUAL(recorder.wheel_.size(), 1U);
    BOOST_CHECK_EQUAL(recorder.wheel_[0].notches()[1], 1.0F);

    // the event carries where the cursor was, which identifies what was scrolled
    BOOST_CHECK_EQUAL(recorder.wheel_[0].position()[0], 40.0F);
    BOOST_CHECK_EQUAL(recorder.wheel_[0].position()[1], 50.0F);

    // a wheel sends one event per notch, so a flick that turned three reads as three
    mouse.handleEvent(wheelEvent({.notches = 1.0F, .x = 40.0F, .y = 50.0F}));
    mouse.handleEvent(wheelEvent({.notches = 1.0F, .x = 40.0F, .y = 50.0F}));
    BOOST_CHECK_EQUAL(mouse.state().wheel(), 3.0F);

    // turning it back subtracts, so a flick each way inside one frame comes to nothing
    mouse.handleEvent(wheelEvent({.notches = -3.0F, .x = 40.0F, .y = 50.0F}));
    BOOST_CHECK_EQUAL(mouse.state().wheel(), 0.0F);

    // a turn has no discrete name, so nothing binds to one
    BOOST_CHECK_EQUAL(recorder.source_.size(), 0U);

    // a frame that reads it late reads nothing, the way a button edge does
    mouse.handleEvent(wheelEvent({.notches = 2.0F, .x = 40.0F, .y = 50.0F}));
    BOOST_CHECK_EQUAL(mouse.state().wheel(), 2.0F);
    mouse.flush();
    BOOST_CHECK_EQUAL(mouse.state().wheel(), 0.0F);
}

};  // namespace
