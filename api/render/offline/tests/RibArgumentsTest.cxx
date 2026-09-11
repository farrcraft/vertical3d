/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/offline/rib/Arguments.h>

#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

namespace {

typedef v3d::render::offline::rib::Declaration Declaration;
typedef v3d::render::offline::rib::Declarations Declarations;
typedef v3d::render::offline::rib::ParameterList ParameterList;

};  // namespace

/**
 * The C interface hands a parameter list over as two parallel arrays, and neither of them
 * says how long a value array is - the declaration does. This is the same answer the
 * reader reaches from a file, which is why the two paths into a render context cannot
 * disagree about what a scene said.
 **/
BOOST_AUTO_TEST_CASE(ribarguments_typed_by_declaration_test) {
    Declarations declarations;
    float roughness = 0.3f;
    float colour[3] = { 0.25f, 0.5f, 0.75f };

    const char* tokens[] = { "roughness", "specularcolor" };
    const void* values[] = { &roughness, colour };

    ParameterList list;
    arguments(declarations, 2, tokens, values, 1, &list);
    BOOST_CHECK_EQUAL(list.size(), 2u);
    BOOST_CHECK_CLOSE(list.number("roughness", 0.0f), 0.3f, 0.01f);
    BOOST_REQUIRE_EQUAL(list.floats("specularcolor").size(), 3u);
    BOOST_CHECK_CLOSE(list.floats("specularcolor")[2], 0.75f, 0.01f);
}

/**
 * A name may carry its declaration inline, which declares it for the one request - the
 * same thing it means in a file.
 **/
BOOST_AUTO_TEST_CASE(ribarguments_inline_declaration_test) {
    Declarations declarations;
    float squish = 5.0f;
    const char* tokens[] = { "uniform float squish" };
    const void* values[] = { &squish };

    ParameterList list;
    arguments(declarations, 1, tokens, values, 1, &list);
    BOOST_CHECK(list.has("squish"));
    BOOST_CHECK_CLOSE(list.number("squish", 0.0f), 5.0f, 0.01f);
}

/**
 * A string parameter's array is an array of pointers to characters, which is what
 * RtString is - the one type whose values are not floats.
 **/
BOOST_AUTO_TEST_CASE(ribarguments_strings_test) {
    Declarations declarations;
    const char* name = "myball";
    const char* text[] = { name };
    const void* values[] = { static_cast<const void*>(text) };
    const char* tokens[] = { "name" };

    ParameterList list;
    arguments(declarations, 1, tokens, values, 1, &list);
    BOOST_CHECK_EQUAL(list.string("name", ""), "myball");
}

/**
 * A varying parameter takes one element per vertex, so the same two arrays read as four
 * values on a quad and as one on a shader request.
 **/
BOOST_AUTO_TEST_CASE(ribarguments_varying_takes_the_vertices_test) {
    Declarations declarations;
    float points[12] = { 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                         1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f };
    const char* tokens[] = { "P" };
    const void* values[] = { points };

    ParameterList list;
    arguments(declarations, 1, tokens, values, 4, &list);
    BOOST_CHECK_EQUAL(list.points("P").size(), 4u);
}

/**
 * A name with no declaration has no length either, so it is dropped rather than read
 * past. A caller that wants to say so is told which names those were.
 **/
BOOST_AUTO_TEST_CASE(ribarguments_undeclared_is_dropped_test) {
    Declarations declarations;
    float veins = 1.0f;
    const char* tokens[] = { "veins", "roughness" };
    const void* values[] = { &veins, &veins };

    std::vector<std::string> unresolved;
    ParameterList list;
    arguments(declarations, 2, tokens, values, 1, &list, &unresolved);
    BOOST_CHECK(!list.has("veins"));
    BOOST_CHECK(list.has("roughness"));
    BOOST_REQUIRE_EQUAL(unresolved.size(), 1u);
    BOOST_CHECK_EQUAL(unresolved[0], "veins");
}

/**
 * A null anywhere is a caller that built its arrays wrong, and reading past one would be
 * the last thing a render did.
 **/
BOOST_AUTO_TEST_CASE(ribarguments_nulls_test) {
    Declarations declarations;
    const char* tokens[] = { nullptr, "roughness" };
    const void* values[] = { nullptr, nullptr };

    ParameterList list;
    arguments(declarations, 2, tokens, values, 1, &list);
    BOOST_CHECK_EQUAL(list.size(), 0u);
    ParameterList nothing;
    arguments(declarations, 2, nullptr, nullptr, 1, &nothing);
    BOOST_CHECK_EQUAL(nothing.size(), 0u);
}
