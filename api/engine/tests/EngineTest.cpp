/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

#include "../Engine.h"
#include "../Feature.h"

namespace {

/**
 * The engine keeps what it built where its app subclass can reach it, so a test reads the
 * mappings it registered the way a Controller does.
 **/
class TestEngine final : public v3d::engine::Engine {
 public:
    using Engine::Engine;

    const boost::shared_ptr<entt::dispatcher>& dispatcher() const {
        return dispatcher_;
    }

    const boost::shared_ptr<v3d::event::Engine>& events() const {
        return eventEngine_;
    }

    const boost::shared_ptr<v3d::config::Config>& config() const {
        return config_;
    }

    const boost::shared_ptr<v3d::asset::Manager>& assets() const {
        return assetManager_;
    }

    /**
     * route() is where the order of ADR-0043 lives, and eventLoop() renders, so a test
     * drives the one and never the other.
     **/
    void offer(const SDL_Event& event) {
        route(event);
    }

    bool onEvent(const SDL_Event& event) override {
        offered_.push_back(event.type);
        return take_;
    }

    bool take_ = false;
    std::vector<Uint32> offered_;
};

/**
 * Collects the destination events a mapping produced.
 **/
struct Recorder {
    void handle(const v3d::event::Event& event) {
        if (event.type() == v3d::event::Type::Destination) {
            events_.push_back(event);
        }
    }

    std::vector<v3d::event::Event> events_;
};

v3d::event::Event source(const boost::shared_ptr<v3d::event::Context>& context,
    const std::string& name, v3d::event::State state) {
    v3d::event::Event event(name, context);
    event.type(v3d::event::Type::Source);
    event.state(state);
    return event;
}

/**
 * An app path rather than a data path: initialize() appends "data/" to it, so a fixture
 * carries the directory the manager ends up resolving against one level down.
 **/
std::string appPath(const std::string& fixture) {
    return "fixtures/" + fixture + "/";
}

const int configFeature = static_cast<int>(v3d::engine::Feature::Config);
const int boundFeature = configFeature | static_cast<int>(v3d::engine::Feature::KeyboardInput);

/**
 * A key going down, as SDL delivers it - the only event in this file the input devices
 * have anything to say about.
 **/
SDL_Event keyDown(SDL_Keycode key) {
    SDL_Event event{};
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.key = key;
    return event;
}

};  // namespace

/**
 * A mask of nothing builds the asset manager, the dispatcher and the event engine and stops
 * there - no config read, no input devices and no window, which is what makes the engine
 * testable without one.
 **/
BOOST_AUTO_TEST_CASE(engine_initialize_no_features_test) {
    TestEngine engine(appPath("good"));

    BOOST_TEST(engine.initialize(0));
    BOOST_TEST(static_cast<bool>(engine.assets()));
    BOOST_TEST(static_cast<bool>(engine.dispatcher()));
    BOOST_TEST(static_cast<bool>(engine.events()));
    BOOST_TEST(!engine.config());
    BOOST_TEST(!engine.window());
}

/**
 * Feature::Config reads config.json out of the app's data directory and files what it names.
 **/
BOOST_AUTO_TEST_CASE(engine_initialize_config_test) {
    TestEngine engine(appPath("good"));

    BOOST_TEST(engine.initialize(configFeature));
    BOOST_REQUIRE(engine.config());
    BOOST_TEST(static_cast<bool>(engine.config()->get(v3d::config::Type::Binding)));
    BOOST_TEST(static_cast<bool>(engine.config()->get(v3d::config::Type::Window)));
    BOOST_TEST(!engine.config()->get(v3d::config::Type::Sound));
}

/**
 * The mappings the config named are registered as one global mapper, so a source event
 * triggered on the dispatcher comes back out as the destination it was bound to.
 **/
BOOST_AUTO_TEST_CASE(engine_registers_mappings_test) {
    TestEngine engine(appPath("good"));
    BOOST_REQUIRE(engine.initialize(configFeature));

    Recorder recorder;
    engine.dispatcher()->sink<v3d::event::Event>().connect<&Recorder::handle>(recorder);

    boost::shared_ptr<v3d::event::Context> keyboard = engine.events()->resolveContext("keyboard");
    engine.dispatcher()->trigger(source(keyboard, "w", v3d::event::State::Pressed));

    BOOST_REQUIRE_EQUAL(recorder.events_.size(), 1u);
    BOOST_CHECK_EQUAL(recorder.events_[0].name(), "leftPaddleUp");
    BOOST_CHECK_EQUAL(recorder.events_[0].context()->name(), "pong");

    // an unbound key produces nothing
    engine.dispatcher()->trigger(source(keyboard, "q", v3d::event::State::Pressed));
    BOOST_CHECK_EQUAL(recorder.events_.size(), 1u);
}

