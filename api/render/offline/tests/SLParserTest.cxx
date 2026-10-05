/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/offline/sl/Parser.h>
#include <api/render/offline/sl/syntax/Assignment.h>
#include <api/render/offline/sl/syntax/Binary.h>
#include <api/render/offline/sl/syntax/Cast.h>
#include <api/render/offline/sl/syntax/Conditional.h>
#include <api/render/offline/sl/syntax/Declaration.h>
#include <api/render/offline/sl/syntax/Expression.h>
#include <api/render/offline/sl/syntax/For.h>
#include <api/render/offline/sl/syntax/Lighting.h>
#include <api/render/offline/sl/syntax/Parameter.h>
#include <api/render/offline/sl/syntax/Shader.h>
#include <api/render/offline/sl/syntax/Statement.h>
#include <api/render/offline/sl/syntax/Ternary.h>
#include <api/render/offline/sl/syntax/Tuple.h>

#include <sstream>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

namespace {

typedef v3d::render::offline::sl::syntax::Expression Expression;
typedef v3d::render::offline::sl::syntax::Statement Statement;

std::vector<v3d::render::offline::sl::syntax::ShaderPtr> parse(const std::string & source,
    std::string * error = nullptr) {
    std::istringstream stream(source);
    v3d::render::offline::sl::Parser parser(stream);
    std::vector<v3d::render::offline::sl::syntax::ShaderPtr> shaders = parser.parse();
    if (error) {
        *error = parser.error();
    }
    return shaders;
}

/**
 * The only statement of a shader whose body holds exactly one, as an expression. The
 * precedence cases check its shape.
 **/
v3d::render::offline::sl::syntax::ExpressionPtr only(const std::string & expression) {
    std::string error;
    std::vector<v3d::render::offline::sl::syntax::ShaderPtr> shaders =
        parse("surface s() { Ci = " + expression + "; }", &error);
    BOOST_REQUIRE_EQUAL(error, "");
    BOOST_REQUIRE_EQUAL(shaders.size(), 1u);
    BOOST_REQUIRE_EQUAL(shaders[0]->body->statements.size(), 1u);
    const Statement & statement = *shaders[0]->body->statements[0];
    BOOST_REQUIRE(statement.kind == Statement::Kind::ASSIGNMENT);
    return static_cast<const v3d::render::offline::sl::syntax::Assignment &>(statement).value;
}

/**
 * The operator of a binary node, or empty when the node is not one, so a failure shows an
 * empty operator rather than the result of a wrong cast.
 **/
std::string binary(const v3d::render::offline::sl::syntax::ExpressionPtr & expression) {
    if (!expression || expression->kind != Expression::Kind::BINARY) {
        return "";
    }
    return static_cast<const v3d::render::offline::sl::syntax::Binary &>(*expression).op;
}

v3d::render::offline::sl::syntax::ExpressionPtr left(const v3d::render::offline::sl::syntax::ExpressionPtr & expression) {
    return static_cast<const v3d::render::offline::sl::syntax::Binary &>(*expression).left;
}

v3d::render::offline::sl::syntax::ExpressionPtr right(const v3d::render::offline::sl::syntax::ExpressionPtr & expression) {
    return static_cast<const v3d::render::offline::sl::syntax::Binary &>(*expression).right;
}

/**
 * The four standard surface shaders, as the RI specification writes them. The library
 * compiles them in.
 **/
const char* const CONSTANT =
"surface constant() {\n"
"    Ci = Cs;\n"
"    Oi = Os;\n"
"}\n";

const char* const MATTE =
"surface matte(float Ka = 1; float Kd = 1;) {\n"
"    normal Nf = faceforward(normalize(N), I);\n"
"    Oi = Os;\n"
"    Ci = Os * Cs * (Ka * ambient() + Kd * diffuse(Nf));\n"
"}\n";

const char* const METAL =
"surface metal(float Ka = 1; float Ks = 1; float roughness = 0.1;) {\n"
"    normal Nf = faceforward(normalize(N), I);\n"
"    vector V = -normalize(I);\n"
"    Oi = Os;\n"
"    Ci = Os * Cs * (Ka * ambient() + Ks * specular(Nf, V, roughness));\n"
"}\n";

const char* const PLASTIC =
"surface plastic(float Ka = 1; float Kd = 0.5; float Ks = 0.5;\n"
"                float roughness = 0.1; color specularcolor = 1;) {\n"
"    normal Nf = faceforward(normalize(N), I);\n"
"    vector V = -normalize(I);\n"
"    Oi = Os;\n"
"    Ci = Os * (Cs * (Ka * ambient() + Kd * diffuse(Nf)) +\n"
"               specularcolor * Ks * specular(Nf, V, roughness));\n"
"}\n";

const char* const AMBIENTLIGHT =
"light ambientlight(float intensity = 1; color lightcolor = 1;) {\n"
"    Cl = intensity * lightcolor;\n"
"}\n";

const char* const DISTANTLIGHT =
"light distantlight(float intensity = 1; color lightcolor = 1;\n"
"                   point from = point \"shader\" (0, 0, 0);\n"
"                   point to = point \"shader\" (0, 0, 1);) {\n"
"    solar(to - from, 0.0) {\n"
"        Cl = intensity * lightcolor;\n"
"    }\n"
"}\n";

const char* const POINTLIGHT =
"light pointlight(float intensity = 1; color lightcolor = 1;\n"
"                 point from = point \"shader\" (0, 0, 0);) {\n"
"    illuminate(from) {\n"
"        Cl = intensity * lightcolor / (L . L);\n"
"    }\n"
"}\n";

const char* const SPOTLIGHT =
"light spotlight(float intensity = 1; color lightcolor = 1;\n"
"                point from = point \"shader\" (0, 0, 0);\n"
"                point to = point \"shader\" (0, 0, 1);\n"
"                float coneangle = 0.1; float conedeltaangle = 0.02;\n"
"                float beamdistribution = 2;) {\n"
"    float atten;\n"
"    float cosangle;\n"
"    uniform vector A = (to - from) / length(to - from);\n"
"    illuminate(from, A, coneangle) {\n"
"        cosangle = (L . A) / length(L);\n"
"        atten = pow(cosangle, beamdistribution) / (L . L);\n"
"        atten = atten * smoothstep(cos(coneangle), cos(coneangle - conedeltaangle), cosangle);\n"
"        Cl = atten * intensity * lightcolor;\n"
"    }\n"
"}\n";

};  // namespace

