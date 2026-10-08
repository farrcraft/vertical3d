/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#define BOOST_TEST_MODULE v3dlib_render_window
#define BOOST_TEST_NO_MAIN
#include <boost/test/unit_test.hpp>

#include "Windowed.h"

/**
 * A suite that needs a window and a device that presents to it, and says so rather than passing
 * when there is not one.
 *
 * A machine may have no display, or no device that can present. The probe runs before the
 * framework starts, because a run where every case skipped exits zero and reads as a pass. With
 * no window, the process exits with the code that api/render/tests/CMakeLists.txt tells ctest to
 * read as "skipped".
 **/
int main(int argc, char* argv[]) {
    if (!v3d::test::windowAvailable()) {
        return v3d::test::skipExitCode;
    }
    return ::boost::unit_test::unit_test_main(&init_unit_test, argc, argv);
}
