/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "../RIBDeclarations.h"
#include "../RIBParameters.h"

namespace {

typedef v3d::render::offline::RIBDeclaration Declaration;

};  // namespace

/**
 * The type gives the float count of an element, per RI table 5.1.
 **/
BOOST_AUTO_TEST_CASE(ribdeclaration_float_counts_test) {
    Declaration declaration;

    BOOST_REQUIRE(Declaration::parse("uniform float", &declaration));
    BOOST_CHECK_EQUAL(declaration.floats(), 1u);

    BOOST_REQUIRE(Declaration::parse("varying point", &declaration));
    BOOST_CHECK_EQUAL(declaration.floats(), 3u);

    BOOST_REQUIRE(Declaration::parse("varying color", &declaration));
    BOOST_CHECK_EQUAL(declaration.floats(), 3u);

    BOOST_REQUIRE(Declaration::parse("vertex hpoint", &declaration));
    BOOST_CHECK_EQUAL(declaration.floats(), 4u);

    BOOST_REQUIRE(Declaration::parse("uniform matrix", &declaration));
    BOOST_CHECK_EQUAL(declaration.floats(), 16u);

    // a string is counted in strings, not in floats
    BOOST_REQUIRE(Declaration::parse("uniform string", &declaration));
    BOOST_CHECK_EQUAL(declaration.floats(), 0u);
}

/**
 * The class gives how many elements a primitive carries, which is what makes "P" on a quad
 * twelve floats and "roughness" on the same quad one.
 **/
BOOST_AUTO_TEST_CASE(ribdeclaration_element_counts_test) {
    Declaration declaration;

    BOOST_REQUIRE(Declaration::parse("vertex point", &declaration));
    BOOST_CHECK_EQUAL(declaration.elements(4), 4u);

    BOOST_REQUIRE(Declaration::parse("varying color", &declaration));
    BOOST_CHECK_EQUAL(declaration.elements(4), 4u);

    BOOST_REQUIRE(Declaration::parse("uniform float", &declaration));
    BOOST_CHECK_EQUAL(declaration.elements(4), 1u);

    BOOST_REQUIRE(Declaration::parse("constant float", &declaration));
    BOOST_CHECK_EQUAL(declaration.elements(4), 1u);
}

/**
 * An omitted class is uniform, and an array count comes off the type word.
 **/
BOOST_AUTO_TEST_CASE(ribdeclaration_defaults_and_arrays_test) {
    Declaration declaration;

    BOOST_REQUIRE(Declaration::parse("point", &declaration));
    BOOST_CHECK(declaration.storage() == Declaration::Storage::UNIFORM);
    BOOST_CHECK(declaration.type() == Declaration::Type::POINT);
    BOOST_CHECK_EQUAL(declaration.count(), 1u);

    BOOST_REQUIRE(Declaration::parse("float[3]", &declaration));
    BOOST_CHECK_EQUAL(declaration.count(), 3u);
    BOOST_CHECK_EQUAL(declaration.floats(), 1u);

    BOOST_CHECK(!Declaration::parse("uniform", &declaration));
    BOOST_CHECK(!Declaration::parse("", &declaration));
    BOOST_CHECK(!Declaration::parse("uniform quaternion", &declaration));
}

/**
 * The standard variables are declared before a file says anything.
 **/
BOOST_AUTO_TEST_CASE(ribdeclarations_standard_variables_test) {
    v3d::render::offline::RIBDeclarations declarations;
    std::string name;
    Declaration declaration;

    BOOST_REQUIRE(declarations.resolve("P", &name, &declaration));
    BOOST_CHECK_EQUAL(name, "P");
    BOOST_CHECK(declaration.storage() == Declaration::Storage::VERTEX);
    BOOST_CHECK_EQUAL(declaration.floats(), 3u);

    BOOST_REQUIRE(declarations.resolve("Cs", &name, &declaration));
    BOOST_CHECK_EQUAL(declaration.floats(), 3u);

    BOOST_REQUIRE(declarations.resolve("roughness", &name, &declaration));
    BOOST_CHECK_EQUAL(declaration.elements(4), 1u);

    BOOST_CHECK(!declarations.resolve("squish", &name, &declaration));
    BOOST_CHECK_EQUAL(name, "squish");
}

