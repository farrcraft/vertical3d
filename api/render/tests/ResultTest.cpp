/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/vulkan/device/Result.h>

#include <stdexcept>
#include <string>

#include <boost/test/unit_test.hpp>

using v3d::render::realtime::vulkan::device::check;

BOOST_AUTO_TEST_SUITE(result_test)

/**
 * A failure names what was being attempted and why it failed, because a log line from a
 * user's machine carries nothing else.
 **/
BOOST_AUTO_TEST_CASE(a_failed_call_throws_what_and_why) {
    try {
        check(VK_ERROR_OUT_OF_HOST_MEMORY, "Unable to create a vulkan sampler");
        BOOST_FAIL("a failed call did not throw");
    } catch (const std::runtime_error& error) {
        BOOST_CHECK_EQUAL(std::string(error.what()), "Unable to create a vulkan sampler - out of host memory");
    }
}

/**
 * Success passes, and so does the one other result a call names as tolerated. No other result
 * passes.
 **/
BOOST_AUTO_TEST_CASE(success_and_what_is_tolerated_pass) {
    BOOST_CHECK_NO_THROW(check(VK_SUCCESS, "Unable to count"));
    BOOST_CHECK_NO_THROW(check(VK_INCOMPLETE, "Unable to count", VK_INCOMPLETE));
    BOOST_CHECK_THROW(check(VK_INCOMPLETE, "Unable to count"), std::runtime_error);
    BOOST_CHECK_THROW(check(VK_TIMEOUT, "Unable to count", VK_INCOMPLETE), std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END()