/**
 * The four standard surface shaders parse, so a renderer can resolve `Surface "matte"` with
 * no shader files at all.
 **/
BOOST_AUTO_TEST_CASE(slparser_standard_surface_shaders_test) {
    const char* const sources[] = { CONSTANT, MATTE, METAL, PLASTIC };
    const char* const names[] = { "constant", "matte", "metal", "plastic" };
    for (std::size_t i = 0; i < 4; i++) {
        std::string error;
        std::vector<v3d::render::offline::sl::syntax::ShaderPtr> shaders = parse(sources[i], &error);
        BOOST_CHECK_EQUAL(error, "");
        BOOST_REQUIRE_EQUAL(shaders.size(), 1u);
        BOOST_CHECK_EQUAL(shaders[0]->name, names[i]);
        BOOST_CHECK(shaders[0]->type == v3d::render::offline::sl::ShaderType::SURFACE);
        BOOST_CHECK(shaders[0]->supported());
    }
}

/**
 * The four standard lights parse; they contain the three lighting constructs.
 **/
BOOST_AUTO_TEST_CASE(slparser_standard_light_shaders_test) {
    const char* const sources[] = { AMBIENTLIGHT, DISTANTLIGHT, POINTLIGHT, SPOTLIGHT };
    const char* const names[] = { "ambientlight", "distantlight", "pointlight", "spotlight" };
    for (std::size_t i = 0; i < 4; i++) {
        std::string error;
        std::vector<v3d::render::offline::sl::syntax::ShaderPtr> shaders = parse(sources[i], &error);
        BOOST_CHECK_EQUAL(error, "");
        BOOST_REQUIRE_EQUAL(shaders.size(), 1u);
        BOOST_CHECK_EQUAL(shaders[0]->name, names[i]);
        BOOST_CHECK(shaders[0]->type == v3d::render::offline::sl::ShaderType::LIGHT);
    }
}

