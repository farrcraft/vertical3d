/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <boost/test/unit_test.hpp>

#include "../Type.h"

/**
 * These are the names a config.json entry's "type" is written as. camera and layout are the
 * editor's alone; everything else is shared.
 **/
BOOST_AUTO_TEST_CASE(config_string_to_type_test) {
    BOOST_TEST((v3d::config::stringToType("window") == v3d::config::Type::Window));
    BOOST_TEST((v3d::config::stringToType("binding") == v3d::config::Type::Binding));
    BOOST_TEST((v3d::config::stringToType("ui") == v3d::config::Type::Ui));
    BOOST_TEST((v3d::config::stringToType("sound") == v3d::config::Type::Sound));
    BOOST_TEST((v3d::config::stringToType("camera") == v3d::config::Type::Camera));
    BOOST_TEST((v3d::config::stringToType("layout") == v3d::config::Type::Layout));
}

/**
 * Unknown is what load() rejects an entry on, so anything that is not one of the six above -
 * a case difference included - has to land there rather than on a neighbouring type.
 **/
BOOST_AUTO_TEST_CASE(config_unknown_type_name_test) {
    BOOST_TEST((v3d::config::stringToType("") == v3d::config::Type::Unknown));
    BOOST_TEST((v3d::config::stringToType("menu") == v3d::config::Type::Unknown));
    BOOST_TEST((v3d::config::stringToType("Window") == v3d::config::Type::Unknown));
    BOOST_TEST((v3d::config::stringToType("windows") == v3d::config::Type::Unknown));
}

/**
 * The mapping is constexpr, and a config type is picked at compile time wherever the name is
 * a literal.
 **/
BOOST_AUTO_TEST_CASE(config_string_to_type_is_constexpr_test) {
    static_assert(v3d::config::stringToType("ui") == v3d::config::Type::Ui);
    static_assert(v3d::config::stringToType("nothing") == v3d::config::Type::Unknown);
    BOOST_TEST(true);
}