/**
 * A binding naming a state binds that edge only; one naming none matches both.
 **/
BOOST_AUTO_TEST_CASE(engine_mapping_state_test) {
    TestEngine engine(appPath("good"));
    BOOST_REQUIRE(engine.initialize(configFeature));

    Recorder recorder;
    engine.dispatcher()->sink<v3d::event::Event>().connect<&Recorder::handle>(recorder);

    boost::shared_ptr<v3d::event::Context> keyboard = engine.events()->resolveContext("keyboard");

    engine.dispatcher()->trigger(source(keyboard, "escape", v3d::event::State::Pressed));
    BOOST_REQUIRE_EQUAL(recorder.events_.size(), 1u);
    BOOST_CHECK_EQUAL(recorder.events_[0].name(), "quit");

    engine.dispatcher()->trigger(source(keyboard, "escape", v3d::event::State::Released));
    BOOST_CHECK_EQUAL(recorder.events_.size(), 1u);

    // the unstated binding takes both edges
    engine.dispatcher()->trigger(source(keyboard, "w", v3d::event::State::Released));
    BOOST_CHECK_EQUAL(recorder.events_.size(), 2u);
}

/**
 * A destination's param reaches the handler as the event's data, in the type the document
 * wrote it as - which is what lets one action serve several bindings.
 **/
BOOST_AUTO_TEST_CASE(engine_mapping_param_test) {
    TestEngine engine(appPath("good"));
    BOOST_REQUIRE(engine.initialize(configFeature));

    Recorder recorder;
    engine.dispatcher()->sink<v3d::event::Event>().connect<&Recorder::handle>(recorder);

    boost::shared_ptr<v3d::event::Context> keyboard = engine.events()->resolveContext("keyboard");

    engine.dispatcher()->trigger(source(keyboard, "1", v3d::event::State::Pressed));
    BOOST_REQUIRE_EQUAL(recorder.events_.size(), 1u);
    BOOST_REQUIRE(recorder.events_[0].data());
    BOOST_CHECK_EQUAL(std::get<std::string>(recorder.events_[0].data().get()), "cube");

    engine.dispatcher()->trigger(source(keyboard, "2", v3d::event::State::Pressed));
    BOOST_REQUIRE_EQUAL(recorder.events_.size(), 2u);
    BOOST_REQUIRE(recorder.events_[1].data());
    BOOST_CHECK_EQUAL(std::get<int>(recorder.events_[1].data().get()), 3);

    engine.dispatcher()->trigger(source(keyboard, "g", v3d::event::State::Pressed));
    BOOST_REQUIRE_EQUAL(recorder.events_.size(), 3u);
    BOOST_REQUIRE(recorder.events_[2].data());
    BOOST_CHECK(std::get<bool>(recorder.events_[2].data().get()));
}

/**
 * The app is offered every event before the bindings are, per ADR-0043, and what it
 * declines goes on to be mapped exactly as it was before there was anywhere else for it
 * to go.
 **/
BOOST_AUTO_TEST_CASE(engine_declined_event_reaches_the_bindings_test) {
    TestEngine engine(appPath("good"));
    BOOST_REQUIRE(engine.initialize(boundFeature));

    Recorder recorder;
    engine.dispatcher()->sink<v3d::event::Event>().connect<&Recorder::handle>(recorder);

    engine.offer(keyDown(SDLK_W));

    BOOST_REQUIRE_EQUAL(engine.offered_.size(), 1u);
    BOOST_REQUIRE_EQUAL(recorder.events_.size(), 1u);
    BOOST_CHECK_EQUAL(recorder.events_[0].name(), "leftPaddleUp");
}

/**
 * And what it takes stops there. This is the whole point of the seam: a click that both
 * presses a button the app drew and gives an order is the bug it exists to prevent.
 **/
BOOST_AUTO_TEST_CASE(engine_taken_event_is_not_mapped_test) {
    TestEngine engine(appPath("good"));
    BOOST_REQUIRE(engine.initialize(boundFeature));
    engine.take_ = true;

    Recorder recorder;
    engine.dispatcher()->sink<v3d::event::Event>().connect<&Recorder::handle>(recorder);

    engine.offer(keyDown(SDLK_W));

    BOOST_CHECK_EQUAL(engine.offered_.size(), 1u);
    BOOST_CHECK_EQUAL(recorder.events_.size(), 0u);
}

/**
 * A close request is a window fact rather than input, so it is not an app's to decline -
 * an app that could swallow one would be a window that could not be closed.
 **/
