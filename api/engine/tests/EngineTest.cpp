/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/event/Source.h>
#include <api/engine/Application.h>
#include <api/engine/Engine.h>
#include <api/engine/Feature.h>

#include <cstdlib>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

namespace {

/**
 * The engine keeps what it built where its app subclass can reach it, so a test reads the
 * mappings it registered the way a Controller does.
 **/
class TestEngine final : public v3d::engine::Engine {
 public:
    explicit TestEngine(const std::string& path, v3d::engine::Features features = v3d::engine::Features()) :
        Engine(path),
        features_(features) {
    }

    /**
     * route() offers an event to the app, the input devices and the engine, in that order.
     * eventLoop() renders, so a test drives route() and never eventLoop().
     **/
    void offer(const SDL_Event& event) {
        route(event);
    }

    using Engine::rebind;

    bool onEvent(const SDL_Event& event) override {
        offered_.push_back(event.type);
        return take_;
    }

    bool take_ = false;
    std::vector<Uint32> offered_;

 protected:
    v3d::engine::Features features() const override {
        return features_;
    }

 private:
    v3d::engine::Features features_;
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

v3d::event::Source source(const boost::shared_ptr<v3d::event::Context>& context,
    const std::string& name, v3d::event::State state) {
    return v3d::event::Source(name, context, state);
}

/**
 * An app path rather than a data path: initialize() appends "data/" to it, so a fixture
 * carries the directory the manager ends up resolving against one level down.
 **/
std::string appPath(const std::string& fixture) {
    return "fixtures/" + fixture + "/";
}

const v3d::engine::Features configFeature = v3d::engine::Feature::Config;
const v3d::engine::Features boundFeature = v3d::engine::Feature::Config | v3d::engine::Feature::KeyboardInput;

/**
 * A key going down, as SDL delivers it. It is the only event in this file that the input
 * devices handle.
 **/
SDL_Event keyDown(SDL_Keycode key) {
    SDL_Event event{};
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.key = key;
    return event;
}

SDL_Event keyUp(SDL_Keycode key) {
    SDL_Event event{};
    event.type = SDL_EVENT_KEY_UP;
    event.key.key = key;
    return event;
}

};  // namespace

/**
 * A mask of nothing builds the asset manager, the dispatcher and the event engine and stops
 * there: no config read, no input devices and no window, so the engine can be tested
 * without a window.
 **/
BOOST_AUTO_TEST_CASE(engine_initialize_no_features_test) {
    TestEngine engine(appPath("good"));

    BOOST_TEST(engine.initialize());
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
    TestEngine engine(appPath("good"), configFeature);

    BOOST_TEST(engine.initialize());
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
    TestEngine engine(appPath("good"), configFeature);
    BOOST_REQUIRE(engine.initialize());

    Recorder recorder;
    engine.dispatcher()->sink<v3d::event::Event>().connect<&Recorder::handle>(recorder);

    boost::shared_ptr<v3d::event::Context> keyboard = engine.events()->resolveContext("keyboard");
    v3d::event::publish(*engine.dispatcher(), source(keyboard, "w", v3d::event::State::Pressed));

    BOOST_REQUIRE_EQUAL(recorder.events_.size(), 1u);
    BOOST_CHECK_EQUAL(recorder.events_[0].name(), "leftPaddleUp");
    BOOST_CHECK_EQUAL(recorder.events_[0].context()->name(), "pong");

    // an unbound key produces nothing
    v3d::event::publish(*engine.dispatcher(), source(keyboard, "q", v3d::event::State::Pressed));
    BOOST_CHECK_EQUAL(recorder.events_.size(), 1u);
}

/**
 * A binding naming a state binds that edge only; one naming none matches both.
 **/
BOOST_AUTO_TEST_CASE(engine_mapping_state_test) {
    TestEngine engine(appPath("good"), configFeature);
    BOOST_REQUIRE(engine.initialize());

    Recorder recorder;
    engine.dispatcher()->sink<v3d::event::Event>().connect<&Recorder::handle>(recorder);

    boost::shared_ptr<v3d::event::Context> keyboard = engine.events()->resolveContext("keyboard");

    v3d::event::publish(*engine.dispatcher(), source(keyboard, "escape", v3d::event::State::Pressed));
    BOOST_REQUIRE_EQUAL(recorder.events_.size(), 1u);
    BOOST_CHECK_EQUAL(recorder.events_[0].name(), "quit");

    v3d::event::publish(*engine.dispatcher(), source(keyboard, "escape", v3d::event::State::Released));
    BOOST_CHECK_EQUAL(recorder.events_.size(), 1u);

    // the unstated binding takes both edges
    v3d::event::publish(*engine.dispatcher(), source(keyboard, "w", v3d::event::State::Released));
    BOOST_CHECK_EQUAL(recorder.events_.size(), 2u);
}

/**
 * A destination's param reaches the handler as the event's data, in the type the document
 * wrote it as, so one action can serve several bindings.
 **/
BOOST_AUTO_TEST_CASE(engine_mapping_param_test) {
    TestEngine engine(appPath("good"), configFeature);
    BOOST_REQUIRE(engine.initialize());

    Recorder recorder;
    engine.dispatcher()->sink<v3d::event::Event>().connect<&Recorder::handle>(recorder);

    boost::shared_ptr<v3d::event::Context> keyboard = engine.events()->resolveContext("keyboard");

    v3d::event::publish(*engine.dispatcher(), source(keyboard, "1", v3d::event::State::Pressed));
    BOOST_REQUIRE_EQUAL(recorder.events_.size(), 1u);
    BOOST_REQUIRE(recorder.events_[0].data());
    BOOST_CHECK_EQUAL(std::get<std::string>(recorder.events_[0].data().get()), "cube");

    v3d::event::publish(*engine.dispatcher(), source(keyboard, "2", v3d::event::State::Pressed));
    BOOST_REQUIRE_EQUAL(recorder.events_.size(), 2u);
    BOOST_REQUIRE(recorder.events_[1].data());
    BOOST_CHECK_EQUAL(std::get<int>(recorder.events_[1].data().get()), 3);

    v3d::event::publish(*engine.dispatcher(), source(keyboard, "g", v3d::event::State::Pressed));
    BOOST_REQUIRE_EQUAL(recorder.events_.size(), 3u);
    BOOST_REQUIRE(recorder.events_[2].data());
    BOOST_CHECK(std::get<bool>(recorder.events_[2].data().get()));
}

/**
 * The app is offered every event before the bindings are, and an event it does not consume
 * is mapped as usual.
 **/
BOOST_AUTO_TEST_CASE(engine_declined_event_reaches_the_bindings_test) {
    TestEngine engine(appPath("good"), boundFeature);
    BOOST_REQUIRE(engine.initialize());

    Recorder recorder;
    engine.dispatcher()->sink<v3d::event::Event>().connect<&Recorder::handle>(recorder);

    engine.offer(keyDown(SDLK_W));

    BOOST_REQUIRE_EQUAL(engine.offered_.size(), 1u);
    BOOST_REQUIRE_EQUAL(recorder.events_.size(), 1u);
    BOOST_CHECK_EQUAL(recorder.events_[0].name(), "leftPaddleUp");
}

/**
 * An event the app consumes goes no further, so one click cannot both press a button the
 * app drew and issue an order.
 **/
BOOST_AUTO_TEST_CASE(engine_taken_event_is_not_mapped_test) {
    TestEngine engine(appPath("good"), boundFeature);
    BOOST_REQUIRE(engine.initialize());
    engine.take_ = true;

    Recorder recorder;
    engine.dispatcher()->sink<v3d::event::Event>().connect<&Recorder::handle>(recorder);

    engine.offer(keyDown(SDLK_W));

    BOOST_CHECK_EQUAL(engine.offered_.size(), 1u);
    BOOST_CHECK_EQUAL(recorder.events_.size(), 0u);
}

/**
 * A close request is handled even when the app consumes it, so the window can always be
 * closed.
 **/
BOOST_AUTO_TEST_CASE(engine_quit_survives_a_taken_event_test) {
    SDL_Event quit{};
    quit.type = SDL_EVENT_QUIT;

    TestEngine declining(appPath("good"), boundFeature);
    BOOST_REQUIRE(declining.initialize());
    declining.offer(quit);
    BOOST_CHECK(declining.quitting());

    TestEngine taking(appPath("good"), boundFeature);
    BOOST_REQUIRE(taking.initialize());
    taking.take_ = true;
    taking.offer(quit);
    BOOST_CHECK(taking.quitting());
}

/**
 * The default onEvent() consumes nothing, so an app that does not override it has every
 * event mapped.
 **/
BOOST_AUTO_TEST_CASE(engine_default_takes_no_event_test) {
    v3d::engine::Engine engine(appPath("good"));
    SDL_Event event{};
    event.type = SDL_EVENT_KEY_DOWN;
    BOOST_CHECK(!engine.onEvent(event));
}

/**
 * Every rejection below makes initialize() return false rather than throw. A malformed
 * document is a data error in the app, and the log says what was wrong with it.
 **/
BOOST_AUTO_TEST_CASE(engine_missing_config_document_test) {
    TestEngine engine(appPath("nowhere"), configFeature);
    BOOST_TEST(!engine.initialize());
}

BOOST_AUTO_TEST_CASE(engine_unloadable_config_file_test) {
    TestEngine engine(appPath("unloadable-config"), configFeature);
    BOOST_TEST(!engine.initialize());
}

BOOST_AUTO_TEST_CASE(engine_no_mappings_key_test) {
    TestEngine engine(appPath("no-mappings-key"), configFeature);
    BOOST_TEST(!engine.initialize());
    // the document itself loaded; reading its mappings rejected it
    BOOST_REQUIRE(engine.config());
    BOOST_TEST(static_cast<bool>(engine.config()->get(v3d::config::Type::Binding)));
}

BOOST_AUTO_TEST_CASE(engine_mapping_not_an_object_test) {
    TestEngine engine(appPath("mapping-not-object"), configFeature);
    BOOST_TEST(!engine.initialize());
    // the document itself loaded; reading its mappings rejected it
    BOOST_REQUIRE(engine.config());
    BOOST_TEST(static_cast<bool>(engine.config()->get(v3d::config::Type::Binding)));
}

BOOST_AUTO_TEST_CASE(engine_mapping_missing_source_test) {
    TestEngine engine(appPath("missing-source"), configFeature);
    BOOST_TEST(!engine.initialize());
    // the document itself loaded; reading its mappings rejected it
    BOOST_REQUIRE(engine.config());
    BOOST_TEST(static_cast<bool>(engine.config()->get(v3d::config::Type::Binding)));
}

BOOST_AUTO_TEST_CASE(engine_mapping_missing_destination_test) {
    TestEngine engine(appPath("missing-destination"), configFeature);
    BOOST_TEST(!engine.initialize());
    // the document itself loaded; reading its mappings rejected it
    BOOST_REQUIRE(engine.config());
    BOOST_TEST(static_cast<bool>(engine.config()->get(v3d::config::Type::Binding)));
}

BOOST_AUTO_TEST_CASE(engine_mapping_unsupported_param_test) {
    TestEngine engine(appPath("bad-param"), configFeature);
    BOOST_TEST(!engine.initialize());
    // the document itself loaded; reading its mappings rejected it
    BOOST_REQUIRE(engine.config());
    BOOST_TEST(static_cast<bool>(engine.config()->get(v3d::config::Type::Binding)));
}

/**
 * A command handler calls quit(), and the loop reads the flag after the handler returns.
 **/
BOOST_AUTO_TEST_CASE(engine_quit_test) {
    TestEngine engine(appPath("good"));
    BOOST_REQUIRE(engine.initialize());

    BOOST_TEST(!engine.quitting());
    engine.quit();
    BOOST_TEST(engine.quitting());

    // calling it twice is the same as calling it once
    engine.quit();
    BOOST_TEST(engine.quitting());
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

/**
 * A command is held while the key bound to it is, whether the binding fires on both edges or
 * on the press alone.
 **/
BOOST_AUTO_TEST_CASE(engine_held_follows_the_keyboard_test) {
    TestEngine engine(appPath("good"), boundFeature);
    BOOST_REQUIRE(engine.initialize());

    BOOST_CHECK(!engine.held("pong::leftPaddleUp"));
    engine.offer(keyDown(SDLK_W));
    BOOST_CHECK(engine.held("pong::leftPaddleUp"));
    engine.offer(keyUp(SDLK_W));
    BOOST_CHECK(!engine.held("pong::leftPaddleUp"));

    // bound for the press alone, and held all the same
    engine.offer(keyDown(SDLK_ESCAPE));
    BOOST_CHECK(engine.held("ui::quit"));
    engine.offer(keyUp(SDLK_ESCAPE));
    BOOST_CHECK(!engine.held("ui::quit"));

    BOOST_CHECK(!engine.held("pong::nothingBound"));
}

/**
 * A rebound command is held by its new key and not by its old one, with nothing asked of the
 * app but the rebind.
 **/
BOOST_AUTO_TEST_CASE(engine_held_follows_a_rebind_test) {
    TestEngine engine(appPath("good"), boundFeature);
    BOOST_REQUIRE(engine.initialize());
    BOOST_REQUIRE(engine.rebind("pong::leftPaddleUp", "arrow_up"));

    engine.offer(keyDown(SDLK_W));
    BOOST_CHECK(!engine.held("pong::leftPaddleUp"));
    engine.offer(keyDown(SDLK_UP));
    BOOST_CHECK(engine.held("pong::leftPaddleUp"));
}

/**
 * Without a keyboard nothing is held, rather than every command reading as up by accident of
 * a null state.
 **/
BOOST_AUTO_TEST_CASE(engine_held_without_a_keyboard_test) {
    TestEngine engine(appPath("good"), configFeature);
    BOOST_REQUIRE(engine.initialize());
    BOOST_CHECK(!engine.held("pong::leftPaddleUp"));
}

/**
 * With no binding config there is nothing for a rebind to rebuild, so rebind() returns false.
 **/
BOOST_AUTO_TEST_CASE(engine_rebind_without_bindings_test) {
    TestEngine engine(appPath("good"), v3d::engine::Feature::KeyboardInput);
    BOOST_REQUIRE(engine.initialize());
    BOOST_CHECK(!engine.rebind("pong::leftPaddleUp", "arrow_up"));
}

// Only run() calls shutdown(), so an app's handlers end the loop with quit().
/**
 * "ui::quit" means the same in every app, so the engine handles it as it handles a closed
 * window, and no app writes the handler.
 **/
BOOST_AUTO_TEST_CASE(engine_answers_ui_quit_test) {
    TestEngine engine(appPath("good"), configFeature);
    BOOST_REQUIRE(engine.initialize());

    v3d::event::Event other("quit", engine.events()->resolveContext("game"));
    other.type(v3d::event::Type::Destination);
    engine.dispatcher()->trigger(other);
    BOOST_TEST(!engine.quitting());

    v3d::event::Event quit("quit", engine.events()->resolveContext("ui"));
    quit.type(v3d::event::Type::Destination);
    engine.dispatcher()->trigger(quit);
    BOOST_TEST(engine.quitting());
}

template <typename T>
concept ShutsDown = requires(T& engine) { engine.shutdown(); };
template <typename T>
concept Quits = requires(T& engine) { engine.quit(); };
static_assert(!ShutsDown<TestEngine>);
static_assert(Quits<TestEngine>);

namespace {

/**
 * How often an engine's hooks ran, kept outside the engine so a test can read them after
 * run() has destroyed it.
 **/
struct Lifecycle final {
    int started = 0;
    int released = 0;
    bool starts = true;
};

/**
 * An engine with no features that counts its hooks. One that starts asks to quit at once, so
 * run() returns without a window or a frame.
 **/
class LifecycleEngine final : public v3d::engine::Engine {
 public:
    LifecycleEngine(const std::string& path, Lifecycle* counts) :
        Engine(path),
        counts_(counts) {
    }

 protected:
    v3d::engine::Features features() const override {
        return v3d::engine::Features();
    }

    bool start() override {
        counts_->started++;
        quit();
        return counts_->starts;
    }

    bool release() override {
        counts_->released++;
        return true;
    }

 private:
    Lifecycle* counts_;
};

};  // namespace

/**
 * run() starts an app once and releases it once, and the release happens whether start()
 * succeeded or not, since start() may have built something before it failed.
 **/
BOOST_AUTO_TEST_CASE(engine_releases_once_whether_or_not_it_started_test) {
    Lifecycle started;
    BOOST_CHECK_EQUAL(v3d::engine::run<LifecycleEngine>("engine_test.exe", "lifecycle", &started), EXIT_SUCCESS);
    BOOST_CHECK_EQUAL(started.started, 1);
    BOOST_CHECK_EQUAL(started.released, 1);

    Lifecycle failed;
    failed.starts = false;
    BOOST_CHECK_EQUAL(v3d::engine::run<LifecycleEngine>("engine_test.exe", "lifecycle", &failed), EXIT_FAILURE);
    BOOST_CHECK_EQUAL(failed.started, 1);
    BOOST_CHECK_EQUAL(failed.released, 1);
}
