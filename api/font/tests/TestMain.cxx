/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#define BOOST_TEST_MODULE v3dlib_font
#include <unicode/uclean.h>

#include <boost/test/unit_test.hpp>

namespace {

/**
 * Frees ICU's caches once every test has run. ICU keeps the rules and dictionaries it loads
 * until the process ends or u_cleanup() is called, and the debug runtime would otherwise report
 * them as leaks.
 **/
struct IcuCleanup {
    IcuCleanup() = default;
    IcuCleanup(const IcuCleanup&) = delete;
    IcuCleanup& operator=(const IcuCleanup&) = delete;
    ~IcuCleanup() {
        u_cleanup();
    }
};

BOOST_TEST_GLOBAL_FIXTURE(IcuCleanup);

};  // namespace