/**
 * A parameter carries a type, an optional storage class, an optional output, and a default
 * that is required: SL has no uninitialised parameter, and a scene that does not mention
 * the parameter gets the default.
 **/
BOOST_AUTO_TEST_CASE(slparser_parameters_test) {
    std::string error;
    std::vector<v3d::render::offline::sl::syntax::ShaderPtr> shaders = parse(
        "surface s(float Ka = 1; output varying color Ci2 = 0; uniform string space = \"world\";) { }",
        &error);

    BOOST_CHECK_EQUAL(error, "");
    BOOST_REQUIRE_EQUAL(shaders.size(), 1u);
    const std::vector<v3d::render::offline::sl::syntax::Parameter> & parameters = shaders[0]->parameters;
    BOOST_REQUIRE_EQUAL(parameters.size(), 3u);

    BOOST_CHECK_EQUAL(parameters[0].name, "Ka");
    BOOST_CHECK(parameters[0].type == v3d::render::offline::sl::Type::FLOAT);
    BOOST_CHECK(parameters[0].storage == v3d::render::offline::sl::Storage::UNSPECIFIED);
    BOOST_CHECK(!parameters[0].output);
    BOOST_REQUIRE(parameters[0].defaultValue);

    BOOST_CHECK(parameters[1].output);
    BOOST_CHECK(parameters[1].storage == v3d::render::offline::sl::Storage::VARYING);
    BOOST_CHECK(parameters[1].type == v3d::render::offline::sl::Type::COLOR);

    BOOST_CHECK(parameters[2].storage == v3d::render::offline::sl::Storage::UNIFORM);
    BOOST_CHECK(parameters[2].type == v3d::render::offline::sl::Type::STRING);
}

/**
 * A missing default is an error rather than a zero, because a shader that omits one and a
 * shader whose default is zero would otherwise be the same shader.
 **/
BOOST_AUTO_TEST_CASE(slparser_parameter_needs_a_default_test) {
    std::string error;
    std::vector<v3d::render::offline::sl::syntax::ShaderPtr> shaders = parse("surface s(float Ka;) { }", &error);

    BOOST_CHECK_EQUAL(shaders.size(), 0u);
    BOOST_CHECK_EQUAL(error, "expected '=' but found ';' at line 1, column 19");
}

/**
 * All five shader types parse. Displacement and volume shaders do not run, and come back
 * marked rather than refused, so a scene carrying one is told what is unsupported rather
 * than what is malformed.
 **/
BOOST_AUTO_TEST_CASE(slparser_unsupported_shader_types_test) {
    std::string error;
    std::vector<v3d::render::offline::sl::syntax::ShaderPtr> shaders = parse(
        "displacement bumpy(float Km = 1;) { P = P + Km * N; }\n"
        "volume fog(float density = 1;) { Ci = Ci * density; }\n"
        "imager background(color bgcolor = 0;) { Ci = Ci + (1 - alpha) * bgcolor; }\n", &error);

    BOOST_CHECK_EQUAL(error, "");
    BOOST_REQUIRE_EQUAL(shaders.size(), 3u);
    BOOST_CHECK(shaders[0]->type == v3d::render::offline::sl::ShaderType::DISPLACEMENT);
    BOOST_CHECK(!shaders[0]->supported());
    BOOST_CHECK(shaders[1]->type == v3d::render::offline::sl::ShaderType::VOLUME);
    BOOST_CHECK(!shaders[1]->supported());
    BOOST_CHECK(shaders[2]->type == v3d::render::offline::sl::ShaderType::IMAGER);
    BOOST_CHECK(shaders[2]->supported());
}

/**
 * '.' is a dot product and '^' a cross product, and both bind tighter than a multiply.
 * Both differ from C, so each has a case.
 **/
