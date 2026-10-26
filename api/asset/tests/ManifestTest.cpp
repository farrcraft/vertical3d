/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/asset/Manager.h>
#include <api/asset/Manifest.h>
#include <api/log/Logger.h>

#include <string>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

namespace {

/**
 * One fixture directory per case, each holding a config.json, because the files a list names
 * resolve against the manager's root.
 **/
v3d::asset::Manager assets(const std::string& fixture) {
    return v3d::asset::Manager("data/manifest/" + fixture, boost::make_shared<v3d::log::Logger>());
}

bool loads(const std::string& fixture) {
    v3d::asset::Manifest manifest(boost::make_shared<v3d::log::Logger>());
    v3d::asset::Manager manager = assets(fixture);
    return manifest.load(manager, "config.json");
}

/**
 * The list names a kind and a file per entry. Each named file loads as an asset of its own and
 * is held under its kind.
 **/
BOOST_AUTO_TEST_CASE(manifest_load_test) {
    v3d::asset::Manifest manifest(boost::make_shared<v3d::log::Logger>());
    v3d::asset::Manager manager = assets("good");
    BOOST_TEST(manifest.load(manager, "config.json"));

    auto window = manifest.get("window");
    BOOST_TEST(static_cast<bool>(window));
    BOOST_TEST(window->document().at("width").as_int64() == 640);

    BOOST_TEST(static_cast<bool>(manifest.get("binding")));
    BOOST_TEST(static_cast<bool>(manifest.get("ui")));
}

/**
 * A kind the list did not name is a null return rather than a default, so an app can treat an
 * absent document as "use the code's defaults".
 **/
BOOST_AUTO_TEST_CASE(manifest_absent_kind_test) {
    v3d::asset::Manifest manifest(boost::make_shared<v3d::log::Logger>());
    v3d::asset::Manager manager = assets("good");
    BOOST_TEST(manifest.load(manager, "config.json"));

    BOOST_TEST(!manifest.get("sound"));
    BOOST_TEST(!manifest.get(""));
}

/**
 * The list is the file the caller names, not one the manifest assumes.
 **/
BOOST_AUTO_TEST_CASE(manifest_named_list_test) {
    v3d::asset::Manifest manifest(boost::make_shared<v3d::log::Logger>());
    v3d::asset::Manager manager = assets("good");
    BOOST_TEST(!manifest.load(manager, "other.json"));
    BOOST_TEST(manifest.load(manager, "config.json"));
}

/**
 * Each rejection below is a false return from load(), not an exception.
 **/
BOOST_AUTO_TEST_CASE(manifest_missing_document_test) {
    BOOST_TEST(!loads("nowhere"));
}

BOOST_AUTO_TEST_CASE(manifest_no_configs_key_test) {
    BOOST_TEST(!loads("no-configs"));
}

BOOST_AUTO_TEST_CASE(manifest_configs_not_an_array_test) {
    BOOST_TEST(!loads("not-an-array"));
}

BOOST_AUTO_TEST_CASE(manifest_entry_not_an_object_test) {
    BOOST_TEST(!loads("entry-not-object"));
}

BOOST_AUTO_TEST_CASE(manifest_entry_missing_file_test) {
    BOOST_TEST(!loads("missing-file-key"));
}

/**
 * A kind or a file that is not a string is a rejection, not the exception boost::json::value_to
 * throws for it.
 **/
BOOST_AUTO_TEST_CASE(manifest_entry_value_not_a_string_test) {
    BOOST_CHECK_NO_THROW(BOOST_TEST(!loads("type-not-string")));
    BOOST_CHECK_NO_THROW(BOOST_TEST(!loads("file-not-string")));
}

/**
 * An empty kind is refused, so get("") never finds a document.
 **/
BOOST_AUTO_TEST_CASE(manifest_entry_empty_kind_test) {
    v3d::asset::Manifest manifest(boost::make_shared<v3d::log::Logger>());
    v3d::asset::Manager manager = assets("empty-type");
    BOOST_TEST(!manifest.load(manager, "config.json"));

    BOOST_TEST(!manifest.get(""));
}

/**
 * A kind no api library reads is an app's own document, held like the rest.
 **/
BOOST_AUTO_TEST_CASE(manifest_app_kind_test) {
    v3d::asset::Manifest manifest(boost::make_shared<v3d::log::Logger>());
    v3d::asset::Manager manager = assets("app-type");
    BOOST_REQUIRE(manifest.load(manager, "config.json"));

    auto menu = manifest.get("menu");
    BOOST_REQUIRE(menu);
    BOOST_TEST(menu->document().at("items").as_array().size() == 2U);
    BOOST_TEST(!manifest.get("nothing"));
}

BOOST_AUTO_TEST_CASE(manifest_named_file_absent_test) {
    BOOST_TEST(!loads("absent-file"));
}

/**
 * The extension picks a loader, and Manager::load returns no asset for an extension nothing is
 * registered for. A list naming a file the manager cannot load is a rejection like any other.
 **/
BOOST_AUTO_TEST_CASE(manifest_unloadable_extension_test) {
    BOOST_TEST(!loads("bad-extension"));
}

/**
 * A failed load stores nothing. The window entry loads before the binding entry fails, and is
 * not kept.
 **/
BOOST_AUTO_TEST_CASE(manifest_rejection_files_nothing_test) {
    v3d::asset::Manifest manifest(boost::make_shared<v3d::log::Logger>());
    v3d::asset::Manager manager = assets("absent-file");
    BOOST_TEST(!manifest.load(manager, "config.json"));

    BOOST_TEST(!manifest.get("window"));
}

};  // namespace
