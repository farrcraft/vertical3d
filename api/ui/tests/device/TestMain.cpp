/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

// Boost.Test linked as a DLL, as on Windows, leaves main to the suite, which probes first. Linked
// statically, as on Linux, it brings its own main, and the probe runs in the function that main
// calls before any case
#ifdef BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE ui_device
#define BOOST_TEST_NO_MAIN
#endif
#include <api/render/tests/device/Headless.h>

#include <cstdlib>

#include <boost/test/unit_test.hpp>

/**
 * The ui's cases that draw on a device, which a machine may not have.
 *
 * As the render device suite does, the probe runs before the framework starts, and a machine
 * with no device exits with skipExitCode, which ctest reports as skipped rather than passed.
 **/
#ifdef BOOST_TEST_DYN_LINK
int main(int argc, char* argv[]) {
    if (!v3d::test::deviceAvailable()) {
        return v3d::test::skipExitCode;
    }
    return ::boost::unit_test::unit_test_main(&init_unit_test, argc, argv);
}
#else
::boost::unit_test::test_suite* init_unit_test_suite(int /*argc*/, char* /*argv*/[]) {
    if (!v3d::test::deviceAvailable()) {
        std::exit(v3d::test::skipExitCode);
    }
    ::boost::unit_test::framework::master_test_suite().p_name.value = "ui_device";
    return nullptr;
}
#endif