BOOST_AUTO_TEST_CASE(slparser_dot_and_cross_test) {
    // a . b * c is (a . b) * c, since the dot binds tighter
    v3d::render::offline::sl::syntax::ExpressionPtr product = only("a . b * c");
    BOOST_CHECK_EQUAL(binary(product), "*");
    BOOST_CHECK_EQUAL(binary(left(product)), ".");

    // a * b ^ c is a * (b ^ c) for the same reason
    v3d::render::offline::sl::syntax::ExpressionPtr cross = only("a * b ^ c");
    BOOST_CHECK_EQUAL(binary(cross), "*");
    BOOST_CHECK_EQUAL(binary(right(cross)), "^");

    // a '.' is not a member access: its right hand side is an expression of its own
    v3d::render::offline::sl::syntax::ExpressionPtr dot = only("L . L");
    BOOST_CHECK_EQUAL(binary(dot), ".");
    BOOST_CHECK(right(dot)->kind == Expression::Kind::VARIABLE);

    // a '^' is not an exponent: pow() raises a number to a power
    v3d::render::offline::sl::syntax::ExpressionPtr power = only("pow(a, b)");
    BOOST_CHECK(power->kind == Expression::Kind::CALL);
}

/**
 * One case per precedence level, lowest to highest. Each checks that the looser operator is
 * the root, which shows that the tighter one bound first.
 **/
BOOST_AUTO_TEST_CASE(slparser_precedence_test) {
    // the ternary is the loosest, so everything else is inside its arms
    v3d::render::offline::sl::syntax::ExpressionPtr ternary = only("a || b ? c + d : e * f");
    BOOST_REQUIRE(ternary->kind == Expression::Kind::TERNARY);
    const v3d::render::offline::sl::syntax::Ternary & choice =
        static_cast<const v3d::render::offline::sl::syntax::Ternary &>(*ternary);
    BOOST_CHECK_EQUAL(binary(choice.condition), "||");
    BOOST_CHECK_EQUAL(binary(choice.whenTrue), "+");
    BOOST_CHECK_EQUAL(binary(choice.whenFalse), "*");

    // '||' is looser than '&&'
    v3d::render::offline::sl::syntax::ExpressionPtr disjunction = only("a || b && c");
    BOOST_CHECK_EQUAL(binary(disjunction), "||");
    BOOST_CHECK_EQUAL(binary(right(disjunction)), "&&");

    // '&&' is looser than an equality
    v3d::render::offline::sl::syntax::ExpressionPtr conjunction = only("a == b && c");
    BOOST_CHECK_EQUAL(binary(conjunction), "&&");
    BOOST_CHECK_EQUAL(binary(left(conjunction)), "==");

    // an equality is looser than a comparison
    v3d::render::offline::sl::syntax::ExpressionPtr equality = only("a < b == c");
    BOOST_CHECK_EQUAL(binary(equality), "==");
    BOOST_CHECK_EQUAL(binary(left(equality)), "<");

    // a comparison is looser than an addition
    v3d::render::offline::sl::syntax::ExpressionPtr comparison = only("a + b < c");
    BOOST_CHECK_EQUAL(binary(comparison), "<");
    BOOST_CHECK_EQUAL(binary(left(comparison)), "+");

    // an addition is looser than a multiply
    v3d::render::offline::sl::syntax::ExpressionPtr sum = only("a + b * c");
    BOOST_CHECK_EQUAL(binary(sum), "+");
    BOOST_CHECK_EQUAL(binary(right(sum)), "*");

    // a multiply is looser than a dot product
    v3d::render::offline::sl::syntax::ExpressionPtr scaled = only("a * b . c");
    BOOST_CHECK_EQUAL(binary(scaled), "*");
    BOOST_CHECK_EQUAL(binary(right(scaled)), ".");

    // a dot product is looser than a unary minus
    v3d::render::offline::sl::syntax::ExpressionPtr negated = only("-a . b");
    BOOST_CHECK_EQUAL(binary(negated), ".");
    BOOST_CHECK(left(negated)->kind == Expression::Kind::UNARY);

    // the same level runs left to right
    v3d::render::offline::sl::syntax::ExpressionPtr chain = only("a - b - c");
    BOOST_CHECK_EQUAL(binary(chain), "-");
    BOOST_CHECK_EQUAL(binary(left(chain)), "-");
    BOOST_CHECK(right(chain)->kind == Expression::Kind::VARIABLE);

    // parentheses override all of it
    v3d::render::offline::sl::syntax::ExpressionPtr grouped = only("(a + b) * c");
    BOOST_CHECK_EQUAL(binary(grouped), "*");
    BOOST_CHECK_EQUAL(binary(left(grouped)), "+");
}

