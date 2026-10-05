/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/log/Logger.h>

#include <spdlog/spdlog.h>

#include <cstdio>
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
 * spdlog throws on a second registration under the same name, and apps really do build two -
 * so the second Logger takes over the one already registered rather than bringing the
 * process down.
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
 * open() is where the log goes from then on, which is how run() puts it beside the executable
 * rather than wherever the app was started from.
 **/
BOOST_AUTO_TEST_CASE(logger_open_moves_the_log_test) {
    const std::string path = "logger_open_test.log";
    std::remove(path.c_str());

    v3d::log::Logger::open(path);
    v3d::log::Logger logger;
    logger.get()->info("opened");
    logger.get()->flush();
    BOOST_TEST(boost::filesystem::exists(path));

    // back to the default for whatever runs after
    v3d::log::Logger::open("v3d.log");
}
