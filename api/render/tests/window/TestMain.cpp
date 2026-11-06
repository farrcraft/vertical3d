/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

// Boost.Test linked as a DLL, as on Windows, leaves main to the suite, which probes first. Linked
// statically, as on Linux, it brings its own main, and the probe runs in the function that main
// calls before any case
#ifdef BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE v3dlib_render_window
#define BOOST_TEST_NO_MAIN
#endif
#include <cstdlib>

#include <boost/test/unit_test.hpp>

#include "Windowed.h"

/**
 * A suite that needs a window and a device that presents to it. Without them, it reports a skip
 * instead of passing.
 *
 * A machine may have no display, or no device that can present. The probe runs before the
 * framework starts, because a run where every case skipped exits zero and reads as a pass. With
 * no window, the process exits with skipExitCode, which ctest reports as skipped.
 **/
#ifdef BOOST_TEST_DYN_LINK
int main(int argc, char* argv[]) {
    if (!v3d::test::windowAvailable()) {
        return v3d::test::skipExitCode;
    }
    return ::boost::unit_test::unit_test_main(&init_unit_test, argc, argv);
}
#else
::boost::unit_test::test_suite* init_unit_test_suite(int /*argc*/, char* /*argv*/[]) {
    if (!v3d::test::windowAvailable()) {
        std::exit(v3d::test::skipExitCode);
    }
    ::boost::unit_test::framework::master_test_suite().p_name.value = "v3dlib_render_window";
    return nullptr;
}
#endif
