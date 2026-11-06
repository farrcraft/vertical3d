/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

// Boost.Test linked as a DLL, as on Windows, leaves main to the suite, which probes first. Linked
// statically, as on Linux, it brings its own main, and the probe runs in the function that main
// calls before any case
#ifdef BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE v3dlib_render_device
#define BOOST_TEST_NO_MAIN
#endif
#include <cstdlib>

#include <boost/test/unit_test.hpp>

#include "Headless.h"

/**
 * A suite that needs a device, and says so rather than passing when there is not one.
 *
 * Every case in this suite needs a GPU, which a machine may not have. CI provides a
 * software implementation. The cases cannot be skipped from inside: a run where every case
 * skipped exits zero and reads as a pass. A validation layer that was never installed hides
 * errors in the same way.
 *
 * The probe therefore runs before the framework starts. With no device to draw with, the
 * process exits with skipExitCode, which ctest reports as skipped, so the result shows whether
 * the cases ran.
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
    ::boost::unit_test::framework::master_test_suite().p_name.value = "v3dlib_render_device";
    return nullptr;
}
#endif
