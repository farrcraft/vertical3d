/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/config/Config.h>
#include <api/config/Type.h>

#include <string>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

namespace {

/**
 * One directory per case, because load() always reads config.json out of the manager's
 * own path and a rejection is only distinguishable from the document that caused it.
 **/
boost::shared_ptr<v3d::asset::Manager> assets(const std::string& fixture) {
    return boost::make_shared<v3d::asset::Manager>(
        "data/" + fixture, boost::make_shared<v3d::log::Logger>());
}

bool loads(const std::string& fixture) {
    v3d::config::Config config(boost::make_shared<v3d::log::Logger>());
    return config.load(assets(fixture));
}

};  // namespace

/**
 * The indirect form every app is on: config.json names a type and a file per entry, and each
 * named file is loaded as an asset of its own and filed under its type.
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
 * A type the document did not name is a null return rather than a default, which is what lets
 * an app treat an absent config as "take what the code already does".
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
 * Every rejection below is a false return rather than an exception out of engine startup,
 * which is the whole reason load() guards each lookup with a contains().
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

BOOST_AUTO_TEST_CASE(config_unknown_type_test) {
    BOOST_TEST(!loads("unknown-type"));
}

BOOST_AUTO_TEST_CASE(config_named_file_absent_test) {
    BOOST_TEST(!loads("absent-file"));
}

/**
 * The extension is what picks a loader, and Manager::loadTypeFromExt throws for one it does
 * not know - so a config naming a file this library cannot load has to be a rejection like
 * any other rather than the one path out of load() that escapes as an exception.
 **/
BOOST_AUTO_TEST_CASE(config_unloadable_extension_test) {
    BOOST_TEST(!loads("bad-extension"));
}

/**
 * A failed load files nothing: an entry read before the one that failed is not worth keeping,
 * since the caller is going to abandon the whole config.
 **/
BOOST_AUTO_TEST_CASE(config_rejection_files_nothing_test) {
    v3d::config::Config config(boost::make_shared<v3d::log::Logger>());
    BOOST_TEST(!config.load(assets("absent-file")));

    BOOST_TEST(!config.get(v3d::config::Type::Window));
}
