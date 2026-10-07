/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/event/Bindings.h>
#include <api/log/Logger.h>

#include <string>
#include <vector>

#include <boost/json.hpp>
#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

namespace {

const char* DOCUMENT = R"({"mappings": [
    {"source": {"name": "w", "context": "keyboard", "state": "pressed"},
     "destination": {"name": "up", "context": "game"}},
    {"source": {"name": "arrow_up", "context": "keyboard"},
     "destination": {"name": "up", "context": "game"}},
    {"source": {"name": "q", "context": "keyboard"},
     "destination": {"name": "quit", "context": "ui"}}
]})";

struct Fixture {
    Fixture() :
        dispatcher(boost::make_shared<entt::dispatcher>()),
        events(boost::make_shared<v3d::event::Engine>(dispatcher)),
        bindings(events, boost::make_shared<v3d::log::Logger>(),
            [this](const v3d::event::Event& source) {
                asked.push_back(std::string(source.name()));
                return source.name() != "q";
            }) {
    }

    boost::shared_ptr<entt::dispatcher> dispatcher;
    boost::shared_ptr<v3d::event::Engine> events;
    std::vector<std::string> asked;
    v3d::event::Bindings bindings;
};

};  // namespace

/**
 * Every source bound to a command is found, and a document that does not describe its
 * bindings is refused without disturbing the ones already in place.
 **/
BOOST_FIXTURE_TEST_CASE(bindings_load_test, Fixture) {
    BOOST_REQUIRE(bindings.load(boost::json::parse(DOCUMENT).as_object()));
    BOOST_TEST(bindings.sources("game::up").size() == 2u);
    BOOST_TEST(bindings.sources("game::down").empty());

    BOOST_TEST(!bindings.load(boost::json::parse(R"({"bindings": []})").as_object()));
    BOOST_TEST(bindings.sources("game::up").size() == 2u);
}

/**
 * A rebind changes the name a command is bound to and keeps the edge the document gave it,
 * and has nothing to rebuild before a document is loaded.
 **/
BOOST_FIXTURE_TEST_CASE(bindings_rebind_test, Fixture) {
    BOOST_TEST(!bindings.rebind("ui::quit", "escape"));

    BOOST_REQUIRE(bindings.load(boost::json::parse(DOCUMENT).as_object()));
    BOOST_REQUIRE(bindings.rebind("ui::quit", "escape"));
    const std::vector<v3d::event::Event> quit = bindings.sources("ui::quit");
    BOOST_REQUIRE_EQUAL(quit.size(), 1u);
    BOOST_TEST(quit[0].name() == "escape");
}

/**
 * Every source is put to the owner's check, and one it does not recognise is still bound -
 * it is reported, and fires on nothing.
 **/
BOOST_FIXTURE_TEST_CASE(bindings_known_test, Fixture) {
    BOOST_REQUIRE(bindings.load(boost::json::parse(DOCUMENT).as_object()));
    BOOST_TEST(asked.size() == 3u);
    BOOST_TEST(bindings.sources("ui::quit").size() == 1u);
}

/**
 * A name, a context or a state that is not a string is refused like any other document the
 * bindings do not understand, and leaves the bindings already in place.
 **/
BOOST_FIXTURE_TEST_CASE(bindings_refuse_a_value_that_is_not_a_string_test, Fixture) {
    BOOST_REQUIRE(bindings.load(boost::json::parse(DOCUMENT).as_object()));

    BOOST_TEST(!bindings.load(boost::json::parse(R"({"mappings": [
        {"source": {"name": 5, "context": "keyboard"}, "destination": {"name": "up", "context": "game"}}
    ]})").as_object()));
    BOOST_TEST(!bindings.load(boost::json::parse(R"({"mappings": [
        {"source": {"name": "w", "context": "keyboard"}, "destination": {"name": "up", "context": null}}
    ]})").as_object()));
    BOOST_TEST(!bindings.load(boost::json::parse(R"({"mappings": [
        {"source": {"name": "w", "context": "keyboard", "state": 1}, "destination": {"name": "up", "context": "game"}}
    ]})").as_object()));
    BOOST_TEST(bindings.sources("game::up").size() == 2u);

    // the document kept is the one that loaded, so a rebind still rebuilds from it
    BOOST_TEST(bindings.rebind("ui::quit", "escape"));
    BOOST_TEST(bindings.sources("game::up").size() == 2u);
}
