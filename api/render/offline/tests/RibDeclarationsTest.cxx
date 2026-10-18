/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/offline/rib/Declaration.h>
#include <api/render/offline/rib/Declarations.h>
#include <api/render/offline/rib/Parameters.h>

#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

namespace {

typedef v3d::render::offline::rib::Declaration Declaration;

/**
 * The type gives the float count of an element, per RI table 5.1.
 **/
BOOST_AUTO_TEST_CASE(ribdeclaration_float_counts_test) {
    Declaration declaration;

    BOOST_REQUIRE(Declaration::parse("uniform float", &declaration));
    BOOST_CHECK_EQUAL(declaration.floats(), 1U);

    BOOST_REQUIRE(Declaration::parse("varying point", &declaration));
    BOOST_CHECK_EQUAL(declaration.floats(), 3U);

    BOOST_REQUIRE(Declaration::parse("varying color", &declaration));
    BOOST_CHECK_EQUAL(declaration.floats(), 3U);

    BOOST_REQUIRE(Declaration::parse("vertex hpoint", &declaration));
    BOOST_CHECK_EQUAL(declaration.floats(), 4U);

    BOOST_REQUIRE(Declaration::parse("uniform matrix", &declaration));
    BOOST_CHECK_EQUAL(declaration.floats(), 16U);

    // a string is counted in strings, not in floats
    BOOST_REQUIRE(Declaration::parse("uniform string", &declaration));
    BOOST_CHECK_EQUAL(declaration.floats(), 0U);
}

/**
 * The class gives how many elements a primitive carries, so "P" on a quad is twelve floats
 * and "roughness" on the same quad is one.
 **/
BOOST_AUTO_TEST_CASE(ribdeclaration_element_counts_test) {
    Declaration declaration;

    BOOST_REQUIRE(Declaration::parse("vertex point", &declaration));
    BOOST_CHECK_EQUAL(declaration.elements(4), 4U);

    BOOST_REQUIRE(Declaration::parse("varying color", &declaration));
    BOOST_CHECK_EQUAL(declaration.elements(4), 4U);

    BOOST_REQUIRE(Declaration::parse("uniform float", &declaration));
    BOOST_CHECK_EQUAL(declaration.elements(4), 1U);

    BOOST_REQUIRE(Declaration::parse("constant float", &declaration));
    BOOST_CHECK_EQUAL(declaration.elements(4), 1U);
}

/**
 * An omitted class is uniform, and an array count comes off the type word.
 **/
BOOST_AUTO_TEST_CASE(ribdeclaration_defaults_and_arrays_test) {
    Declaration declaration;

    BOOST_REQUIRE(Declaration::parse("point", &declaration));
    BOOST_CHECK(declaration.storage() == Declaration::Storage::UNIFORM);
    BOOST_CHECK(declaration.type() == Declaration::Type::POINT);
    BOOST_CHECK_EQUAL(declaration.count(), 1U);

    BOOST_REQUIRE(Declaration::parse("float[3]", &declaration));
    BOOST_CHECK_EQUAL(declaration.count(), 3U);
    BOOST_CHECK_EQUAL(declaration.floats(), 1U);

    BOOST_CHECK(!Declaration::parse("uniform", &declaration));
    BOOST_CHECK(!Declaration::parse("", &declaration));
    BOOST_CHECK(!Declaration::parse("uniform quaternion", &declaration));
}

/**
 * The standard variables are declared before a file says anything.
 **/
BOOST_AUTO_TEST_CASE(ribdeclarations_standard_variables_test) {
    const v3d::render::offline::rib::Declarations declarations;
    std::string name;
    Declaration declaration;

    BOOST_REQUIRE(declarations.resolve("P", &name, &declaration));
    BOOST_CHECK_EQUAL(name, "P");
    BOOST_CHECK(declaration.storage() == Declaration::Storage::VERTEX);
    BOOST_CHECK_EQUAL(declaration.floats(), 3U);

    BOOST_REQUIRE(declarations.resolve("Cs", &name, &declaration));
    BOOST_CHECK_EQUAL(declaration.floats(), 3U);

    BOOST_REQUIRE(declarations.resolve("roughness", &name, &declaration));
    BOOST_CHECK_EQUAL(declaration.elements(4), 1U);

    BOOST_CHECK(!declarations.resolve("squish", &name, &declaration));
    BOOST_CHECK_EQUAL(name, "squish");
}

BOOST_AUTO_TEST_CASE(ribdeclarations_declare_then_use_test) {
    v3d::render::offline::rib::Declarations declarations;
    std::string name;
    Declaration declaration;

    BOOST_REQUIRE(declarations.declare({.name = "d", .type = "uniform point"}));
    BOOST_REQUIRE(declarations.resolve("d", &name, &declaration));
    BOOST_CHECK_EQUAL(name, "d");
    BOOST_CHECK_EQUAL(declaration.floats(), 3U);

    // a declaration that does not parse leaves the table alone
    BOOST_CHECK(!declarations.declare({.name = "e", .type = "uniform nonsense"}));
    BOOST_CHECK(!declarations.resolve("e", &name, &declaration));
}

/**
 * The inline form types a parameter where it is used and does not enter the table.
 **/
BOOST_AUTO_TEST_CASE(ribdeclarations_inline_form_test) {
    const v3d::render::offline::rib::Declarations declarations;
    std::string name;
    Declaration declaration;

    BOOST_REQUIRE(declarations.resolve("uniform float squish", &name, &declaration));
    BOOST_CHECK_EQUAL(name, "squish");
    BOOST_CHECK_EQUAL(declaration.floats(), 1U);
    BOOST_CHECK(declaration.storage() == Declaration::Storage::UNIFORM);

    BOOST_REQUIRE(declarations.resolve("varying point Q", &name, &declaration));
    BOOST_CHECK_EQUAL(name, "Q");
    BOOST_CHECK(declaration.storage() == Declaration::Storage::VARYING);
    BOOST_CHECK_EQUAL(declaration.floats(), 3U);

    // it stayed out of the table
    BOOST_CHECK(!declarations.resolve("squish", &name, &declaration));
}

BOOST_AUTO_TEST_CASE(parameterlist_reads_back_typed_test) {
    v3d::render::offline::rib::ParameterList parameters;
    Declaration point;
    BOOST_REQUIRE(Declaration::parse("vertex point", &point));
    Declaration text;
    BOOST_REQUIRE(Declaration::parse("uniform string", &text));

    std::vector<float> values;
    values.reserve(6);
    for (int i = 0; i < 6; i++) {
        values.push_back(static_cast<float>(i));
    }
    parameters.add("P", point, values, std::vector<std::string>());
    parameters.add("texturename", text, std::vector<float>(), std::vector<std::string>(1, "wood.tif"));

    BOOST_CHECK_EQUAL(parameters.size(), 2U);
    BOOST_CHECK(parameters.has("P"));
    BOOST_CHECK(!parameters.has("N"));

    const std::vector<glm::vec3> points = parameters.points("P");
    BOOST_REQUIRE_EQUAL(points.size(), 2U);
    BOOST_CHECK_EQUAL(points[1].x, 3.0F);
    BOOST_CHECK_EQUAL(points[1].z, 5.0F);

    BOOST_CHECK_EQUAL(parameters.string("texturename"), "wood.tif");

    // an absent name is the fallback rather than a throw
    BOOST_CHECK_EQUAL(parameters.number("roughness", 0.5F), 0.5F);
    BOOST_CHECK(parameters.floats("N").empty());
    BOOST_CHECK(parameters.declaration("N") == nullptr);
}

/**
 * A partial trailing triple contributes nothing. A file whose array does not match its
 * declaration produces one.
 **/
BOOST_AUTO_TEST_CASE(parameterlist_partial_point_test) {
    v3d::render::offline::rib::ParameterList parameters;
    Declaration point;
    BOOST_REQUIRE(Declaration::parse("uniform point", &point));

    std::vector<float> values;
    values.push_back(1.0F);
    parameters.add("d", point, values, std::vector<std::string>());

    BOOST_CHECK(parameters.points("d").empty());
    BOOST_CHECK_EQUAL(parameters.floats("d").size(), 1U);
}

/**
 * A matrix read in RIB's order needs no transpose to become glm's, as ParameterList::matrix
 * explains. The test checks the convention by applying the result to a point.
 **/
BOOST_AUTO_TEST_CASE(parameterlist_matrix_convention_test) {
    v3d::render::offline::rib::ParameterList parameters;
    Declaration matrix;
    BOOST_REQUIRE(Declaration::parse("uniform matrix", &matrix));

    // RI writes the translation in the last row
    const float values[16] = {
        1.0F, 0.0F, 0.0F, 0.0F,
        0.0F, 1.0F, 0.0F, 0.0F,
        0.0F, 0.0F, 1.0F, 0.0F,
        2.0F, 3.0F, 4.0F, 1.0F
    };
    parameters.add("m", matrix, std::vector<float>(values, values + 16), std::vector<std::string>());

    const glm::vec4 moved = parameters.matrix("m") * glm::vec4(0.0F, 0.0F, 0.0F, 1.0F);
    BOOST_CHECK_EQUAL(moved.x, 2.0F);
    BOOST_CHECK_EQUAL(moved.y, 3.0F);
    BOOST_CHECK_EQUAL(moved.z, 4.0F);
}

};  // namespace
