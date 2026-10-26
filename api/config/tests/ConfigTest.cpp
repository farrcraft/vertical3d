/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/asset/Manager.h>
#include <api/config/Config.h>
#include <api/config/Type.h>
#include <api/log/Logger.h>

#include <string>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

namespace {

/**
 * One fixture directory per case, because load() always reads config.json from the
 * manager's root.
 **/
boost::shared_ptr<v3d::asset::Manager> assets(const std::string& fixture) {
    return boost::make_shared<v3d::asset::Manager>(
        "data/" + fixture, boost::make_shared<v3d::log::Logger>());
}

bool loads(const std::string& fixture) {
    v3d::config::Config config(boost::make_shared<v3d::log::Logger>());
    return config.load(assets(fixture));
}

/**
 * config.json names a type and a file per entry. Each named file loads as an asset of its
 * own and is stored under its type.
 **/
BOOST_AUTO_TEST_CASE(config_load_test) {
    v3d::config::Config config(boost::make_shared<v3d::log::Logger>());
    BOOST_TEST(config.load(assets("good")));

    auto window = config.get(v3d::config::Type::Window);
    BOOST_TEST(static_cast<bool>(window));
    BOOST_TEST(window->document().at("width").as_int64() == 640);

    BOOST_TEST(static_cast<bool>(config.get(v3d::config::Type::Binding)));
    BOOST_TEST(static_cast<bool>(config.get(v3d::config::Type::Ui)));
}

/**
 * A type the document did not name is a null return rather than a default, so an app can
 * treat an absent config as "use the code's defaults".
 **/
BOOST_AUTO_TEST_CASE(config_absent_type_test) {
    v3d::config::Config config(boost::make_shared<v3d::log::Logger>());
    BOOST_TEST(config.load(assets("good")));

    BOOST_TEST(!config.get(v3d::config::Type::Sound));
    BOOST_TEST(!config.get(v3d::config::Type::Camera));
    BOOST_TEST(!config.get(v3d::config::Type::Layout));
    BOOST_TEST(!config.get(v3d::config::Type::Unknown));
}

/**
 * Each rejection below is a false return from load(), not an exception.
 **/
BOOST_AUTO_TEST_CASE(config_missing_document_test) {
    BOOST_TEST(!loads("nowhere"));
}

BOOST_AUTO_TEST_CASE(config_no_configs_key_test) {
    BOOST_TEST(!loads("no-configs"));
}

BOOST_AUTO_TEST_CASE(config_configs_not_an_array_test) {
    BOOST_TEST(!loads("not-an-array"));
}

BOOST_AUTO_TEST_CASE(config_entry_not_an_object_test) {
    BOOST_TEST(!loads("entry-not-object"));
}

BOOST_AUTO_TEST_CASE(config_entry_missing_file_test) {
    BOOST_TEST(!loads("missing-file-key"));
}

/**
 * A type or a file that is not a string is a rejection, not the exception boost::json::value_to
 * throws for it.
 **/
BOOST_AUTO_TEST_CASE(config_entry_value_not_a_string_test) {
    BOOST_CHECK_NO_THROW(BOOST_TEST(!loads("type-not-string")));
    BOOST_CHECK_NO_THROW(BOOST_TEST(!loads("file-not-string")));
}

/**
 * An empty type is refused. Filed under the empty name, it would be what
 * get(Type::Unknown) finds.
 **/
BOOST_AUTO_TEST_CASE(config_entry_empty_type_test) {
    v3d::config::Config config(boost::make_shared<v3d::log::Logger>());
    BOOST_TEST(!config.load(assets("empty-type")));

    BOOST_TEST(!config.get(v3d::config::Type::Unknown));
    BOOST_TEST(!config.get(""));
}

/**
 * A type the api has no name for is an app's own document. load() stores it like the rest,
 * for the app to ask for by name, and does not refuse the config.
 **/
BOOST_AUTO_TEST_CASE(config_app_type_test) {
    v3d::config::Config config(boost::make_shared<v3d::log::Logger>());
    BOOST_REQUIRE(config.load(assets("app-type")));

    auto menu = config.get("menu");
    BOOST_REQUIRE(menu);
    BOOST_TEST(menu->document().at("items").as_array().size() == 2U);
    BOOST_TEST(!config.get("nothing"));
}

BOOST_AUTO_TEST_CASE(config_named_file_absent_test) {
    BOOST_TEST(!loads("absent-file"));
}

/**
 * The extension picks a loader, and Manager::load returns no asset for an
 * extension nothing is registered for. A config naming a file this library cannot load is
 * therefore a rejection like any other.
 **/
BOOST_AUTO_TEST_CASE(config_unloadable_extension_test) {
    BOOST_TEST(!loads("bad-extension"));
}

/**
 * A failed load stores nothing. The window entry loads before the binding entry fails, and is
 * not kept.
 **/
BOOST_AUTO_TEST_CASE(config_rejection_files_nothing_test) {
    v3d::config::Config config(boost::make_shared<v3d::log::Logger>());
    BOOST_TEST(!config.load(assets("absent-file")));

    BOOST_TEST(!config.get(v3d::config::Type::Window));
}

};  // namespace
