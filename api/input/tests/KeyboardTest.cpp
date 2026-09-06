/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

#include "../Keyboard.h"
#include "../../event/KeyDown.h"
#include "../../event/KeyUp.h"

/**
 * The device this replaces was v3D::KeyboardDevice, which pushed key names at registered
 * listeners. A Keyboard turns SDL events into dispatcher events instead, so what a test
 * feeds it is an SDL_Event and what it watches for is what comes out of the dispatcher.
 **/
namespace {
struct Recorder {
    void down(const v3d::event::KeyDown& event) {
        down_.push_back(std::string(event.name()));
    }

    void up(const v3d::event::KeyUp& event) {
        up_.push_back(std::string(event.name()));
    }

    void sourceEvent(const v3d::event::Event& event) {
        if (event.type() == v3d::event::Type::Source) {
            source_.push_back(event);
        }
    }

    std::vector<std::string> down_;
    std::vector<std::string> up_;
    std::vector<v3d::event::Event> source_;
};

SDL_Event keyEvent(uint32_t type, SDL_Keycode key) {
    SDL_Event event{};
    event.type = type;
    event.key.key = key;
    return event;
}
};  // namespace

BOOST_AUTO_TEST_CASE(keyboard_test) {
    boost::shared_ptr<entt::dispatcher> dispatcher = boost::make_shared<entt::dispatcher>();
    boost::shared_ptr<v3d::event::Context> context = boost::make_shared<v3d::event::Context>("keyboard");
    v3d::input::Keyboard keyboard(context, dispatcher);

    Recorder recorder;
    dispatcher->sink<v3d::event::KeyDown>().connect<&Recorder::down>(recorder);
    dispatcher->sink<v3d::event::KeyUp>().connect<&Recorder::up>(recorder);
    dispatcher->sink<v3d::event::Event>().connect<&Recorder::sourceEvent>(recorder);

    // a key press is a KeyDown, plus a source event any mapper can bind
    BOOST_CHECK_EQUAL(keyboard.handleEvent(keyEvent(SDL_EVENT_KEY_DOWN, SDLK_Q)), true);
    BOOST_REQUIRE_EQUAL(recorder.down_.size(), 1u);
    BOOST_CHECK_EQUAL(recorder.down_[0], "q");
    BOOST_REQUIRE_EQUAL(recorder.source_.size(), 1u);
    BOOST_CHECK_EQUAL(recorder.source_[0].name(), "q");
    BOOST_CHECK_EQUAL(recorder.source_[0].context(), context);
    BOOST_CHECK(recorder.source_[0].state() == v3d::event::State::Pressed);

    // a release is the other edge of the same key
    BOOST_CHECK_EQUAL(keyboard.handleEvent(keyEvent(SDL_EVENT_KEY_UP, SDLK_Q)), true);
    BOOST_REQUIRE_EQUAL(recorder.up_.size(), 1u);
    BOOST_CHECK_EQUAL(recorder.up_[0], "q");
    BOOST_REQUIRE_EQUAL(recorder.source_.size(), 2u);
    BOOST_CHECK(recorder.source_[1].state() == v3d::event::State::Released);

    // the named keys a binding config can reach
    keyboard.handleEvent(keyEvent(SDL_EVENT_KEY_DOWN, SDLK_SPACE));
    keyboard.handleEvent(keyEvent(SDL_EVENT_KEY_DOWN, SDLK_UP));
    keyboard.handleEvent(keyEvent(SDL_EVENT_KEY_DOWN, SDLK_ESCAPE));
    keyboard.handleEvent(keyEvent(SDL_EVENT_KEY_DOWN, SDLK_F1));
    BOOST_REQUIRE_EQUAL(recorder.down_.size(), 5u);
    BOOST_CHECK_EQUAL(recorder.down_[1], "space");
    BOOST_CHECK_EQUAL(recorder.down_[2], "arrow_up");
    BOOST_CHECK_EQUAL(recorder.down_[3], "escape");
    BOOST_CHECK_EQUAL(recorder.down_[4], "f1");

    // anything that is not a key event belongs to some other device
    SDL_Event motion{};
    motion.type = SDL_EVENT_MOUSE_MOTION;
    BOOST_CHECK_EQUAL(keyboard.handleEvent(motion), false);
    BOOST_CHECK_EQUAL(recorder.down_.size(), 5u);
}

BOOST_AUTO_TEST_CASE(keystate_test) {
    v3d::input::KeyState state;

    // nothing is held to begin with
    BOOST_CHECK_EQUAL(state.pressed("w"), false);

    // the call operator toggles, returning the state it arrived at
    BOOST_CHECK_EQUAL(state("w"), true);
    BOOST_CHECK_EQUAL(state.pressed("w"), true);
    BOOST_CHECK_EQUAL(state.pressed("s"), false);

    BOOST_CHECK_EQUAL(state("s"), true);
    BOOST_CHECK_EQUAL(state.pressed("w"), true);
    BOOST_CHECK_EQUAL(state.pressed("s"), true);

    // and toggling a held key releases it, leaving the others alone
    BOOST_CHECK_EQUAL(state("w"), false);
    BOOST_CHECK_EQUAL(state.pressed("w"), false);
    BOOST_CHECK_EQUAL(state.pressed("s"), true);
}

BOOST_AUTO_TEST_CASE(keyboard_held_key_test) {
    boost::shared_ptr<entt::dispatcher> dispatcher = boost::make_shared<entt::dispatcher>();
    boost::shared_ptr<v3d::event::Context> context = boost::make_shared<v3d::event::Context>("keyboard");
    v3d::input::Keyboard keyboard(context, dispatcher);

    Recorder recorder;
    dispatcher->sink<v3d::event::Event>().connect<&Recorder::sourceEvent>(recorder);

    // SDL repeats key down while a key is held. Every repeat is still a press, and the
    // release that follows is still a release - the state tracking must not invert on the
    // way through.
    keyboard.handleEvent(keyEvent(SDL_EVENT_KEY_DOWN, SDLK_W));
    keyboard.handleEvent(keyEvent(SDL_EVENT_KEY_DOWN, SDLK_W));
    keyboard.handleEvent(keyEvent(SDL_EVENT_KEY_DOWN, SDLK_W));
    keyboard.handleEvent(keyEvent(SDL_EVENT_KEY_UP, SDLK_W));
    keyboard.handleEvent(keyEvent(SDL_EVENT_KEY_DOWN, SDLK_W));

    BOOST_REQUIRE_EQUAL(recorder.source_.size(), 5u);
    BOOST_CHECK(recorder.source_[0].state() == v3d::event::State::Pressed);
    BOOST_CHECK(recorder.source_[1].state() == v3d::event::State::Pressed);
    BOOST_CHECK(recorder.source_[2].state() == v3d::event::State::Pressed);
    BOOST_CHECK(recorder.source_[3].state() == v3d::event::State::Released);
    BOOST_CHECK(recorder.source_[4].state() == v3d::event::State::Pressed);
}
