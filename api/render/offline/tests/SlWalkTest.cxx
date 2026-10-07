/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/offline/sl/Parser.h>
#include <api/render/offline/sl/syntax/Block.h>
#include <api/render/offline/sl/syntax/Shader.h>
#include <api/render/offline/sl/syntax/Walk.h>

#include <sstream>
#include <vector>

#include <boost/test/unit_test.hpp>

namespace {

typedef v3d::render::offline::sl::syntax::Expression Expression;
typedef v3d::render::offline::sl::syntax::Statement Statement;

/**
 * The last statement of a surface shader's body, parsed.
 **/
v3d::render::offline::sl::syntax::StatementPtr last(const char* source) {
    std::istringstream stream(source);
    v3d::render::offline::sl::Parser parser(stream);
    const std::vector<v3d::render::offline::sl::syntax::ShaderPtr> shaders = parser.parse();
    BOOST_REQUIRE_EQUAL(shaders.size(), 1u);
    BOOST_REQUIRE(!shaders[0]->body->statements.empty());
    return shaders[0]->body->statements.back();
}

};  // namespace

/**
 * A `for` hands over its initialiser, condition, step and body in the order they are written,
 * each to the visitor for its kind.
 **/
BOOST_AUTO_TEST_CASE(slwalk_for_test) {
    const v3d::render::offline::sl::syntax::StatementPtr loop =
        last("surface s() { float i; for (i = 0; i < 4; i += 1) { Ci = 1; } }");
    std::vector<int> order;
    v3d::render::offline::sl::syntax::forEachChild(*loop,
        [&order](const v3d::render::offline::sl::syntax::ExpressionPtr & child) {
            order.push_back(child->kind == Expression::Kind::BINARY ? 1 : -1);
        },
        [&order](const v3d::render::offline::sl::syntax::StatementPtr & child) {
            order.push_back(child->kind == Statement::Kind::BLOCK ? 2 : 0);
        });
    // the initialiser and the step are assignments, the condition a comparison
    BOOST_CHECK((order == std::vector<int>{ 0, 1, 0, 2 }));
}

/**
 * An `if` with no `else` hands over its condition and the one arm it has, not a null.
 **/
BOOST_AUTO_TEST_CASE(slwalk_missing_test) {
    const v3d::render::offline::sl::syntax::StatementPtr conditional = last("surface s() { if (1 < 2) Ci = 1; }");
    int expressions = 0;
    int statements = 0;
    v3d::render::offline::sl::syntax::forEachChild(*conditional,
        [&expressions](const v3d::render::offline::sl::syntax::ExpressionPtr & child) {
            BOOST_CHECK(child);
            expressions++;
        },
        [&statements](const v3d::render::offline::sl::syntax::StatementPtr & child) {
            BOOST_CHECK(child);
            statements++;
        });
    BOOST_CHECK_EQUAL(expressions, 1);
    BOOST_CHECK_EQUAL(statements, 1);
}
