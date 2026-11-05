/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/version/Version.h>

#include <string>
#include <string_view>

#include <boost/test/unit_test.hpp>

namespace {

/** The numbers as text, as a release shows them. **/
std::string dotted(const v3d::version::Number& number) {
    return std::to_string(number.major) + "." + std::to_string(number.minor) + "." + std::to_string(number.patch);
}

/**
 * The text a user sees starts with the release's numbers, so the two never disagree.
 **/
BOOST_AUTO_TEST_CASE(the_text_starts_with_the_number_test) {
    const std::string_view text = v3d::version::text();
    const std::string number = dotted(v3d::version::number());

    BOOST_TEST(text.substr(0, number.size()) == number);
}

/**
 * A build that is not a release says so after the number, as a semantic version's pre-release
 * part, so it sorts before the release it is building toward.
 **/
BOOST_AUTO_TEST_CASE(a_build_that_is_not_a_release_is_marked_as_one_test) {
    const std::string_view text = v3d::version::text();
    const std::string number = dotted(v3d::version::number());

    BOOST_TEST((text == number || text.substr(number.size(), 4) == "-dev"));
}

/**
 * The tree builds toward a release with a number, and 0.0.0 is not one.
 **/
BOOST_AUTO_TEST_CASE(the_number_is_a_release_test) {
    const v3d::version::Number number = v3d::version::number();

    BOOST_TEST(number.major + number.minor + number.patch > 0U);
}

};  // namespace
