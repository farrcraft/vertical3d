/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/engine/Application.h>
#include <api/engine/Identity.h>
#include <api/asset/Manager.h>
#include <api/log/Logger.h>

#include <cstdlib>
#include <string>
#include <utility>

#include <boost/test/unit_test.hpp>

#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/make_shared.hpp>

namespace {

/**
 * What run<T> built its engine with. run<T> builds the engine on the stack, and it is gone by
 * the time the call returns. A case therefore hands the engine one of these, and the engine
 * records into it what it was built and named with.
 **/
struct Built final {
    std::string path;
    int option = 0;
    std::string label;
    v3d::engine::Identity identity;
    bool namedFirst = false;
};

/**
 * What run<T> requires of an engine: a constructor, identity(), initialize(), eventLoop() and
 * shutdown(). It does not require a v3d::engine::Engine, which cannot run its loop in a test.
 **/
struct StubEngine final {
    StubEngine(const std::string& path, Built* built, int option = 0, std::string label = std::string()) :
        built_(built) {
        built_->path = path;
        built_->option = option;
        built_->label = std::move(label);
    }

    void identity(const v3d::engine::Identity& identity) {
        built_->identity = identity;
        built_->namedFirst = !initialized_;
    }

    bool initialize() {
        initialized_ = true;
        return true;
    }

    static bool eventLoop() {
        return true;
    }
    static bool shutdown() {
        return true;
    }

 private:
    Built* built_;
    // whether initialize() has run, so a case can tell the order run<T> calls them in
    bool initialized_ = false;
};

/**
 * An engine whose constructor takes the path and nothing else, as most apps' engines do.
 * run<T> forwarding anything more would not compile.
 **/
struct PathOnlyEngine final {
    explicit PathOnlyEngine(const std::string& path) : path_(path) {
    }

    static void identity(const v3d::engine::Identity& /*identity*/) {
    }
    bool initialize() const {
        return !path_.empty();
    }
    static bool eventLoop() {
        return true;
    }
    static bool shutdown() {
        return true;
    }

 private:
    std::string path_;
};

/** The identity the cases run the stub under. **/
v3d::engine::Identity stub() {
    return { .organization = "vertical3d_tests", .application = "stub" };
}

BOOST_AUTO_TEST_SUITE(application_test)

/**
 * Everything an app loads is named relative to this, so it has to end with a separator. The
 * engine appends "data/" to it as a string, not as a path join.
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
 * userPath() is where an app writes what the player chose.
 *
 * It has to differ from appPath(), which every build overwrites. It ends with a separator,
 * as appPath() does, so that a second asset::Manager can be rooted at it.
 **/
BOOST_AUTO_TEST_CASE(a_user_path_is_absolute_and_ends_with_a_separator) {
    const std::string path = v3d::engine::userPath("Vertical3D", "ApplicationTest");

    BOOST_REQUIRE(!path.empty());
    BOOST_TEST(boost::filesystem::path(path).is_absolute());

    const char last = path[path.size() - 1];
    BOOST_TEST((last == '/' || last == '\\'));

    // SDL creates it, so the app can write to it without checking first
    BOOST_TEST(boost::filesystem::exists(boost::filesystem::path(path)));

    // it is not where the app reads its assets from
    BOOST_TEST(path != v3d::engine::appPath("ApplicationTest.exe"));
}

/**
 * The app name separates two apps that share an organization, so one game's settings are
 * not another's.
 **/
BOOST_AUTO_TEST_CASE(a_user_path_is_per_app) {
    const std::string one = v3d::engine::userPath("Vertical3D", "ApplicationTestOne");
    const std::string other = v3d::engine::userPath("Vertical3D", "ApplicationTestTwo");

    BOOST_REQUIRE(!one.empty());
    BOOST_REQUIRE(!other.empty());
    BOOST_TEST(one != other);
}

/**
 * An app whose engine needs more than a path, such as options parsed from the command line,
 * passes them to run<T>. It does not write its own main.
 **/
BOOST_AUTO_TEST_CASE(run_forwards_what_the_engine_is_built_from) {
    Built record;

    BOOST_TEST(v3d::engine::run<StubEngine>("stub.exe", stub(), &record, 7, std::string("scene.gltf")) == EXIT_SUCCESS);

    BOOST_CHECK_EQUAL(record.option, 7);
    BOOST_CHECK_EQUAL(record.label, "scene.gltf");
    BOOST_CHECK(!record.path.empty());
}

/**
 * The engine is told who the app is before it starts, so it can read the player's settings
 * as it starts.
 **/
BOOST_AUTO_TEST_CASE(run_names_the_engine_before_it_starts) {
    Built record;

    BOOST_TEST(v3d::engine::run<StubEngine>("stub.exe", stub(), &record) == EXIT_SUCCESS);

    BOOST_CHECK_EQUAL(record.identity.organization, "vertical3d_tests");
    BOOST_CHECK_EQUAL(record.identity.application, "stub");
    BOOST_CHECK(record.namedFirst);
}

/**
 * An app whose engine takes only the path passes no further arguments, and the engine is built
 * from a path that is not empty.
 **/
BOOST_AUTO_TEST_CASE(run_still_takes_a_path_alone) {
    BOOST_TEST(v3d::engine::run<PathOnlyEngine>("stub.exe", stub()) == EXIT_SUCCESS);
}

/**
 * A narrow path is UTF-8. A game installed under a directory whose name the ANSI code page
 * cannot hold still writes its log there and finds its assets there.
 *
 * The directory is made and checked through wide paths, so the test does not depend on the
 * conversion it is testing.
 **/
BOOST_AUTO_TEST_CASE(a_path_outside_the_ansi_code_page_is_written_and_read) {
    // "日本", which no Western ANSI code page can hold
    const boost::filesystem::path directory = boost::filesystem::current_path() / L"日本";
    const std::string narrow = boost::filesystem::current_path().string() + "/\xE6\x97\xA5\xE6\x9C\xAC";
    boost::filesystem::remove_all(directory);
    boost::filesystem::create_directory(directory);
    {
        boost::filesystem::ofstream asset(directory / L"found.txt");
        asset << "found";
    }

    Built record;
    BOOST_TEST(v3d::engine::run<StubEngine>((narrow + "/stub.exe").c_str(), stub(), &record) == EXIT_SUCCESS);

    BOOST_TEST(boost::filesystem::exists(directory / L"v3d.log"));

    {
        auto logger = boost::make_shared<v3d::log::Logger>();
        v3d::asset::Manager manager(record.path, logger);
        BOOST_TEST(manager.load("found.txt") != nullptr);
    }

    // the log file stays open while any logger holds it, so it is moved back before the
    // directory goes
    v3d::log::Logger::open("v3d.log");
    boost::filesystem::remove_all(directory);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
