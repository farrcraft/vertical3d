/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/engine/Application.h>
#include <api/engine/Identity.h>
#include <api/engine/Launch.h>
#include <api/asset/Manager.h>
#include <api/log/Logger.h>
#include <api/version/Version.h>

#include <SDL3/SDL_init.h>

#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/make_shared.hpp>
#include <boost/system/error_code.hpp>

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
    v3d::engine::Launch launch;
    bool launchedFirst = false;
};

/**
 * What run<T> requires of an engine: a constructor, identity(), launch(), initialize(),
 * eventLoop() and shutdown(). It does not require a v3d::engine::Engine, which cannot run its
 * loop in a test.
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

    void launch(const v3d::engine::Launch& launch) {
        built_->launch = launch;
        built_->launchedFirst = !initialized_;
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
    static void launch(const v3d::engine::Launch& /*launch*/) {
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
    const std::string path = v3d::engine::appPath();

    BOOST_REQUIRE(!path.empty());
    const char last = path[path.size() - 1];
    BOOST_TEST((last == '/' || last == '\\'));
}

/**
 * An app started from a shortcut or from PATH has some other working directory, and still
 * finds the directory it is installed in.
 **/
BOOST_AUTO_TEST_CASE(an_app_path_does_not_follow_the_working_directory) {
    const boost::filesystem::path working = boost::filesystem::current_path();
    const std::string before = v3d::engine::appPath();
    boost::filesystem::current_path(boost::filesystem::temp_directory_path());
    const std::string after = v3d::engine::appPath();
    boost::filesystem::current_path(working);

    BOOST_TEST(boost::filesystem::path(before).is_absolute());
    BOOST_CHECK_EQUAL(after, before);
}

/**
 * The app's own assets are read before the shared ones, so an app's file of the same name
 * wins.
 **/
BOOST_AUTO_TEST_CASE(an_apps_own_data_is_searched_before_the_shared_data) {
    const std::vector<std::string> roots = v3d::engine::dataRoots("C:/games/", { .program = "pong" });

    BOOST_REQUIRE_EQUAL(roots.size(), 2U);
    BOOST_CHECK_EQUAL(roots[0], "C:/games/data/pong/");
    BOOST_CHECK_EQUAL(roots[1], "C:/games/data/");
}

/**
 * An engine with no command line, as in a test, has no program name and reads data/ alone.
 **/
