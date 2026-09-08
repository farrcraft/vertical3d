/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <string>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/json.hpp>
#include <boost/make_shared.hpp>

#include "../Settings.h"
#include "../../asset/Writer.h"

namespace {

boost::shared_ptr<v3d::log::Logger> logger() {
    return boost::make_shared<v3d::log::Logger>();
}

/**
 * Every case reads and writes the same real directory - userPath() is the only thing that
 * says where that is - so each starts by removing whatever the last one left.
 **/
v3d::engine::Settings fresh(const std::string& app) {
    v3d::engine::Settings settings("vertical3d_tests", app, logger());
    if (!settings.path().empty()) {
        boost::filesystem::remove(settings.path());
    }
    return settings;
}

/**
 * Put a document in place of the one a Settings would write, to read back.
 **/
void put(const v3d::engine::Settings& settings, const std::string& text) {
    BOOST_REQUIRE(v3d::asset::writeFile(settings.path(), text));
}

};  // namespace

/**
 * A player who has never opened a settings screen is the common case, and it is not a
 * failure: every read gives back what the build ships.
 **/
BOOST_AUTO_TEST_CASE(settings_missing_document_test) {
    v3d::engine::Settings settings = fresh("missing");
    BOOST_REQUIRE(!settings.path().empty());
    BOOST_CHECK(!boost::filesystem::exists(settings.path()));

    BOOST_CHECK_EQUAL(settings.load(), false);
    BOOST_CHECK_EQUAL(settings.writable(), true);
    BOOST_CHECK_EQUAL(settings.text("paddle1up", "w"), "w");
    BOOST_CHECK_EQUAL(settings.integer("window.width", 1280), 1280);
    BOOST_CHECK_EQUAL(settings.number("volume.music", 0.8), 0.8);
    BOOST_CHECK_EQUAL(settings.flag("fullscreen", false), false);
}

/**
 * What was set is what comes back on the next run, and nothing else is: an overlay, so a
 * setting nobody touched still tracks the build.
 **/
BOOST_AUTO_TEST_CASE(settings_round_trip_test) {
    v3d::engine::Settings settings = fresh("roundtrip");
    settings.set("paddle1up", "q");
    settings.set("window.width", 1920);
    settings.set("volume.music", 0.25);
    settings.set("fullscreen", true);
    BOOST_REQUIRE_EQUAL(settings.save(), true);

    v3d::engine::Settings reread("vertical3d_tests", "roundtrip", logger());
    BOOST_REQUIRE_EQUAL(reread.load(), true);
    BOOST_CHECK_EQUAL(reread.text("paddle1up", "w"), "q");
    BOOST_CHECK_EQUAL(reread.integer("window.width", 1280), 1920);
    BOOST_CHECK_EQUAL(reread.number("volume.music", 0.8), 0.25);
    BOOST_CHECK_EQUAL(reread.flag("fullscreen", false), true);

    // a key nobody stored is still the caller's default
    BOOST_CHECK_EQUAL(reread.text("paddle2up", "up"), "up");

    // and clearing one puts it back to tracking the build
    reread.clear("paddle1up");
    BOOST_REQUIRE_EQUAL(reread.save(), true);
    v3d::engine::Settings cleared("vertical3d_tests", "roundtrip", logger());
    BOOST_REQUIRE_EQUAL(cleared.load(), true);
    BOOST_CHECK_EQUAL(cleared.text("paddle1up", "w"), "w");

    boost::filesystem::remove(settings.path());
}

/**
 * A key belonging to a feature this build has never heard of survives a save by it, so a
 * player who moves between builds does not lose what the other one stored.
 **/
BOOST_AUTO_TEST_CASE(settings_unknown_key_survives_test) {
    v3d::engine::Settings settings = fresh("unknown");
    put(settings, "{ \"version\": 1, \"settings\": { \"fromlater\": \"kept\", \"known\": 1 } }");

    BOOST_REQUIRE_EQUAL(settings.load(), true);
    settings.set("known", 2);
    BOOST_REQUIRE_EQUAL(settings.save(), true);

    v3d::engine::Settings reread("vertical3d_tests", "unknown", logger());
    BOOST_REQUIRE_EQUAL(reread.load(), true);
    BOOST_CHECK_EQUAL(reread.integer("known", 0), 2);
    BOOST_CHECK_EQUAL(reread.text("fromlater", ""), "kept");

    boost::filesystem::remove(settings.path());
}

/**
 * A document from a later build runs on defaults and is not written back: the build that
 * wrote it knows what is in it, and this one would drop everything it does not.
 **/
BOOST_AUTO_TEST_CASE(settings_future_version_test) {
    v3d::engine::Settings settings = fresh("future");
    const std::string document =
        "{ \"version\": 99, \"settings\": { \"paddle1up\": \"z\" } }";
    put(settings, document);

    BOOST_CHECK_EQUAL(settings.load(), false);
    BOOST_CHECK_EQUAL(settings.writable(), false);
    BOOST_CHECK_EQUAL(settings.text("paddle1up", "w"), "w");

    settings.set("paddle1up", "x");
    BOOST_CHECK_EQUAL(settings.save(), false);

    // and the document is exactly as the later build left it
    BOOST_CHECK(boost::filesystem::exists(settings.path()));
    BOOST_CHECK_EQUAL(boost::filesystem::file_size(settings.path()), document.size());

    boost::filesystem::remove(settings.path());
}

/**
 * A document that got truncated, or that somebody edited into something that will not parse,
 * costs the settings and not the app.
 **/
BOOST_AUTO_TEST_CASE(settings_malformed_document_test) {
    v3d::engine::Settings settings = fresh("malformed");
    put(settings, "{ \"version\": 1, \"settings\": { \"paddle1up\": ");

    BOOST_CHECK_EQUAL(settings.load(), false);
    BOOST_CHECK_EQUAL(settings.text("paddle1up", "w"), "w");

    // unlike a future document, this one is replaceable - there is nothing in it to keep
    BOOST_CHECK_EQUAL(settings.writable(), true);
    settings.set("paddle1up", "e");
    BOOST_REQUIRE_EQUAL(settings.save(), true);

    v3d::engine::Settings reread("vertical3d_tests", "malformed", logger());
    BOOST_REQUIRE_EQUAL(reread.load(), true);
    BOOST_CHECK_EQUAL(reread.text("paddle1up", "w"), "e");

    boost::filesystem::remove(settings.path());
}

/**
 * A hand edited document cannot make an app read a string as a volume.
 **/
BOOST_AUTO_TEST_CASE(settings_wrong_type_test) {
    v3d::engine::Settings settings = fresh("types");
    put(settings, "{ \"version\": 1, \"settings\": "
        "{ \"volume\": \"loud\", \"name\": 4, \"fullscreen\": \"yes\", \"width\": 12.5 } }");

    BOOST_REQUIRE_EQUAL(settings.load(), true);
    BOOST_CHECK_EQUAL(settings.number("volume", 0.5), 0.5);
    BOOST_CHECK_EQUAL(settings.text("name", "player"), "player");
    BOOST_CHECK_EQUAL(settings.flag("fullscreen", false), false);
    BOOST_CHECK_EQUAL(settings.integer("width", 1280), 1280);

    boost::filesystem::remove(settings.path());
}
