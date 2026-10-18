/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/offline/sl/Types.h>

#include <boost/test/unit_test.hpp>

#include <glm/gtc/matrix_transform.hpp>

namespace {

typedef v3d::render::offline::sl::Type Type;

/**
 * A matrix that both rotates and translates, so the three transforms give different results.
 * Under a pure rotation ptransform and vtransform agree, and under a uniform scale vtransform
 * and ntransform agree.
 **/
glm::mat4x4 placed() {
    const glm::mat4x4 matrix = glm::translate(glm::mat4x4(1.0F), glm::vec3(10.0F, 20.0F, 30.0F));
    return glm::rotate(matrix, glm::radians(90.0F), glm::vec3(0.0F, 0.0F, 1.0F));
}

BOOST_AUTO_TEST_CASE(sltypes_components_test) {
    BOOST_CHECK_EQUAL(v3d::render::offline::sl::components(Type::FLOAT), 1U);
    BOOST_CHECK_EQUAL(v3d::render::offline::sl::components(Type::POINT), 3U);
    BOOST_CHECK_EQUAL(v3d::render::offline::sl::components(Type::COLOR), 3U);
    BOOST_CHECK_EQUAL(v3d::render::offline::sl::components(Type::MATRIX), 16U);
    // a string names a space, a texture or a message: there is no arithmetic to do on it
    BOOST_CHECK_EQUAL(v3d::render::offline::sl::components(Type::STRING), 0U);
}

/**
 * RI's coercions: a float replicates into anything made of floats, and the three point-like
 * types convert to each other. A colour does not convert to or from a position, so using
 * one as the other is an error.
 **/
BOOST_AUTO_TEST_CASE(sltypes_coercion_test) {
    BOOST_CHECK(v3d::render::offline::sl::coercible(Type::FLOAT, Type::POINT));
    BOOST_CHECK(v3d::render::offline::sl::coercible(Type::FLOAT, Type::COLOR));
    BOOST_CHECK(v3d::render::offline::sl::coercible(Type::FLOAT, Type::MATRIX));

    BOOST_CHECK(v3d::render::offline::sl::coercible(Type::POINT, Type::VECTOR));
    BOOST_CHECK(v3d::render::offline::sl::coercible(Type::VECTOR, Type::NORMAL));
    BOOST_CHECK(v3d::render::offline::sl::coercible(Type::NORMAL, Type::POINT));

    BOOST_CHECK(!v3d::render::offline::sl::coercible(Type::COLOR, Type::POINT));
    BOOST_CHECK(!v3d::render::offline::sl::coercible(Type::POINT, Type::COLOR));
    BOOST_CHECK(!v3d::render::offline::sl::coercible(Type::POINT, Type::FLOAT));
    BOOST_CHECK(!v3d::render::offline::sl::coercible(Type::STRING, Type::FLOAT));
    BOOST_CHECK(!v3d::render::offline::sl::coercible(Type::FLOAT, Type::STRING));
    BOOST_CHECK(v3d::render::offline::sl::coercible(Type::STRING, Type::STRING));
}

BOOST_AUTO_TEST_CASE(sltypes_arithmetic_test) {
    BOOST_CHECK(v3d::render::offline::sl::arithmetic(Type::FLOAT, Type::FLOAT) == Type::FLOAT);
    // a float is a scale over anything else, whichever side it is on
    BOOST_CHECK(v3d::render::offline::sl::arithmetic(Type::FLOAT, Type::COLOR) == Type::COLOR);
    BOOST_CHECK(v3d::render::offline::sl::arithmetic(Type::POINT, Type::FLOAT) == Type::POINT);
    // the point-like types mix, taking the left operand's type
    BOOST_CHECK(v3d::render::offline::sl::arithmetic(Type::POINT, Type::VECTOR) == Type::POINT);
    BOOST_CHECK(v3d::render::offline::sl::arithmetic(Type::VECTOR, Type::NORMAL) == Type::VECTOR);
    // a colour and a position have none between them
    BOOST_CHECK(v3d::render::offline::sl::arithmetic(Type::COLOR, Type::POINT) == Type::VOID);
    BOOST_CHECK(v3d::render::offline::sl::arithmetic(Type::STRING, Type::STRING) == Type::VOID);
}

/**
 * A point has a position, so a translation moves it.
 **/
BOOST_AUTO_TEST_CASE(sltypes_ptransform_test) {
    const glm::vec3 moved = v3d::render::offline::sl::ptransform(placed(), glm::vec3(1.0F, 0.0F, 0.0F));

    // the rotation takes +x to +y, and the translation then carries it
    BOOST_CHECK_CLOSE(moved.x, 10.0F, 0.01F);
    BOOST_CHECK_CLOSE(moved.y, 21.0F, 0.01F);
    BOOST_CHECK_CLOSE(moved.z, 30.0F, 0.01F);
}

/**
 * A direction has no position, so a translation does nothing to it. That is the only
 * difference between a vector and a point, and it is invisible until a scene translates.
 **/
BOOST_AUTO_TEST_CASE(sltypes_vtransform_test) {
    const glm::vec3 turned = v3d::render::offline::sl::vtransform(placed(), glm::vec3(1.0F, 0.0F, 0.0F));

    BOOST_CHECK_SMALL(turned.x, 0.0001F);
    BOOST_CHECK_CLOSE(turned.y, 1.0F, 0.01F);
    BOOST_CHECK_SMALL(turned.z, 0.0001F);
}

/**
 * A normal goes by the inverse transpose. Under a rotation a normal transforms as a vector
 * does; under a scale of one axis it does not. moya's dicing and its traced fan follow the
 * same rule.
 **/
BOOST_AUTO_TEST_CASE(sltypes_ntransform_test) {
    // under a rotation and a translation the three agree about direction
    const glm::vec3 turned = v3d::render::offline::sl::ntransform(placed(), glm::vec3(1.0F, 0.0F, 0.0F));
    BOOST_CHECK_SMALL(turned.x, 0.0001F);
    BOOST_CHECK_CLOSE(turned.y, 1.0F, 0.01F);
    BOOST_CHECK_SMALL(turned.z, 0.0001F);

    // under a scale of one axis they differ: the plane of a surface leans the opposite way
    // to the points that make it
    const glm::mat4x4 stretched = glm::scale(glm::mat4x4(1.0F), glm::vec3(1.0F, 2.0F, 1.0F));
    const glm::vec3 normal(1.0F, 1.0F, 0.0F);

    const glm::vec3 asVector = v3d::render::offline::sl::vtransform(stretched, normal);
    BOOST_CHECK_CLOSE(asVector.y, 2.0F, 0.01F);

    const glm::vec3 asNormal = v3d::render::offline::sl::ntransform(stretched, normal);
    BOOST_CHECK_CLOSE(asNormal.x, 1.0F, 0.01F);
    BOOST_CHECK_CLOSE(asNormal.y, 0.5F, 0.01F);
}

};  // namespace