BOOST_AUTO_TEST_CASE(an_engine_with_no_program_reads_the_shared_data_alone) {
    const std::vector<std::string> roots = v3d::engine::dataRoots("C:/games/", v3d::engine::Launch());

    BOOST_REQUIRE_EQUAL(roots.size(), 1U);
    BOOST_CHECK_EQUAL(roots[0], "C:/games/data/");
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
    BOOST_TEST(path != v3d::engine::appPath());
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
 * An installed app's directory may not be writable, so a named app logs to its user directory.
 **/
BOOST_AUTO_TEST_CASE(a_named_app_logs_to_its_user_directory) {
    BOOST_CHECK_EQUAL(v3d::engine::logPath(stub()), v3d::engine::userPath("vertical3d_tests", "stub") + "v3d.log");
}

/**
 * An app with no identity has no user directory, and logs beside the executable.
 **/
BOOST_AUTO_TEST_CASE(an_unnamed_app_logs_beside_the_executable) {
    BOOST_CHECK_EQUAL(v3d::engine::logPath(v3d::engine::Identity()), v3d::engine::appPath() + "v3d.log");
}

/**
 * run<T> opens the log before the engine is built, in the user directory.
 **/
BOOST_AUTO_TEST_CASE(run_writes_the_log_in_the_user_directory) {
    Built record;

    BOOST_TEST(v3d::engine::run<StubEngine>("stub.exe", stub(), &record) == EXIT_SUCCESS);

    BOOST_TEST(boost::filesystem::exists(boost::filesystem::path(v3d::engine::logPath(stub()))));
    v3d::log::Logger::open("v3d.log");
}

/**
 * A user's documents go in a directory named for the organization, in their documents folder,
 * which is made for them.
 **/
BOOST_AUTO_TEST_CASE(documents_go_in_a_directory_named_for_the_organization) {
    const v3d::engine::Identity identity{ .organization = "vertical3d_tests_documents", .application = "stub" };

    const std::string path = v3d::engine::documentsPath(identity);

    BOOST_REQUIRE(!path.empty());
    BOOST_TEST(boost::filesystem::path(path).is_absolute());
    BOOST_TEST(boost::filesystem::is_directory(boost::filesystem::path(path)));
    BOOST_CHECK_EQUAL(boost::filesystem::path(path).parent_path().filename().string(), "vertical3d_tests_documents");
    BOOST_TEST(path != v3d::engine::appPath());

    boost::system::error_code error;
    boost::filesystem::remove(boost::filesystem::path(path), error);
}

/**
 * An app with no identity has no documents directory, and the caller chooses where to go.
 **/
BOOST_AUTO_TEST_CASE(an_unnamed_app_has_no_documents_directory) {
    BOOST_TEST(v3d::engine::documentsPath(v3d::engine::Identity()).empty());
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
 * Given the command line, run<T> reads it and gives the engine what it asks for before the
 * engine starts. The arguments the engine does not read are kept for the app.
 **/
BOOST_AUTO_TEST_CASE(run_gives_the_engine_its_command_line_before_it_starts) {
    Built record;
    const char* arguments[] = { "stub.exe", "--capture", "frame.png", "--script", "scene.ts" };

    BOOST_TEST(v3d::engine::run<StubEngine>(5, const_cast<char**>(arguments), stub(), &record) == EXIT_SUCCESS);

    BOOST_CHECK_EQUAL(record.launch.program, "stub");
    BOOST_CHECK_EQUAL(record.launch.capture, "frame.png");
    BOOST_CHECK_EQUAL(record.launch.option("--script").value_or(""), "scene.ts");
    BOOST_CHECK(record.launchedFirst);
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
 * --version prints the version and nothing else: the engine is never built, so no window
 * opens.
 **/
BOOST_AUTO_TEST_CASE(run_answers_the_version_without_starting_the_app) {
    Built record;
    const char* arguments[] = { "stub.exe", "--version" };

    BOOST_TEST(v3d::engine::run<StubEngine>(2, const_cast<char**>(arguments), stub(), &record) == EXIT_SUCCESS);

    BOOST_TEST(record.path.empty());
}

/**
 * run<T> names the app and its version to SDL before the engine is built, so the renderer can
 * name the app to Vulkan.
 **/
BOOST_AUTO_TEST_CASE(run_names_the_app_and_its_version_to_sdl) {
    Built record;

    BOOST_TEST(v3d::engine::run<StubEngine>("stub.exe", stub(), &record) == EXIT_SUCCESS);

    BOOST_CHECK_EQUAL(std::string(SDL_GetAppMetadataProperty(SDL_PROP_APP_METADATA_NAME_STRING)), "stub");
    BOOST_CHECK_EQUAL(std::string(SDL_GetAppMetadataProperty(SDL_PROP_APP_METADATA_VERSION_STRING)),
                      std::string(v3d::version::text()));
}

/**
 * Given only argv[0], run<T> still names the program, so the app finds its own data.
 **/
BOOST_AUTO_TEST_CASE(run_names_the_program_from_the_executable_alone) {
    Built record;

    BOOST_TEST(v3d::engine::run<StubEngine>("C:/somewhere/stub.exe", stub(), &record) == EXIT_SUCCESS);

    BOOST_CHECK_EQUAL(record.launch.program, "stub");
    BOOST_CHECK_EQUAL(record.path, v3d::engine::appPath());
}

/**
 * An app whose engine takes only the path passes no further arguments, and the engine is built
 * from a path that is not empty.
 **/
BOOST_AUTO_TEST_CASE(run_still_takes_a_path_alone) {
    BOOST_TEST(v3d::engine::run<PathOnlyEngine>("stub.exe", stub()) == EXIT_SUCCESS);
}

/**
 * A narrow path is UTF-8. A log and an asset under a directory whose name the ANSI code page
 * cannot hold are still written and read there.
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

    v3d::log::Logger::open(narrow + "/v3d.log");
    BOOST_TEST(boost::filesystem::exists(directory / L"v3d.log"));

    {
        auto logger = boost::make_shared<v3d::log::Logger>();
        v3d::asset::Manager manager(narrow + "/", logger);
        BOOST_TEST(manager.load("found.txt") != nullptr);
    }

    // the log file stays open while any logger holds it, so it is moved back before the
    // directory goes
    v3d::log::Logger::open("v3d.log");
    boost::filesystem::remove_all(directory);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