/**
 * A cast, with the space name that makes it a transform, and the triple it usually wraps.
 * Sixteen elements are a matrix; the compiler checks whether the count matches the type.
 **/
BOOST_AUTO_TEST_CASE(slparser_cast_and_tuple_test) {
    v3d::render::offline::sl::syntax::ExpressionPtr cast = only("point \"world\" (0, 1, 2)");
    BOOST_REQUIRE(cast->kind == Expression::Kind::CAST);
    const v3d::render::offline::sl::syntax::Cast & transform =
        static_cast<const v3d::render::offline::sl::syntax::Cast &>(*cast);
    BOOST_CHECK(transform.type == v3d::render::offline::sl::Type::POINT);
    BOOST_CHECK_EQUAL(transform.space, "world");
    BOOST_REQUIRE(transform.operand->kind == Expression::Kind::TUPLE);
    BOOST_CHECK_EQUAL(static_cast<const v3d::render::offline::sl::syntax::Tuple &>(*transform.operand).elements.size(), 3u);

    // no space named is the shader's current one
    v3d::render::offline::sl::syntax::ExpressionPtr plain = only("color (1, 0, 0)");
    BOOST_REQUIRE(plain->kind == Expression::Kind::CAST);
    BOOST_CHECK_EQUAL(static_cast<const v3d::render::offline::sl::syntax::Cast &>(*plain).space, "");

    // one element in parentheses is that element, not a tuple of one
    v3d::render::offline::sl::syntax::ExpressionPtr single = only("(a)");
    BOOST_CHECK(single->kind == Expression::Kind::VARIABLE);

    // a cast binds like a unary, so what follows it is outside
    v3d::render::offline::sl::syntax::ExpressionPtr outside = only("float a + b");
    BOOST_CHECK_EQUAL(binary(outside), "+");
    BOOST_CHECK(left(outside)->kind == Expression::Kind::CAST);
}

/**
 * The statements, one shader carrying each. A declaration may name more than one variable,
 * and an assignment has the compound forms.
 **/
BOOST_AUTO_TEST_CASE(slparser_statements_test) {
    std::string error;
    std::vector<v3d::render::offline::sl::syntax::ShaderPtr> shaders = parse(
        "surface s() {\n"
        "    float a = 0, b;\n"
        "    uniform float c = 1;\n"
        "    a += 1;\n"
        "    if (a > 0) a = 1; else { a = 2; }\n"
        "    for (a = 0; a < 4; a += 1) { b = b + a; }\n"
        "    while (a > 0) { a -= 1; if (a == 2) break; else continue; }\n"
        "    printf(\"%f\\n\", a);\n"
        "    Ci = color (a, b, c);\n"
        "}\n", &error);

    BOOST_CHECK_EQUAL(error, "");
    BOOST_REQUIRE_EQUAL(shaders.size(), 1u);
    const std::vector<v3d::render::offline::sl::syntax::StatementPtr> & body = shaders[0]->body->statements;
    BOOST_REQUIRE_EQUAL(body.size(), 8u);

    BOOST_REQUIRE(body[0]->kind == Statement::Kind::DECLARATION);
    const v3d::render::offline::sl::syntax::Declaration & declaration =
        static_cast<const v3d::render::offline::sl::syntax::Declaration &>(*body[0]);
    BOOST_REQUIRE_EQUAL(declaration.declarators.size(), 2u);
    BOOST_CHECK_EQUAL(declaration.declarators[0].name, "a");
    BOOST_REQUIRE(declaration.declarators[0].initialiser);
    // the second names no value, which is not the same as naming zero
    BOOST_CHECK(!declaration.declarators[1].initialiser);

    BOOST_REQUIRE(body[1]->kind == Statement::Kind::DECLARATION);
    BOOST_CHECK(static_cast<const v3d::render::offline::sl::syntax::Declaration &>(*body[1]).storage ==
        v3d::render::offline::sl::Storage::UNIFORM);

    BOOST_REQUIRE(body[2]->kind == Statement::Kind::ASSIGNMENT);
    BOOST_CHECK_EQUAL(static_cast<const v3d::render::offline::sl::syntax::Assignment &>(*body[2]).op, "+=");

    BOOST_REQUIRE(body[3]->kind == Statement::Kind::CONDITIONAL);
    BOOST_CHECK(static_cast<const v3d::render::offline::sl::syntax::Conditional &>(*body[3]).whenFalse);

    BOOST_REQUIRE(body[4]->kind == Statement::Kind::FOR);
    const v3d::render::offline::sl::syntax::For & loop = static_cast<const v3d::render::offline::sl::syntax::For &>(*body[4]);
    BOOST_CHECK(loop.initialiser);
    BOOST_CHECK(loop.condition);
    BOOST_CHECK(loop.step);

    BOOST_CHECK(body[5]->kind == Statement::Kind::WHILE);
    BOOST_CHECK(body[6]->kind == Statement::Kind::EXPRESSION);
    BOOST_CHECK(body[7]->kind == Statement::Kind::ASSIGNMENT);
}

