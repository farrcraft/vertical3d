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
 * A matrix that both rotates and translates, which is the only kind that tells the three
 * transforms apart: a pure rotation leaves ptransform and vtransform agreeing, and a uniform
 * scale leaves vtransform and ntransform agreeing.
 **/
glm::mat4x4 placed() {
    glm::mat4x4 matrix = glm::translate(glm::mat4x4(1.0f), glm::vec3(10.0f, 20.0f, 30.0f));
    return glm::rotate(matrix, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
}

};  // namespace

BOOST_AUTO_TEST_CASE(sltypes_components_test) {
    BOOST_CHECK_EQUAL(v3d::render::offline::sl::components(Type::FLOAT), 1u);
    BOOST_CHECK_EQUAL(v3d::render::offline::sl::components(Type::POINT), 3u);
    BOOST_CHECK_EQUAL(v3d::render::offline::sl::components(Type::COLOR), 3u);
    BOOST_CHECK_EQUAL(v3d::render::offline::sl::components(Type::MATRIX), 16u);
    // a string names a space, a texture or a message: there is no arithmetic to do on it
    BOOST_CHECK_EQUAL(v3d::render::offline::sl::components(Type::STRING), 0u);
}

/**
 * RI's coercions: a float replicates into anything made of floats, and the three point-like
 * types convert to each other. A colour does not convert to or from a position - a colour
 * that has drifted into one is a mistake worth catching.
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
    // and the point-like types mix, taking the left operand's
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
    const glm::vec3 moved = v3d::render::offline::sl::ptransform(placed(), glm::vec3(1.0f, 0.0f, 0.0f));

    // the rotation takes +x to +y, and the translation then carries it
    BOOST_CHECK_CLOSE(moved.x, 10.0f, 0.01f);
    BOOST_CHECK_CLOSE(moved.y, 21.0f, 0.01f);
    BOOST_CHECK_CLOSE(moved.z, 30.0f, 0.01f);
}

/**
 * A direction has no position, so a translation does nothing to it. That is the whole of
 * what tells a vector from a point, and it is invisible until a scene translates.
 **/
BOOST_AUTO_TEST_CASE(sltypes_vtransform_test) {
    const glm::vec3 turned = v3d::render::offline::sl::vtransform(placed(), glm::vec3(1.0f, 0.0f, 0.0f));

    BOOST_CHECK_SMALL(turned.x, 0.0001f);
    BOOST_CHECK_CLOSE(turned.y, 1.0f, 0.01f);
    BOOST_CHECK_SMALL(turned.z, 0.0001f);
}

/**
 * A normal goes by the inverse transpose. Under a rotation that is what a vector does, and
 * the moment a scene scales one axis it is not - the same fault as moya's dicing and talyn's
 * fan, in a third place.
 **/
BOOST_AUTO_TEST_CASE(sltypes_ntransform_test) {
    // under a rotation and a translation the three agree about direction
    const glm::vec3 turned = v3d::render::offline::sl::ntransform(placed(), glm::vec3(1.0f, 0.0f, 0.0f));
    BOOST_CHECK_SMALL(turned.x, 0.0001f);
    BOOST_CHECK_CLOSE(turned.y, 1.0f, 0.01f);
    BOOST_CHECK_SMALL(turned.z, 0.0001f);

    // and a scale of one axis is where they part company: the plane of a surface leans the
    // opposite way to the points that make it
    const glm::mat4x4 stretched = glm::scale(glm::mat4x4(1.0f), glm::vec3(1.0f, 2.0f, 1.0f));
    const glm::vec3 normal(1.0f, 1.0f, 0.0f);

    const glm::vec3 asVector = v3d::render::offline::sl::vtransform(stretched, normal);
    BOOST_CHECK_CLOSE(asVector.y, 2.0f, 0.01f);

    const glm::vec3 asNormal = v3d::render::offline::sl::ntransform(stretched, normal);
    BOOST_CHECK_CLOSE(asNormal.x, 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(asNormal.y, 0.5f, 0.01f);
}
