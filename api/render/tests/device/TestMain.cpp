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
 * Every case below the recorder needs a gpu, which a machine or a runner may not have - ADR-0007
 * puts a software implementation on the runner for exactly this reason. The cases cannot simply
 * be skipped from inside, because a run where every case skipped exits zero and reads as a pass,
 * which is the same trap as a validation layer that was never installed reporting no errors.
 *
 * So the probe happens before the framework starts, and a tree with no device it can draw with
 * exits with the code ctest is told to read as "skipped" in api/render/tests/CMakeLists.txt. The
 * run then says which of the two it was.
 **/
int main(int argc, char* argv[]) {
    if (!v3d::test::deviceAvailable()) {
        return v3d::test::skipExitCode;
    }
    return ::boost::unit_test::unit_test_main(&init_unit_test, argc, argv);
}
