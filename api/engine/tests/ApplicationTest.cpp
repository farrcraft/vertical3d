/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <string>

#include <boost/test/unit_test.hpp>

#include "../Application.h"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

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

BOOST_AUTO_TEST_SUITE_END()