/**
 * An else binds to the nearest if, which resolves the dangling else of every C-like grammar.
 **/
BOOST_AUTO_TEST_CASE(slparser_dangling_else_test) {
    std::string error;
    std::vector<v3d::render::offline::sl::syntax::ShaderPtr> shaders = parse(
        "surface s() { if (a) if (b) c = 1; else c = 2; }", &error);

    BOOST_CHECK_EQUAL(error, "");
    BOOST_REQUIRE_EQUAL(shaders.size(), 1u);
    const v3d::render::offline::sl::syntax::Conditional & outer =
        static_cast<const v3d::render::offline::sl::syntax::Conditional &>(*shaders[0]->body->statements[0]);
    // the else belongs to the inner if, so the outer one has none
    BOOST_CHECK(!outer.whenFalse);
    BOOST_REQUIRE(outer.whenTrue->kind == Statement::Kind::CONDITIONAL);
    BOOST_CHECK(static_cast<const v3d::render::offline::sl::syntax::Conditional &>(*outer.whenTrue).whenFalse);
}

/**
 * The three lighting constructs take a body rather than being calls. They are specific to SL.
 **/
BOOST_AUTO_TEST_CASE(slparser_lighting_constructs_test) {
    std::string error;
    std::vector<v3d::render::offline::sl::syntax::ShaderPtr> shaders = parse(
        "surface s() { illuminance(P) { Ci = Ci + Cl; } }", &error);

    BOOST_CHECK_EQUAL(error, "");
    BOOST_REQUIRE_EQUAL(shaders.size(), 1u);
    BOOST_REQUIRE(shaders[0]->body->statements[0]->kind == Statement::Kind::LIGHTING);
    const v3d::render::offline::sl::syntax::Lighting & loop =
        static_cast<const v3d::render::offline::sl::syntax::Lighting &>(*shaders[0]->body->statements[0]);
    BOOST_CHECK(loop.construct == v3d::render::offline::sl::syntax::Lighting::Construct::ILLUMINANCE);
    BOOST_CHECK_EQUAL(loop.arguments.size(), 1u);
    BOOST_REQUIRE(loop.body);
    BOOST_CHECK(loop.body->kind == Statement::Kind::BLOCK);
}

/**
 * A function may be defined inside a shader, and only at the top of its body. The machine has
 * a register file per shader run and no call stack, and the compiler requires this placement.
 **/
BOOST_AUTO_TEST_CASE(slparser_function_test) {
    std::string error;
    std::vector<v3d::render::offline::sl::syntax::ShaderPtr> shaders = parse(
        "surface s(float Ka = 1;) {\n"
        "    float sqr(float x) { return x * x; }\n"
        "    Ci = sqr(Ka);\n"
        "}\n", &error);

    BOOST_CHECK_EQUAL(error, "");
    BOOST_REQUIRE_EQUAL(shaders.size(), 1u);
    BOOST_REQUIRE_EQUAL(shaders[0]->functions.size(), 1u);
    BOOST_CHECK_EQUAL(shaders[0]->functions[0].name, "sqr");
    BOOST_CHECK(shaders[0]->functions[0].type == v3d::render::offline::sl::Type::FLOAT);
    BOOST_REQUIRE_EQUAL(shaders[0]->functions[0].parameters.size(), 1u);
    // a function's formals carry no default, the only difference between the two lists
    BOOST_CHECK(!shaders[0]->functions[0].parameters[0].defaultValue);
    // the function is not left in the body as a statement
    BOOST_CHECK_EQUAL(shaders[0]->body->statements.size(), 1u);
}

