/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/event/Engine.h>
#include <api/event/Source.h>

#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

/**
 * The engine resolves contexts by name and routes a source event through its mappers.
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

v3d::event::Source source(const boost::shared_ptr<v3d::event::Context>& context,
    const std::string& name, v3d::event::State state) {
    return v3d::event::Source(name, context, state);
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
    v3d::event::publish(*dispatcher, source(keyboard, "w", v3d::event::State::Pressed));
    BOOST_REQUIRE_EQUAL(recorder.events_.size(), 1u);
    BOOST_CHECK_EQUAL(recorder.events_[0].name(), "leftPaddleUp");

    // carrying the edge with it, so one binding serves press and release
    BOOST_CHECK(recorder.events_[0].state() == v3d::event::State::Pressed);
    v3d::event::publish(*dispatcher, source(keyboard, "w", v3d::event::State::Released));
    BOOST_REQUIRE_EQUAL(recorder.events_.size(), 2u);
    BOOST_CHECK(recorder.events_[1].state() == v3d::event::State::Released);

    // an unbound key produces nothing
    v3d::event::publish(*dispatcher, source(keyboard, "q", v3d::event::State::Pressed));
    BOOST_CHECK_EQUAL(recorder.events_.size(), 2u);
}

BOOST_AUTO_TEST_CASE(engine_lets_the_dispatcher_go_test) {
    boost::shared_ptr<entt::dispatcher> dispatcher = boost::make_shared<entt::dispatcher>();
    {
        v3d::event::Engine engine(dispatcher);
        BOOST_CHECK(!dispatcher->sink<v3d::event::Unclaimed>().empty());
    }
    // the dispatcher outlives the engine, and a delegate to it would be a dangling call
    BOOST_CHECK(dispatcher->sink<v3d::event::Unclaimed>().empty());
}

namespace {
/**
 * Hears both sinks into one list, so the order a key and its command arrive in can be read.
 **/
struct Order {
    void key(const v3d::event::Source& source) {
        heard_.push_back("key " + std::string(source.name()));
        if (drop_) {
            source.consume();
        }
    }

    void command(const v3d::event::Event& event) {
        heard_.push_back("command " + std::string(event.name()));
    }

    bool drop_ = false;
    std::vector<std::string> heard_;
};
};  // namespace

/**
 * A key and the command it is bound to go to two sinks, and every listener receives the key
 * before any receives the command, whichever was connected first. A listener on the key can
 * also consume it, as a key capture does, and then its bindings send nothing.
 **/
BOOST_AUTO_TEST_CASE(engine_a_key_is_heard_before_its_command_test) {
    boost::shared_ptr<entt::dispatcher> dispatcher = boost::make_shared<entt::dispatcher>();
    v3d::event::Engine engine(dispatcher);

    boost::shared_ptr<v3d::event::Context> keyboard = engine.resolveContext("keyboard");
    boost::shared_ptr<v3d::event::Mapper> mapper = boost::make_shared<v3d::event::Mapper>("global");
    v3d::event::Event bound("up", engine.resolveContext("game"));
    bound.type(v3d::event::Type::Destination);
    mapper->map(source(keyboard, "w", v3d::event::State::Any), bound);
    engine.addMapper(mapper);

    Order order;
    dispatcher->sink<v3d::event::Event>().connect<&Order::command>(order);
    dispatcher->sink<v3d::event::Source>().connect<&Order::key>(order);

    v3d::event::publish(*dispatcher, source(keyboard, "w", v3d::event::State::Pressed));
    BOOST_REQUIRE_EQUAL(order.heard_.size(), 2u);
    BOOST_TEST(order.heard_[0] == "key w");
    BOOST_TEST(order.heard_[1] == "command up");

    order.drop_ = true;
    v3d::event::publish(*dispatcher, source(keyboard, "w", v3d::event::State::Pressed));
    BOOST_REQUIRE_EQUAL(order.heard_.size(), 3u);
    BOOST_TEST(order.heard_[2] == "key w");
}
