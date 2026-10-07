/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/log/Logger.h>

#include <spdlog/spdlog.h>

#include <cstdio>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>

#include <boost/filesystem/operations.hpp>
#include <boost/test/unit_test.hpp>

/**
 * One name in spdlog's global registry, and the levels a wrapper sets that a default logger
 * would not: debug to record everything, and a flush from info up so a process that stops
 * responding has already written what it was doing.
 **/
BOOST_AUTO_TEST_CASE(logger_registers_under_one_name_test) {
    v3d::log::Logger logger;

    BOOST_TEST(static_cast<bool>(logger.get()));
    BOOST_CHECK_EQUAL(logger.get()->name(), std::string("v3d-logger"));
    BOOST_TEST(logger.get()->level() == spdlog::level::debug);
    BOOST_TEST(logger.get()->flush_level() == spdlog::level::info);
    BOOST_TEST(static_cast<bool>(spdlog::get("v3d-logger")));
}

/**
 * spdlog throws on a second registration under the same name, and apps do construct two
 * Loggers. The second takes over the one already registered rather than ending the process.
 **/
BOOST_AUTO_TEST_CASE(logger_second_instance_shares_the_first_test) {
    v3d::log::Logger first;
    v3d::log::Logger second;

    BOOST_TEST(first.get().get() == second.get().get());
}

/**
 * The sink is what every consumer logs through, so it has to accept a formatted line at each
 * level without throwing.
 **/
BOOST_AUTO_TEST_CASE(logger_writes_at_every_level_test) {
    v3d::log::Logger logger;

    BOOST_CHECK_NO_THROW(logger.get()->debug("debug {}", 1));
    BOOST_CHECK_NO_THROW(logger.get()->info("info {}", "text"));
    BOOST_CHECK_NO_THROW(logger.get()->warn("warn {:.2f}", 1.5f));
    BOOST_CHECK_NO_THROW(logger.get()->error("error {} {}", 1, 2));
    BOOST_CHECK_NO_THROW(logger.get()->flush());
}

/**
 * open() is where the log goes from then on. run() uses it to put the log beside the
 * executable rather than wherever the app was started from.
 **/
BOOST_AUTO_TEST_CASE(logger_open_moves_the_log_test) {
    const std::string path = "logger_open_test.log";
    std::remove(path.c_str());

    BOOST_TEST(v3d::log::Logger::open(path));
    {
        // a handle keeps the file open, so it is let go of before the file is removed below
        v3d::log::Logger logger;
        logger.get()->info("written to the moved log");
        logger.get()->flush();
    }

    // the file exists as soon as it is opened, so it is the line in it that shows the log moved
    std::string contents;
    {
        std::ifstream file(path);
        contents.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    }
    BOOST_TEST(contents.find("written to the moved log") != std::string::npos);

    // back to the default for whatever runs after. With no handle left on the test's log, that
    // closes its file, and the removal can succeed
    v3d::log::Logger::open("v3d.log");
    BOOST_CHECK_EQUAL(std::remove(path.c_str()), 0);
}

/**
 * A path that cannot be opened does not throw. The log goes to stderr instead, so an app in a
 * directory it cannot write to still starts.
 **/
BOOST_AUTO_TEST_CASE(logger_open_falls_back_when_the_file_cannot_be_opened_test) {
    // a directory is not a file the log can be opened as
    const std::string path = "logger_open_test_directory";
    boost::filesystem::create_directory(path);

    bool opened = true;
    BOOST_CHECK_NO_THROW(opened = v3d::log::Logger::open(path));
    BOOST_TEST(!opened);
    v3d::log::Logger logger;
    BOOST_CHECK_NO_THROW(logger.get()->info("still logging"));

    v3d::log::Logger::open("v3d.log");
    boost::filesystem::remove(path);
}