BOOST_AUTO_TEST_CASE(slparser_nested_function_rejected_test) {
    std::string error;
    std::vector<v3d::render::offline::sl::syntax::ShaderPtr> shaders = parse(
        "surface s() { if (a) { float sqr(float x) { return x; } } }", &error);

    BOOST_CHECK_EQUAL(shaders.size(), 0u);
    BOOST_CHECK_EQUAL(error, "a function may only be defined at the top of a shader body at line 1, column 30");
}

/**
 * A file may hold more than one shader, so the standard ones can be compiled in as source
 * strings.
 **/
BOOST_AUTO_TEST_CASE(slparser_several_shaders_test) {
    std::string error;
    std::vector<v3d::render::offline::sl::syntax::ShaderPtr> shaders =
        parse(std::string(CONSTANT) + MATTE + AMBIENTLIGHT, &error);

    BOOST_CHECK_EQUAL(error, "");
    BOOST_REQUIRE_EQUAL(shaders.size(), 3u);
    BOOST_CHECK_EQUAL(shaders[0]->name, "constant");
    BOOST_CHECK_EQUAL(shaders[1]->name, "matte");
    BOOST_CHECK_EQUAL(shaders[2]->name, "ambientlight");
}

BOOST_AUTO_TEST_CASE(slparser_empty_source_test) {
    std::string error;
    std::vector<v3d::render::offline::sl::syntax::ShaderPtr> shaders = parse("  // nothing but a comment\n", &error);

    BOOST_CHECK_EQUAL(error, "");
    BOOST_CHECK_EQUAL(shaders.size(), 0u);
}

/**
 * Every diagnostic names a line, a column and what was expected. A parse that fails yields no
 * shaders at all - half a shader is worse than none, because a renderer would run it.
 **/
BOOST_AUTO_TEST_CASE(slparser_error_position_test) {
    std::string error;
    std::vector<v3d::render::offline::sl::syntax::ShaderPtr> shaders = parse(
        "surface good() { Ci = Cs; }\n"
        "surface bad() {\n"
        "    Ci = Cs\n"
        "    Oi = Os;\n"
        "}\n", &error);

    BOOST_CHECK_EQUAL(shaders.size(), 0u);
    BOOST_CHECK_EQUAL(error, "expected ';' but found 'Oi' at line 4, column 5");
}

BOOST_AUTO_TEST_CASE(slparser_expected_expression_test) {
    std::string error;
    parse("surface s() { Ci = ; }", &error);

    BOOST_CHECK_EQUAL(error, "expected an expression but found ';' at line 1, column 20");
}

BOOST_AUTO_TEST_CASE(slparser_truncated_shader_test) {
    std::string error;
    parse("surface s() { Ci = Cs;", &error);

    BOOST_CHECK_EQUAL(error, "expected '}' but found end of source at line 1, column 23");
}

BOOST_AUTO_TEST_CASE(slparser_not_a_shader_test) {
    std::string error;
    parse("float x = 1;", &error);

    BOOST_CHECK_EQUAL(error, "expected a shader type but found 'float' at line 1, column 1");
}

/**
 * A lexer error is reported as itself rather than as the end of source the lexer returns, so
 * a shader that needs the preprocessor is reported as such instead of looking truncated.
 **/
BOOST_AUTO_TEST_CASE(slparser_reports_a_lexer_error_test) {
    std::string error;
    std::vector<v3d::render::offline::sl::syntax::ShaderPtr> shaders =
        parse("#include \"common.h\"\nsurface s() { }", &error);

    BOOST_CHECK_EQUAL(shaders.size(), 0u);
    BOOST_CHECK_EQUAL(error, "the C preprocessor is not run, so a '#' directive cannot be read at line 1, column 1");
}
