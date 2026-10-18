/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/offline/rib/Arguments.h>
#include <api/render/offline/rib/Declaration.h>
#include <api/render/offline/rib/Declarations.h>
#include <api/render/offline/rib/Parameters.h>

#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

namespace {

typedef v3d::render::offline::rib::Declaration Declaration;
typedef v3d::render::offline::rib::Declarations Declarations;
typedef v3d::render::offline::rib::ParameterList ParameterList;

/**
 * The C interface passes a parameter list as two parallel arrays, and neither says how long
 * a value array is; only the declaration does. The reader resolves a file's parameters the
 * same way, so both paths into a render context agree.
 **/
BOOST_AUTO_TEST_CASE(ribarguments_typed_by_declaration_test) {
    const Declarations declarations;
    float roughness = 0.3F;
    float colour[3] = { 0.25F, 0.5F, 0.75F };

    const char* const tokens[] = { "roughness", "specularcolor" };
    const void* const values[] = { &roughness, colour };

    ParameterList list;
    arguments(declarations, 2, tokens, values, 1, &list);
    BOOST_CHECK_EQUAL(list.size(), 2U);
    BOOST_CHECK_CLOSE(list.number("roughness", 0.0F), 0.3F, 0.01F);
    BOOST_REQUIRE_EQUAL(list.floats("specularcolor").size(), 3U);
    BOOST_CHECK_CLOSE(list.floats("specularcolor")[2], 0.75F, 0.01F);
}

/**
 * A name may carry its declaration inline, which declares it for that request only, as it
 * does in a file.
 **/
BOOST_AUTO_TEST_CASE(ribarguments_inline_declaration_test) {
    const Declarations declarations;
    float squish = 5.0F;
    const char* const tokens[] = { "uniform float squish" };
    const void* const values[] = { &squish };

    ParameterList list;
    arguments(declarations, 1, tokens, values, 1, &list);
    BOOST_CHECK(list.has("squish"));
    BOOST_CHECK_CLOSE(list.number("squish", 0.0F), 5.0F, 0.01F);
}

/**
 * An integer parameter's array holds ints, as RtInt is. Each is read as an int and kept as the
 * same number, not as its bit pattern.
 **/
BOOST_AUTO_TEST_CASE(ribarguments_integers_test) {
    const Declarations declarations;
    const int samples[2] = { 3, -2 };
    const char* const tokens[] = { "uniform integer[2] samples" };
    const void* const values[] = { samples };

    ParameterList list;
    arguments(declarations, 1, tokens, values, 1, &list);
    BOOST_REQUIRE_EQUAL(list.floats("samples").size(), 2U);
    BOOST_CHECK_EQUAL(list.floats("samples")[0], 3.0F);
    BOOST_CHECK_EQUAL(list.floats("samples")[1], -2.0F);
}

/**
 * A string parameter's array is an array of pointers to characters, as RtString is.
 **/
BOOST_AUTO_TEST_CASE(ribarguments_strings_test) {
    const Declarations declarations;
    const char* name = "myball";
    const char* const text[] = { name };
    const void* const values[] = { static_cast<const void*>(text) };
    const char* const tokens[] = { "name" };

    ParameterList list;
    arguments(declarations, 1, tokens, values, 1, &list);
    BOOST_CHECK_EQUAL(list.string("name"), "myball");
}

/**
 * A varying parameter takes one element per vertex, so the same two arrays read as four
 * values on a quad and as one on a shader request.
 **/
BOOST_AUTO_TEST_CASE(ribarguments_varying_takes_the_vertices_test) {
    const Declarations declarations;
    float points[12] = { 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F,
                         1.0F, 1.0F, 0.0F, 0.0F, 1.0F, 0.0F };
    const char* const tokens[] = { "P" };
    const void* const values[] = { points };

    ParameterList list;
    arguments(declarations, 1, tokens, values, 4, &list);
    BOOST_CHECK_EQUAL(list.points("P").size(), 4U);
}

/**
 * A name with no declaration has no length either, so it is dropped rather than read
 * past. The caller receives the list of those names.
 **/
BOOST_AUTO_TEST_CASE(ribarguments_undeclared_is_dropped_test) {
    const Declarations declarations;
    float veins = 1.0F;
    const char* const tokens[] = { "veins", "roughness" };
    const void* const values[] = { &veins, &veins };

    std::vector<std::string> unresolved;
    ParameterList list;
    arguments(declarations, 2, tokens, values, 1, &list, &unresolved);
    BOOST_CHECK(!list.has("veins"));
    BOOST_CHECK(list.has("roughness"));
    BOOST_REQUIRE_EQUAL(unresolved.size(), 1U);
    BOOST_CHECK_EQUAL(unresolved[0], "veins");
}

/**
 * A null anywhere means the caller built its arrays wrong. It is skipped, because reading
 * through it would crash the render.
 **/
BOOST_AUTO_TEST_CASE(ribarguments_nulls_test) {
    const Declarations declarations;
    const char* const tokens[] = { nullptr, "roughness" };
    const void* const values[] = { nullptr, nullptr };

    ParameterList list;
    arguments(declarations, 2, tokens, values, 1, &list);
    BOOST_CHECK_EQUAL(list.size(), 0U);
    ParameterList nothing;
    arguments(declarations, 2, nullptr, nullptr, 1, &nothing);
    BOOST_CHECK_EQUAL(nothing.size(), 0U);
}

};  // namespace
