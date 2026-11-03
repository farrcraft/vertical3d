/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/engine/Launch.h>

#include <stdexcept>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

namespace {

v3d::engine::Launch parse(const std::vector<const char*>& arguments) {
    std::vector<const char*> argv{ "app.exe" };
    argv.insert(argv.end(), arguments.begin(), arguments.end());
    return v3d::engine::Launch::parse(static_cast<int>(argv.size()), argv.data());
}

BOOST_AUTO_TEST_CASE(launch_with_no_options_captures_nothing_test) {
    const v3d::engine::Launch launch = parse({});
    BOOST_TEST(launch.capture.empty());
    BOOST_CHECK_EQUAL(launch.captureAfter, v3d::engine::Launch::CAPTURE_AFTER);
    BOOST_TEST(launch.rest.empty());
    BOOST_TEST(!launch.due(v3d::engine::Launch::CAPTURE_AFTER));
}

BOOST_AUTO_TEST_CASE(launch_reads_the_capture_file_and_frame_test) {
    const v3d::engine::Launch launch = parse({ "--capture", "frame.png", "--capture-after", "12" });
    BOOST_CHECK_EQUAL(launch.capture, "frame.png");
    BOOST_CHECK_EQUAL(launch.captureAfter, 12U);
    BOOST_TEST(!launch.due(11));
    BOOST_TEST(launch.due(12));
    BOOST_TEST(!launch.due(13));
}

BOOST_AUTO_TEST_CASE(launch_keeps_every_other_argument_in_order_for_the_app_test) {
    const v3d::engine::Launch launch = parse({ "--script", "scene.ts", "--capture", "f.png", "loose" });
    const std::vector<std::string> expected{ "--script", "scene.ts", "loose" };
    BOOST_TEST(launch.rest == expected, boost::test_tools::per_element());
    BOOST_CHECK_EQUAL(launch.option("--script").value_or(""), "scene.ts");
    BOOST_TEST(!launch.option("--missing").has_value());
    BOOST_TEST(!launch.option("loose").has_value());
}

BOOST_AUTO_TEST_CASE(launch_refuses_a_capture_with_no_file_test) {
    BOOST_CHECK_THROW(parse({ "--capture" }), std::invalid_argument);
    BOOST_CHECK_THROW(parse({ "--capture", "" }), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(launch_refuses_a_frame_that_is_not_a_whole_number_from_one_test) {
    for (const char* frame : { "0", "-3", "2.5", "ten", "12x", "" }) {
        BOOST_TEST_CONTEXT(frame) {
            BOOST_CHECK_THROW(parse({ "--capture", "f.png", "--capture-after", frame }), std::invalid_argument);
        }
    }
    BOOST_CHECK_THROW(parse({ "--capture-after" }), std::invalid_argument);
}

};  // namespace
