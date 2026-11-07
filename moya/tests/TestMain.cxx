/*
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 */

#define BOOST_TEST_MODULE v3dlib_moya
#include <tbb/global_control.h>

#include <new>

#include <boost/test/unit_test.hpp>

namespace {

/**
 * Waits for TBB's worker threads to end once every test has run. A CPU device's launch starts
 * them, and they otherwise outlive main, so the debug runtime would report what they hold as
 * leaks.
 **/
struct TbbCleanup {
    TbbCleanup() = default;
    TbbCleanup(const TbbCleanup&) = delete;
    TbbCleanup& operator=(const TbbCleanup&) = delete;
    ~TbbCleanup() {
        tbb::finalize(handle_, std::nothrow);
    }

 private:
    tbb::task_scheduler_handle handle_ { tbb::attach {} };
};

BOOST_TEST_GLOBAL_FIXTURE(TbbCleanup);

};  // namespace
