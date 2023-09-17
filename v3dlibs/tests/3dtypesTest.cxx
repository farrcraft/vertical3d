#include <boost/test/unit_test.hpp>

#include "../3dtypes/3dtypes.h"

BOOST_AUTO_TEST_CASE( types_test )
{
    // test radian to degree conversion
    float degrees = rad2deg<float>(PI);
    BOOST_CHECK_EQUAL(degrees, 180.0f);

    // test degree to radian conversion
    float radians = deg2rad<float>(360.0f);
    BOOST_CHECK_EQUAL(radians, (PI * 2.0f));

    // test float comparison
    bool comparison = float_eq<float>(123.46f, 123.46f);
    // [FIXME] this test is not working right now
    //BOOST_CHECK_EQUAL(comparison, true);
    comparison = float_eq<float>(78.9f, 53.1f);
    BOOST_CHECK_EQUAL(comparison, false);
    comparison = float_eq<float>(13.8f, 91.3f);
    BOOST_CHECK_EQUAL(comparison, false);

    // test swap
    float a = 291.3f;
    float b = 17.33f;
    swap<float>(a, b);
    BOOST_CHECK_EQUAL(a, 17.33f);
    BOOST_CHECK_EQUAL(b, 291.3f);
}