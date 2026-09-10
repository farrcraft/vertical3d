/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/event/Engine.h>

#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

/**
 * The replacement for CommandDirectory: the engine resolves contexts by name, routes a
 * source event through its mappers, and dispatches a destination event by name for callers
 * that hold a string rather than a resolved event.
 **/
namespace {
/**
 * Collects every destination event the dispatcher delivers.
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
};  // namespace

BOOST_AUTO_TEST_CASE(engine_context_test) {
    boost::shared_ptr<entt::dispatcher> dispatcher = boost::make_shared<entt::dispatcher>();
    v3d::event::Engine engine(dispatcher);

    // a name resolves to a context, and the same name to the same one
    boost::shared_ptr<v3d::event::Context> keyboard = engine.resolveContext("keyboard");
    BOOST_REQUIRE(keyboard != nullptr);
    BOOST_CHECK_EQUAL(keyboard->name(), "keyboard");
    BOOST_CHECK_EQUAL(engine.resolveContext("keyboard"), keyboard);

    // a different name to a different one
    BOOST_CHECK(engine.resolveContext("mouse") != keyboard);
}

BOOST_AUTO_TEST_CASE(engine_dispatch_test) {
    boost::shared_ptr<entt::dispatcher> dispatcher = boost::make_shared<entt::dispatcher>();
    v3d::event::Engine engine(dispatcher);

    Recorder recorder;
    dispatcher->sink<v3d::event::Event>().connect<&Recorder::handle>(recorder);

    // dispatching by name resolves the context and builds the event, so a caller holding
    // two strings reaches the same sink a resolved Event would
    engine.dispatch("ui", "quit");
    BOOST_REQUIRE_EQUAL(recorder.events_.size(), 1u);
    BOOST_CHECK_EQUAL(recorder.events_[0].name(), "quit");
    BOOST_CHECK_EQUAL(recorder.events_[0].context()->name(), "ui");
    BOOST_CHECK(!recorder.events_[0].data());

    // and it can carry the parameter the old one passed as a string
    engine.dispatch("ui", "setMaxScore", 11);
    BOOST_REQUIRE_EQUAL(recorder.events_.size(), 2u);
    BOOST_REQUIRE(recorder.events_[1].data());
    BOOST_CHECK_EQUAL(std::get<int>(recorder.events_[1].data().get()), 11);
}

BOOST_AUTO_TEST_CASE(engine_mapping_test) {
    boost::shared_ptr<entt::dispatcher> dispatcher = boost::make_shared<entt::dispatcher>();
    v3d::event::Engine engine(dispatcher);

    Recorder recorder;
    dispatcher->sink<v3d::event::Event>().connect<&Recorder::handle>(recorder);

    boost::shared_ptr<v3d::event::Context> keyboard = engine.resolveContext("keyboard");
    boost::shared_ptr<v3d::event::Context> game = engine.resolveContext("pong");

    boost::shared_ptr<v3d::event::Mapper> mapper = boost::make_shared<v3d::event::Mapper>("global");
    v3d::event::Event bound("leftPaddleUp", game);
    bound.type(v3d::event::Type::Destination);
    mapper->map(source(keyboard, "w", v3d::event::State::Any), bound);
    engine.addMapper(mapper);

    // a source event reaches the engine through the same dispatcher and comes back out as
    // the destination it maps to
    dispatcher->trigger(source(keyboard, "w", v3d::event::State::Pressed));
    BOOST_REQUIRE_EQUAL(recorder.events_.size(), 1u);
    BOOST_CHECK_EQUAL(recorder.events_[0].name(), "leftPaddleUp");

    // carrying the edge with it, so one binding serves press and release
    BOOST_CHECK(recorder.events_[0].state() == v3d::event::State::Pressed);
    dispatcher->trigger(source(keyboard, "w", v3d::event::State::Released));
    BOOST_REQUIRE_EQUAL(recorder.events_.size(), 2u);
    BOOST_CHECK(recorder.events_[1].state() == v3d::event::State::Released);

    // an unbound key produces nothing
    dispatcher->trigger(source(keyboard, "q", v3d::event::State::Pressed));
    BOOST_CHECK_EQUAL(recorder.events_.size(), 2u);
}
