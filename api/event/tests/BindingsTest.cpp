/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/event/Bindings.h>
#include <api/event/Engine.h>
#include <api/event/Event.h>
#include <api/event/State.h>
#include <api/log/Logger.h>

#include <algorithm>
#include <string>
#include <string_view>
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

/**
 * Every source bound to a command is found, and a document that does not describe its
 * bindings is refused without disturbing the ones already in place.
 **/
BOOST_FIXTURE_TEST_CASE(bindings_load_test, Fixture) {
    BOOST_REQUIRE(bindings.load(boost::json::parse(DOCUMENT).as_object()));
    BOOST_TEST(bindings.sources("game::up").size() == 2U);
    BOOST_TEST(bindings.sources("game::down").empty());

    BOOST_TEST(!bindings.load(boost::json::parse(R"({"bindings": []})").as_object()));
    BOOST_TEST(bindings.sources("game::up").size() == 2U);
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
    BOOST_REQUIRE_EQUAL(quit.size(), 1U);
    BOOST_TEST(quit[0].name() == "escape");
}

/**
 * Every source is put to the owner's check, and one it does not recognise is still bound -
 * it is reported, and fires on nothing.
 **/
BOOST_FIXTURE_TEST_CASE(bindings_known_test, Fixture) {
    BOOST_REQUIRE(bindings.load(boost::json::parse(DOCUMENT).as_object()));
    BOOST_TEST(asked.size() == 3U);
    BOOST_TEST(bindings.sources("ui::quit").size() == 1U);
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
    BOOST_TEST(bindings.sources("game::up").size() == 2U);

    // the document kept is the one that loaded, so a rebind still rebuilds from it
    BOOST_TEST(bindings.rebind("ui::quit", "escape"));
    BOOST_TEST(bindings.sources("game::up").size() == 2U);
}

/**
 * A param an int cannot hold is refused rather than narrowed to a different number, and one an
 * int can hold arrives as the command's data.
 **/
BOOST_FIXTURE_TEST_CASE(bindings_refuse_a_param_beyond_an_int_test, Fixture) {
    BOOST_TEST(!bindings.load(boost::json::parse(R"({"mappings": [
        {"source": {"name": "w", "context": "keyboard"},
         "destination": {"name": "up", "context": "game", "param": 4294967936}}
    ]})").as_object()));
    BOOST_TEST(!bindings.load(boost::json::parse(R"({"mappings": [
        {"source": {"name": "w", "context": "keyboard"},
         "destination": {"name": "up", "context": "game", "param": 18446744073709551615}}
    ]})").as_object()));

    BOOST_REQUIRE(bindings.load(boost::json::parse(R"({"mappings": [
        {"source": {"name": "w", "context": "keyboard"},
         "destination": {"name": "up", "context": "game", "param": -7}}
    ]})").as_object()));
    const std::vector<v3d::event::Event> sources = bindings.sources("game::up");
    BOOST_REQUIRE_EQUAL(sources.size(), 1U);
}

/**
 * The commands are what the bindings send, each once however many sources it has, and none
 * before a document is loaded.
 **/
BOOST_FIXTURE_TEST_CASE(bindings_list_their_commands_test, Fixture) {
    BOOST_TEST(bindings.commands().empty());

    BOOST_REQUIRE(bindings.load(boost::json::parse(DOCUMENT).as_object()));
    const std::vector<std::string> commands = bindings.commands();
    BOOST_REQUIRE_EQUAL(commands.size(), 2U);
    BOOST_TEST(commands[0] == "game::up");
    BOOST_TEST(commands[1] == "ui::quit");
}

/**
 * The names of a command's sources, in name order, for comparing what a rebind left.
 **/
std::vector<std::string> names(const v3d::event::Bindings& bindings, std::string_view command) {
    std::vector<std::string> found;
    for (const v3d::event::Event& source : bindings.sources(command)) {
        found.emplace_back(source.name());
    }
    std::sort(found.begin(), found.end());
    return found;
}

/**
 * A rebind of a command bound to two keys binds it to one. The mapping that fires on either
 * edge covers the one that fires on a press, so only it is installed, and a press of the new
 * key sends the command once.
 **/
BOOST_FIXTURE_TEST_CASE(bindings_rebind_every_source_to_one_key_test, Fixture) {
    BOOST_REQUIRE(bindings.load(boost::json::parse(DOCUMENT).as_object()));
    BOOST_REQUIRE(bindings.rebind("game::up", "e"));

    const std::vector<v3d::event::Event> up = bindings.sources("game::up");
    BOOST_REQUIRE_EQUAL(up.size(), 1U);
    BOOST_TEST(up[0].name() == "e");
    BOOST_CHECK(up[0].state() == v3d::event::State::Any);
}

/**
 * Two sources on the same edge rebound to one key leave one mapping, not two that would each
 * send the command.
 **/
