/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/config/Type.h>

#include <boost/test/unit_test.hpp>

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
    BOOST_TEST((v3d::config::stringToType("sprite") == v3d::config::Type::Sprite));
}

/**
 * load() files an entry whose type is Unknown under its own name, for the app to ask for.
 * Anything that is not one of the seven above, a case difference included, must map to Unknown
 * rather than to a neighbouring type.
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

/**
 * typeName is the inverse of stringToType for every type the api reads, and Unknown has no
 * name, so Config::get(Type::Unknown) finds nothing.
 **/
BOOST_AUTO_TEST_CASE(config_type_name_round_trip_test) {
    const v3d::config::Type types[] = {
        v3d::config::Type::Window,
        v3d::config::Type::Binding,
        v3d::config::Type::Ui,
        v3d::config::Type::Sound,
        v3d::config::Type::Camera,
        v3d::config::Type::Layout,
        v3d::config::Type::Sprite,
    };
    for (const v3d::config::Type type : types) {
        BOOST_TEST(!v3d::config::typeName(type).empty());
        BOOST_TEST((v3d::config::stringToType(v3d::config::typeName(type)) == type));
    }
    BOOST_TEST(v3d::config::typeName(v3d::config::Type::Unknown).empty());
    static_assert(v3d::config::typeName(v3d::config::Type::Sprite) == "sprite");
}