BOOST_AUTO_TEST_CASE(ribdeclarations_declare_then_use_test) {
    v3d::render::offline::RIBDeclarations declarations;
    std::string name;
    Declaration declaration;

    BOOST_REQUIRE(declarations.declare("d", "uniform point"));
    BOOST_REQUIRE(declarations.resolve("d", &name, &declaration));
    BOOST_CHECK_EQUAL(name, "d");
    BOOST_CHECK_EQUAL(declaration.floats(), 3u);

    // a declaration that does not parse leaves the table alone
    BOOST_CHECK(!declarations.declare("e", "uniform nonsense"));
    BOOST_CHECK(!declarations.resolve("e", &name, &declaration));
}

/**
 * The inline form types a parameter where it is used and does not enter the table.
 **/
BOOST_AUTO_TEST_CASE(ribdeclarations_inline_form_test) {
    v3d::render::offline::RIBDeclarations declarations;
    std::string name;
    Declaration declaration;

    BOOST_REQUIRE(declarations.resolve("uniform float squish", &name, &declaration));
    BOOST_CHECK_EQUAL(name, "squish");
    BOOST_CHECK_EQUAL(declaration.floats(), 1u);
    BOOST_CHECK(declaration.storage() == Declaration::Storage::UNIFORM);

    BOOST_REQUIRE(declarations.resolve("varying point Q", &name, &declaration));
    BOOST_CHECK_EQUAL(name, "Q");
    BOOST_CHECK(declaration.storage() == Declaration::Storage::VARYING);
    BOOST_CHECK_EQUAL(declaration.floats(), 3u);

    // and it stayed out of the table
    BOOST_CHECK(!declarations.resolve("squish", &name, &declaration));
}

BOOST_AUTO_TEST_CASE(parameterlist_reads_back_typed_test) {
    v3d::render::offline::ParameterList parameters;
    Declaration point;
    BOOST_REQUIRE(Declaration::parse("vertex point", &point));
    Declaration text;
    BOOST_REQUIRE(Declaration::parse("uniform string", &text));

    std::vector<float> values;
    for (int i = 0; i < 6; i++) {
        values.push_back(static_cast<float>(i));
    }
    parameters.add("P", point, values, std::vector<std::string>());
    parameters.add("texturename", text, std::vector<float>(), std::vector<std::string>(1, "wood.tif"));

    BOOST_CHECK_EQUAL(parameters.size(), 2u);
    BOOST_CHECK(parameters.has("P"));
    BOOST_CHECK(!parameters.has("N"));

    const std::vector<glm::vec3> points = parameters.points("P");
    BOOST_REQUIRE_EQUAL(points.size(), 2u);
    BOOST_CHECK_EQUAL(points[1].x, 3.0f);
    BOOST_CHECK_EQUAL(points[1].z, 5.0f);

    BOOST_CHECK_EQUAL(parameters.string("texturename", ""), "wood.tif");

    // an absent name is the fallback rather than a throw
    BOOST_CHECK_EQUAL(parameters.number("roughness", 0.5f), 0.5f);
    BOOST_CHECK(parameters.floats("N").empty());
    BOOST_CHECK(parameters.declaration("N") == nullptr);
}

/**
 * A partial trailing triple contributes nothing, which is what a file whose array does not
 * match its declaration produces.
 **/
BOOST_AUTO_TEST_CASE(parameterlist_partial_point_test) {
    v3d::render::offline::ParameterList parameters;
    Declaration point;
    BOOST_REQUIRE(Declaration::parse("uniform point", &point));

    std::vector<float> values;
    values.push_back(1.0f);
    parameters.add("d", point, values, std::vector<std::string>());

    BOOST_CHECK(parameters.points("d").empty());
    BOOST_CHECK_EQUAL(parameters.floats("d").size(), 1u);
}

/**
 * RIB writes a matrix in row major order under RI's row vector convention and glm stores
 * column major under a column vector one, so reading the floats in order is the change of
 * convention. A transpose here would undo it, which is why this applies the result to a point.
 **/
BOOST_AUTO_TEST_CASE(parameterlist_matrix_convention_test) {
    v3d::render::offline::ParameterList parameters;
    Declaration matrix;
    BOOST_REQUIRE(Declaration::parse("uniform matrix", &matrix));

    // RI writes the translation in the last row
    const float values[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        2.0f, 3.0f, 4.0f, 1.0f
    };
    parameters.add("m", matrix, std::vector<float>(values, values + 16), std::vector<std::string>());

    const glm::vec4 moved = parameters.matrix("m") * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    BOOST_CHECK_EQUAL(moved.x, 2.0f);
    BOOST_CHECK_EQUAL(moved.y, 3.0f);
    BOOST_CHECK_EQUAL(moved.z, 4.0f);
}