BOOST_AUTO_TEST_CASE(engine_quit_survives_a_taken_event_test) {
    SDL_Event quit{};
    quit.type = SDL_EVENT_QUIT;

    TestEngine declining(appPath("good"));
    BOOST_REQUIRE(declining.initialize(boundFeature));
    declining.offer(quit);
    BOOST_CHECK(declining.quitting());

    TestEngine taking(appPath("good"));
    BOOST_REQUIRE(taking.initialize(boundFeature));
    taking.take_ = true;
    taking.offer(quit);
    BOOST_CHECK(taking.quitting());
}

/**
 * The default takes nothing, which is what makes the seam additive: the four apps in this
 * tree do not override it and see the events they always saw.
 **/
BOOST_AUTO_TEST_CASE(engine_default_takes_no_event_test) {
    v3d::engine::Engine engine(appPath("good"));
    SDL_Event event{};
    event.type = SDL_EVENT_KEY_DOWN;
    BOOST_CHECK(!engine.onEvent(event));
}

/**
 * Every rejection below is a false return out of initialize rather than an exception, because
 * a malformed document is what an app ships and a throw out of startup says nothing about
 * which line of it was wrong.
 **/
BOOST_AUTO_TEST_CASE(engine_missing_config_document_test) {
    TestEngine engine(appPath("nowhere"));
    BOOST_TEST(!engine.initialize(configFeature));
}

BOOST_AUTO_TEST_CASE(engine_unloadable_config_file_test) {
    TestEngine engine(appPath("unloadable-config"));
    BOOST_TEST(!engine.initialize(configFeature));
}

BOOST_AUTO_TEST_CASE(engine_no_mappings_key_test) {
    TestEngine engine(appPath("no-mappings-key"));
    BOOST_TEST(!engine.initialize(configFeature));
    // the document itself loaded - it is the mapping walk that rejected it
    BOOST_REQUIRE(engine.config());
    BOOST_TEST(static_cast<bool>(engine.config()->get(v3d::config::Type::Binding)));
}

BOOST_AUTO_TEST_CASE(engine_mapping_not_an_object_test) {
    TestEngine engine(appPath("mapping-not-object"));
    BOOST_TEST(!engine.initialize(configFeature));
    // the document itself loaded - it is the mapping walk that rejected it
    BOOST_REQUIRE(engine.config());
    BOOST_TEST(static_cast<bool>(engine.config()->get(v3d::config::Type::Binding)));
}

BOOST_AUTO_TEST_CASE(engine_mapping_missing_source_test) {
    TestEngine engine(appPath("missing-source"));
    BOOST_TEST(!engine.initialize(configFeature));
    // the document itself loaded - it is the mapping walk that rejected it
    BOOST_REQUIRE(engine.config());
    BOOST_TEST(static_cast<bool>(engine.config()->get(v3d::config::Type::Binding)));
}

BOOST_AUTO_TEST_CASE(engine_mapping_missing_destination_test) {
    TestEngine engine(appPath("missing-destination"));
    BOOST_TEST(!engine.initialize(configFeature));
    // the document itself loaded - it is the mapping walk that rejected it
    BOOST_REQUIRE(engine.config());
    BOOST_TEST(static_cast<bool>(engine.config()->get(v3d::config::Type::Binding)));
}

BOOST_AUTO_TEST_CASE(engine_mapping_unsupported_param_test) {
    TestEngine engine(appPath("bad-param"));
    BOOST_TEST(!engine.initialize(configFeature));
    // the document itself loaded - it is the mapping walk that rejected it
    BOOST_REQUIRE(engine.config());
    BOOST_TEST(static_cast<bool>(engine.config()->get(v3d::config::Type::Binding)));
}

/**
 * quit() is what a command handler calls, and the loop reads it after the handler returns.
 * shutdown() is not: it tears down the window that the frame after the handler would draw
 * into.
 **/
BOOST_AUTO_TEST_CASE(engine_quit_test) {
    TestEngine engine(appPath("good"));
    BOOST_REQUIRE(engine.initialize(0));

    BOOST_TEST(!engine.quitting());
    engine.quit();
    BOOST_TEST(engine.quitting());

    // asking twice is asking once
    engine.quit();
    BOOST_TEST(engine.quitting());
}

/**
 * An engine that never reached the window has nothing to tear down, so an app that fails in
 * initialize can still call shutdown once from main.
 **/
BOOST_AUTO_TEST_CASE(engine_shutdown_without_window_test) {
    TestEngine engine(appPath("good"));
    BOOST_REQUIRE(engine.initialize(0));

    BOOST_TEST(engine.shutdown());
}

/**
 * The base tick and render do nothing and succeed - an app overrides both, and the loop reads
 * the return to stop.
 **/
BOOST_AUTO_TEST_CASE(engine_base_tick_and_render_test) {
    TestEngine engine(appPath("good"));

    BOOST_TEST(engine.tick(16));
    BOOST_TEST(engine.render());
}