BOOST_FIXTURE_TEST_CASE(bindings_collapse_two_sources_on_one_edge_test, Fixture) {
    BOOST_REQUIRE(bindings.load(boost::json::parse(R"({"mappings": [
        {"source": {"name": "w", "context": "keyboard"}, "destination": {"name": "up", "context": "game"}},
        {"source": {"name": "arrow_up", "context": "keyboard"}, "destination": {"name": "up", "context": "game"}}
    ]})").as_object()));
    BOOST_REQUIRE(bindings.rebind("game::up", "e"));
    BOOST_CHECK(names(bindings, "game::up") == std::vector<std::string>({"e"}));
}

/**
 * A rebind of one source leaves the command's other source as the document bound it, and
 * keeps the edge the document gave the one it replaced. A source the document does not bind
 * the command to cannot be rebound.
 **/
BOOST_FIXTURE_TEST_CASE(bindings_rebind_one_source_test, Fixture) {
    BOOST_TEST(!bindings.rebind("game::up", "w", "e"));
    BOOST_REQUIRE(bindings.load(boost::json::parse(DOCUMENT).as_object()));

    BOOST_REQUIRE(bindings.rebind("game::up", "w", "e"));
    BOOST_CHECK(names(bindings, "game::up") == std::vector<std::string>({"arrow_up", "e"}));
    for (const v3d::event::Event& source : bindings.sources("game::up")) {
        if (source.name() == "e") {
            BOOST_CHECK(source.state() == v3d::event::State::Pressed);
        }
    }

    // named by the document's name however often it has been rebound
    BOOST_REQUIRE(bindings.rebind("game::up", "w", "r"));
    BOOST_CHECK(names(bindings, "game::up") == std::vector<std::string>({"arrow_up", "r"}));

    BOOST_TEST(!bindings.rebind("game::up", "q", "e"));
    BOOST_TEST(!bindings.rebind("game::up", "e", "f"));
    BOOST_CHECK(names(bindings, "game::up") == std::vector<std::string>({"arrow_up", "r"}));
}

/**
 * A reset of one command puts it back as the document binds it and leaves the others rebound.
 * A reset of everything puts every command back.
 **/
BOOST_FIXTURE_TEST_CASE(bindings_reset_test, Fixture) {
    BOOST_TEST(!bindings.reset());
    BOOST_REQUIRE(bindings.load(boost::json::parse(DOCUMENT).as_object()));
    BOOST_REQUIRE(bindings.rebind("ui::quit", "escape"));
    BOOST_REQUIRE(bindings.rebind("game::up", "e"));

    BOOST_REQUIRE(bindings.reset("ui::quit"));
    BOOST_CHECK(names(bindings, "ui::quit") == std::vector<std::string>({"q"}));
    BOOST_CHECK(names(bindings, "game::up") == std::vector<std::string>({"e"}));
    BOOST_TEST(bindings.overrides().size() == 1U);

    BOOST_REQUIRE(bindings.reset());
    BOOST_CHECK(names(bindings, "game::up") == std::vector<std::string>({"arrow_up", "w"}));
    BOOST_TEST(bindings.overrides().empty());
}

/**
 * The overrides are what an app saves, and restoring them over the same document binds every
 * command as it was bound when they were saved. An override naming a source the document no
 * longer binds is dropped.
 **/
BOOST_FIXTURE_TEST_CASE(bindings_restore_saved_overrides_test, Fixture) {
    BOOST_REQUIRE(bindings.load(boost::json::parse(DOCUMENT).as_object()));
    BOOST_REQUIRE(bindings.rebind("game::up", "w", "e"));
    BOOST_REQUIRE(bindings.rebind("ui::quit", "escape"));
    std::vector<v3d::event::Bindings::Override> saved = bindings.overrides();
    BOOST_REQUIRE_EQUAL(saved.size(), 2U);
    saved.push_back(v3d::event::Bindings::Override{.command = "game::up", .source = "gone", .name = "x"});

    const Fixture fresh;
    v3d::event::Bindings restored(fresh.events, boost::make_shared<v3d::log::Logger>());
    BOOST_REQUIRE(restored.load(boost::json::parse(DOCUMENT).as_object()));
    BOOST_REQUIRE(restored.restore(saved));

    BOOST_CHECK(names(restored, "game::up") == names(bindings, "game::up"));
    BOOST_CHECK(names(restored, "ui::quit") == std::vector<std::string>({"escape"}));
    BOOST_CHECK(restored.overrides() == bindings.overrides());
}

/**
 * What a key fires is read with the overrides applied, so a controls screen can say what a
 * key is already bound to before it is rebound.
 **/
BOOST_FIXTURE_TEST_CASE(bindings_say_what_a_key_fires_test, Fixture) {
    BOOST_TEST(bindings.bound("q").empty());
    BOOST_REQUIRE(bindings.load(boost::json::parse(DOCUMENT).as_object()));
    BOOST_CHECK(bindings.bound("q") == std::vector<std::string>({"ui::quit"}));
    BOOST_TEST(bindings.bound("e").empty());

    BOOST_REQUIRE(bindings.rebind("game::up", "w", "q"));
    BOOST_CHECK(bindings.bound("q") == std::vector<std::string>({"game::up", "ui::quit"}));
    BOOST_TEST(bindings.bound("w").empty());
}

};  // namespace
