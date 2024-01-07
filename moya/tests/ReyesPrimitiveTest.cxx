/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include <boost/test/unit_test.hpp>

#include "../libmoya/ReyesPrimitive.h"

BOOST_AUTO_TEST_CASE(reyesPrimitive_test) {
    v3D::Moya::ReyesPrimitive primitive;

    primitive.diceable(true);

    bool diceable = primitive.diceable();

    BOOST_CHECK_EQUAL(diceable, true);

    primitive.diceable(false);

    diceable = primitive.diceable();

    BOOST_CHECK_EQUAL(diceable, false);
}
