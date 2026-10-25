/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#define BOOST_TEST_MODULE vertical3d_device
#define BOOST_TEST_NO_MAIN
#include <api/render/tests/device/Headless.h>

#include <boost/test/unit_test.hpp>

/**
 * The editor's cases that draw on a device, which a machine may not have.
 *
 * As the render device suite does, the probe runs before the framework starts, and a machine
 * with no device exits with skipExitCode, which ctest reports as skipped rather than passed.
 **/
int main(int argc, char* argv[]) {
    if (!v3d::test::deviceAvailable()) {
        return v3d::test::skipExitCode;
    }
    return ::boost::unit_test::unit_test_main(&init_unit_test, argc, argv);
}
