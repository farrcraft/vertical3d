/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#define BOOST_TEST_MODULE v3dlib_render_device
#define BOOST_TEST_NO_MAIN
#include <boost/test/unit_test.hpp>

#include "Headless.h"

/**
 * A suite that needs a device, and says so rather than passing when there is not one.
 *
 * Every case below the recorder needs a gpu, which a machine may not have. CI provides a
 * software implementation. The cases cannot be skipped from inside: a run where every case
 * skipped exits zero and reads as a pass. A validation layer that was never installed hides
 * errors in the same way.
 *
 * The probe therefore runs before the framework starts. With no device to draw with, the
 * process exits with the code that api/render/tests/CMakeLists.txt tells ctest to read as
 * "skipped", so the result shows whether the cases ran.
 **/
int main(int argc, char* argv[]) {
    if (!v3d::test::deviceAvailable()) {
        return v3d::test::skipExitCode;
    }
    return ::boost::unit_test::unit_test_main(&init_unit_test, argc, argv);
}
