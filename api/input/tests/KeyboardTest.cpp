/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/event/Source.h>
#include <api/event/kind/KeyDown.h>
#include <api/event/kind/KeyUp.h>
#include <api/input/Keyboard.h>
#include <api/input/Mouse.h>

#include <cstddef>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

/**
 * A Keyboard turns SDL events into dispatcher events, so a test feeds it an SDL_Event and
 * checks what comes out of the dispatcher.
 **/
namespace {
struct Recorder {
    void down(const v3d::event::kind::KeyDown& event) {
        down_.push_back(std::string(event.name()));
    }

    void up(const v3d::event::kind::KeyUp& event) {
        up_.push_back(std::string(event.name()));
    }

    void sourceEvent(const v3d::event::Source& event) {
        source_.push_back(event);
    }

    std::vector<std::string> down_;
    std::vector<std::string> up_;
    std::vector<v3d::event::Event> source_;
};

/** An event type, the key, and whether the press is a held key's repeat. **/
struct Key {
    uint32_t type = SDL_EVENT_KEY_DOWN;
    SDL_Keycode key = SDLK_UNKNOWN;
    bool repeat = false;
};

SDL_Event keyEvent(const Key& spec) {
    SDL_Event event{};
    event.type = spec.type;
    event.key.key = spec.key;
    event.key.repeat = spec.repeat;
    return event;
}

BOOST_AUTO_TEST_CASE(keyboard_test) {
    const boost::shared_ptr<entt::dispatcher> dispatcher = boost::make_shared<entt::dispatcher>();
    const boost::shared_ptr<v3d::event::Context> context = boost::make_shared<v3d::event::Context>("keyboard");
    v3d::input::Keyboard keyboard(context, dispatcher);

    Recorder recorder;
    dispatcher->sink<v3d::event::kind::KeyDown>().connect<&Recorder::down>(recorder);
    dispatcher->sink<v3d::event::kind::KeyUp>().connect<&Recorder::up>(recorder);
    dispatcher->sink<v3d::event::Source>().connect<&Recorder::sourceEvent>(recorder);

    // a key press is a KeyDown, plus a source event any mapper can bind
    BOOST_CHECK_EQUAL(keyboard.handleEvent(keyEvent({.type = SDL_EVENT_KEY_DOWN, .key = SDLK_Q})), true);
    BOOST_REQUIRE_EQUAL(recorder.down_.size(), 1U);
    BOOST_CHECK_EQUAL(recorder.down_[0], "q");
    BOOST_REQUIRE_EQUAL(recorder.source_.size(), 1U);
    BOOST_CHECK_EQUAL(recorder.source_[0].name(), "q");
    BOOST_CHECK_EQUAL(recorder.source_[0].context(), context);
    BOOST_CHECK(recorder.source_[0].state() == v3d::event::State::Pressed);

    // a release is the other edge of the same key
    BOOST_CHECK_EQUAL(keyboard.handleEvent(keyEvent({.type = SDL_EVENT_KEY_UP, .key = SDLK_Q})), true);
    BOOST_REQUIRE_EQUAL(recorder.up_.size(), 1U);
    BOOST_CHECK_EQUAL(recorder.up_[0], "q");
    BOOST_REQUIRE_EQUAL(recorder.source_.size(), 2U);
    BOOST_CHECK(recorder.source_[1].state() == v3d::event::State::Released);

    // the named keys a binding config can reach
    keyboard.handleEvent(keyEvent({.type = SDL_EVENT_KEY_DOWN, .key = SDLK_SPACE}));
    keyboard.handleEvent(keyEvent({.type = SDL_EVENT_KEY_DOWN, .key = SDLK_UP}));
    keyboard.handleEvent(keyEvent({.type = SDL_EVENT_KEY_DOWN, .key = SDLK_ESCAPE}));
    keyboard.handleEvent(keyEvent({.type = SDL_EVENT_KEY_DOWN, .key = SDLK_F1}));
    BOOST_REQUIRE_EQUAL(recorder.down_.size(), 5U);
    BOOST_CHECK_EQUAL(recorder.down_[1], "space");
    BOOST_CHECK_EQUAL(recorder.down_[2], "arrow_up");
    BOOST_CHECK_EQUAL(recorder.down_[3], "escape");
    BOOST_CHECK_EQUAL(recorder.down_[4], "f1");

    // including the punctuation, which a binding config names as the character it types
    keyboard.handleEvent(keyEvent({.type = SDL_EVENT_KEY_DOWN, .key = SDLK_EQUALS}));
    keyboard.handleEvent(keyEvent({.type = SDL_EVENT_KEY_DOWN, .key = SDLK_LEFTBRACKET}));
    keyboard.handleEvent(keyEvent({.type = SDL_EVENT_KEY_DOWN, .key = SDLK_BACKSLASH}));
    BOOST_REQUIRE_EQUAL(recorder.down_.size(), 8U);
    BOOST_CHECK_EQUAL(recorder.down_[5], "=");
    BOOST_CHECK_EQUAL(recorder.down_[6], "[");
    BOOST_CHECK_EQUAL(recorder.down_[7], "\\");

    // a key with no name is still the keyboard's event, but nothing can bind to it
    const std::size_t named = recorder.source_.size();
    BOOST_CHECK_EQUAL(keyboard.handleEvent(keyEvent({.type = SDL_EVENT_KEY_DOWN, .key = SDLK_PRINTSCREEN})), true);
    BOOST_CHECK_EQUAL(recorder.down_.size(), 8U);
    BOOST_CHECK_EQUAL(recorder.source_.size(), named);

    // anything that is not a key event belongs to some other device
    SDL_Event motion{};
    motion.type = SDL_EVENT_MOUSE_MOTION;
    BOOST_CHECK_EQUAL(keyboard.handleEvent(motion), false);
    BOOST_CHECK_EQUAL(recorder.down_.size(), 8U);
}

BOOST_AUTO_TEST_CASE(keystate_test) {
    v3d::input::KeyState state;

    // nothing is held to begin with
    BOOST_CHECK_EQUAL(state.held("w"), false);

    // the call operator toggles, returning the state it arrived at
    BOOST_CHECK_EQUAL(state("w"), true);
    BOOST_CHECK_EQUAL(state.held("w"), true);
    BOOST_CHECK_EQUAL(state.held("s"), false);

    BOOST_CHECK_EQUAL(state("s"), true);
    BOOST_CHECK_EQUAL(state.held("w"), true);
    BOOST_CHECK_EQUAL(state.held("s"), true);

    // and toggling a held key releases it, leaving the others alone
    BOOST_CHECK_EQUAL(state("w"), false);
    BOOST_CHECK_EQUAL(state.held("w"), false);
    BOOST_CHECK_EQUAL(state.held("s"), true);
}

/**
 * A key pressed and released between two flushes was never held when anything checked, but
 * both of its edges are still true for that frame. Polling the held state alone cannot
 * detect it.
 **/
BOOST_AUTO_TEST_CASE(keystate_edge_test) {
    v3d::input::KeyState state;

    BOOST_CHECK_EQUAL(state.pressed("w"), false);
    BOOST_CHECK_EQUAL(state.released("w"), false);

    // going down is an edge and a held key both
    state("w");
    BOOST_CHECK_EQUAL(state.pressed("w"), true);
    BOOST_CHECK_EQUAL(state.released("w"), false);
    BOOST_CHECK_EQUAL(state.held("w"), true);

    // and coming up in the same frame leaves both edges true and nothing held
    state("w");
    BOOST_CHECK_EQUAL(state.pressed("w"), true);
    BOOST_CHECK_EQUAL(state.released("w"), true);
    BOOST_CHECK_EQUAL(state.held("w"), false);

    // the flush ends the frame, and what is held survives it
    state("s");
    state.flush();
    BOOST_CHECK_EQUAL(state.pressed("w"), false);
    BOOST_CHECK_EQUAL(state.released("w"), false);
    BOOST_CHECK_EQUAL(state.pressed("s"), false);
    BOOST_CHECK_EQUAL(state.held("s"), true);
}

/**
 * The device clears its own state when the loop flushes it, so an app reading through
 * Keyboard::state() sees the same frame the events arrived in.
 **/
BOOST_AUTO_TEST_CASE(keyboard_flush_clears_the_edge_it_recorded) {
    const boost::shared_ptr<entt::dispatcher> dispatcher = boost::make_shared<entt::dispatcher>();
    const boost::shared_ptr<v3d::event::Context> context = boost::make_shared<v3d::event::Context>("keyboard");
    v3d::input::Keyboard keyboard(context, dispatcher);

    keyboard.handleEvent(keyEvent({.type = SDL_EVENT_KEY_DOWN, .key = SDLK_Q}));
    BOOST_CHECK_EQUAL(keyboard.state().pressed("q"), true);
    BOOST_CHECK_EQUAL(keyboard.state().held("q"), true);

    keyboard.flush();
    BOOST_CHECK_EQUAL(keyboard.state().pressed("q"), false);
    // still down: a flush ends a frame, it does not release anything
    BOOST_CHECK_EQUAL(keyboard.state().held("q"), true);

    keyboard.handleEvent(keyEvent({.type = SDL_EVENT_KEY_UP, .key = SDLK_Q}));
    BOOST_CHECK_EQUAL(keyboard.state().released("q"), true);
    BOOST_CHECK_EQUAL(keyboard.state().held("q"), false);
}

BOOST_AUTO_TEST_CASE(keyboard_held_key_test) {
    const boost::shared_ptr<entt::dispatcher> dispatcher = boost::make_shared<entt::dispatcher>();
    const boost::shared_ptr<v3d::event::Context> context = boost::make_shared<v3d::event::Context>("keyboard");
    v3d::input::Keyboard keyboard(context, dispatcher);

    Recorder recorder;
    dispatcher->sink<v3d::event::Source>().connect<&Recorder::sourceEvent>(recorder);

    // SDL repeats key down while a key is held. Every repeat is still a press, marked as a
    // repeat, and the release that follows is still a release - the state tracking must not
    // invert on the way through.
    keyboard.handleEvent(keyEvent({.type = SDL_EVENT_KEY_DOWN, .key = SDLK_W}));
    keyboard.handleEvent(keyEvent({.type = SDL_EVENT_KEY_DOWN, .key = SDLK_W, .repeat = true}));
    keyboard.handleEvent(keyEvent({.type = SDL_EVENT_KEY_DOWN, .key = SDLK_W, .repeat = true}));
    keyboard.handleEvent(keyEvent({.type = SDL_EVENT_KEY_UP, .key = SDLK_W}));
    keyboard.handleEvent(keyEvent({.type = SDL_EVENT_KEY_DOWN, .key = SDLK_W}));

    BOOST_REQUIRE_EQUAL(recorder.source_.size(), 5U);
    BOOST_CHECK(recorder.source_[0].state() == v3d::event::State::Pressed);
    BOOST_CHECK(recorder.source_[1].state() == v3d::event::State::Pressed);
    BOOST_CHECK(recorder.source_[2].state() == v3d::event::State::Pressed);
    BOOST_CHECK(recorder.source_[3].state() == v3d::event::State::Released);
    BOOST_CHECK(recorder.source_[4].state() == v3d::event::State::Pressed);
    BOOST_CHECK(!recorder.source_[0].repeat());
    BOOST_CHECK(recorder.source_[1].repeat());
    BOOST_CHECK(recorder.source_[2].repeat());
    BOOST_CHECK(!recorder.source_[4].repeat());
}

/**
 * A key's name reads both ways off one table, so a binding document can be checked against
 * the names a key event will actually carry.
 **/
BOOST_AUTO_TEST_CASE(keyboard_name_table_test) {
    BOOST_TEST(v3d::input::keyName(SDLK_ESCAPE) == "escape");
    BOOST_TEST(v3d::input::isKeyName("escape"));
    BOOST_TEST(v3d::input::isKeyName(v3d::input::keyName(SDLK_A)));
    BOOST_TEST(!v3d::input::isKeyName("escpae"));
    BOOST_TEST(!v3d::input::isKeyName(""));

    BOOST_TEST(v3d::input::isButtonName("left"));
    BOOST_TEST(!v3d::input::isButtonName("a"));
}

};  // namespace
