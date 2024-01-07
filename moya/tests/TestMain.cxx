#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Master Test Suite
#include <boost/test/unit_test.hpp>

// include log4cxx header files.
#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/helpers/exception.h>

/**
 * Global Test Fixture used for initializing the logger
 */
struct LogInitializerFixture
{
    LogInitializerFixture()
    {
        log4cxx::BasicConfigurator::configure();
    }
};

BOOST_GLOBAL_FIXTURE(LogInitializerFixture)
