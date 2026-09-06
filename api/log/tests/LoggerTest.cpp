/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <spdlog/spdlog.h>

#include <memory>
#include <string>

#include <boost/test/unit_test.hpp>

#include "../Logger.h"

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
 * get() hands back the reference the wrapper holds, so a caller that reseats it - which is
 * what a test double would do - is seen by the next call rather than by a copy.
 **/
BOOST_AUTO_TEST_CASE(logger_get_is_a_reference_test) {
    v3d::log::Logger logger;
    std::shared_ptr<spdlog::logger>& held = logger.get();

    BOOST_TEST(held.get() == logger.get().get());
}
