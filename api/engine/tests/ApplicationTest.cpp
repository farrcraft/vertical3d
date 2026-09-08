/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <string>
#include <utility>

#include <boost/test/unit_test.hpp>

#include "../Application.h"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

namespace {

/**
 * What run<T> built its engine with. run<T> builds it on the stack and lets it go, so this
 * is recorded as the engine is constructed rather than read back off an object that is gone
 * by the time the call returns.
 **/
struct Built final {
    std::string path;
    int option = 0;
    std::string label;
};

Built* built = nullptr;

/**
 * What run<T> actually requires of an engine, which is three methods and a constructor -
 * not a v3d::engine::Engine, which cannot be built in a test without a window.
 **/
struct StubEngine final {
    StubEngine(const std::string& path, int option = 0, std::string label = std::string()) {
        if (built != nullptr) {
            built->path = path;
            built->option = option;
            built->label = std::move(label);
        }
    }

    bool initialize() {
        return true;
    }
    bool eventLoop() {
        return true;
    }
    bool shutdown() {
        return true;
    }
};

};  // namespace

BOOST_AUTO_TEST_SUITE(application_test)

/**
 * Everything an app loads is named relative to this, so it has to end with a separator -
 * the engine appends "data/" to it rather than joining a path onto it.
 **/
BOOST_AUTO_TEST_CASE(an_app_path_ends_with_a_separator) {
    const std::string path = v3d::engine::appPath("C:/somewhere/pong.exe");

    BOOST_REQUIRE(!path.empty());
    const char last = path[path.size() - 1];
    BOOST_TEST((last == '/' || last == '\\'));
}

/**
 * It is the directory the executable is in, not the executable.
 **/
BOOST_AUTO_TEST_CASE(an_app_path_drops_the_executable) {
    const std::string path = v3d::engine::appPath("C:/somewhere/pong.exe");

    BOOST_TEST(path.find("pong.exe") == std::string::npos);
    BOOST_TEST(path.find("somewhere") != std::string::npos);
}

/**
 * argv[0] is whatever the shell was given, so a game started from another directory has to
 * come back with the one it is actually installed in.
 **/
BOOST_AUTO_TEST_CASE(a_relative_argument_comes_back_absolute) {
    const std::string path = v3d::engine::appPath("pong.exe");

    BOOST_TEST(boost::filesystem::path(path).is_absolute());
    BOOST_TEST(path.find(boost::filesystem::current_path().filename().string()) != std::string::npos);
}

/**
 * The other half of the pair: where an app writes what the player chose.
 *
 * It has to be somewhere else than appPath() - that directory is overwritten from source on
 * every build - and it has to end with a separator for the same reason appPath() does, so
 * that a second asset::Manager can be rooted at it.
 **/
BOOST_AUTO_TEST_CASE(a_user_path_is_absolute_and_ends_with_a_separator) {
    const std::string path = v3d::engine::userPath("Vertical3D", "ApplicationTest");

    BOOST_REQUIRE(!path.empty());
    BOOST_TEST(boost::filesystem::path(path).is_absolute());

    const char last = path[path.size() - 1];
    BOOST_TEST((last == '/' || last == '\\'));

    // SDL creates it, which is what makes it writable without the app checking first
    BOOST_TEST(boost::filesystem::exists(boost::filesystem::path(path)));

    // and it is not where the app reads its assets from
    BOOST_TEST(path != v3d::engine::appPath("ApplicationTest.exe"));
}

/**
 * The app name separates two apps sharing an organization, which is what stops one game's
 * settings from being another's.
 **/
BOOST_AUTO_TEST_CASE(a_user_path_is_per_app) {
    const std::string one = v3d::engine::userPath("Vertical3D", "ApplicationTestOne");
    const std::string other = v3d::engine::userPath("Vertical3D", "ApplicationTestTwo");

    BOOST_REQUIRE(!one.empty());
    BOOST_REQUIRE(!other.empty());
    BOOST_TEST(one != other);
}

/**
 * An app whose engine is built from more than a path - a command line parsed into options
 * before the engine exists - hands them over here rather than writing its own main.
 **/
BOOST_AUTO_TEST_CASE(run_forwards_what_the_engine_is_built_from) {
    Built record;
    built = &record;

    BOOST_TEST(v3d::engine::run<StubEngine>("stub.exe", "stub", 7, std::string("scene.gltf")) == EXIT_SUCCESS);
    built = nullptr;

    BOOST_CHECK_EQUAL(record.option, 7);
    BOOST_CHECK_EQUAL(record.label, "scene.gltf");
    BOOST_CHECK(!record.path.empty());
}

/**
 * And an app that is built from nothing else still calls it the way it always did.
 **/
BOOST_AUTO_TEST_CASE(run_still_takes_a_path_alone) {
    Built record;
    record.option = 9;
    record.label = "stale";
    built = &record;

    BOOST_TEST(v3d::engine::run<StubEngine>("stub.exe", "stub") == EXIT_SUCCESS);
    built = nullptr;

    BOOST_CHECK_EQUAL(record.option, 0);
    BOOST_CHECK(record.label.empty());
}

BOOST_AUTO_TEST_SUITE_END()
