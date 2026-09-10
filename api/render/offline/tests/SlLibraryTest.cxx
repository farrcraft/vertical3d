/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/offline/sl/Compiler.h>
#include <api/render/offline/sl/Emitter.h>
#include <api/render/offline/sl/Parser.h>
#include <api/render/offline/sl/runtime/Machine.h>

#include <sstream>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace {

typedef v3d::render::offline::sl::runtime::Machine Machine;
typedef v3d::render::offline::sl::runtime::Program Program;

/**
 * A shader body, run over a batch of one, with its symbols readable afterwards. Every case
 * here is "does this built-in answer what the standard says", which wants a source string
 * and a register rather than a hand-built program.
 **/
class Shaded final {
 public:
    explicit Shaded(const std::string & body, unsigned int batch = 1) {
        const std::string source = "surface test() {\n" + body + "\n}\n";
        std::istringstream stream(source);
        v3d::render::offline::sl::Parser parser(stream);
        std::vector<v3d::render::offline::sl::ShaderPtr> shaders = parser.parse();
        BOOST_REQUIRE_MESSAGE(shaders.size() == 1, parser.error() + " in: " + body);
        v3d::render::offline::sl::Compiler compiler(shaders[0]);
        BOOST_REQUIRE_MESSAGE(compiler.compile(), compiler.error() + " in: " + body);
        v3d::render::offline::sl::Emitter emitter(shaders[0], compiler.symbols());
        BOOST_REQUIRE_MESSAGE(emitter.emit(&program_), emitter.error() + " in: " + body);
        machine_.prepare(program_, batch);
        BOOST_REQUIRE_MESSAGE(machine_.run(program_), machine_.error());
    }

    float number(const std::string & name, unsigned int point = 0) const {
        const int reg = program_.symbol(name);
        BOOST_REQUIRE_MESSAGE(reg >= 0, "no symbol named " + name);
        return machine_.value(reg).number(point);
    }

    glm::vec3 triple(const std::string & name, unsigned int point = 0) const {
        const int reg = program_.symbol(name);
        BOOST_REQUIRE_MESSAGE(reg >= 0, "no symbol named " + name);
        return machine_.value(reg).triple(point);
    }

    const Machine & machine() const {
        return machine_;
    }

 private:
    Program program_;
    Machine machine_;
};

/**
 * The float a one line expression comes to, which is what most of the library is asserted
 * through.
 **/
float answer(const std::string & expression) {
    return Shaded("float answer = " + expression + ";").number("answer");
}

};  // namespace

/**
 * The maths, each against the value the standard gives it. They are one loop over the
 * components in the machine, so a case per name is what says the name reached the right
 * body rather than the one beside it in the table.
 **/
BOOST_AUTO_TEST_CASE(sllibrary_maths_test) {
    BOOST_CHECK_CLOSE(answer("abs(-3)"), 3.0f, 0.01f);
    BOOST_CHECK_CLOSE(answer("sign(-3)"), -1.0f, 0.01f);
    BOOST_CHECK_SMALL(answer("sign(0)"), 0.0001f);
    BOOST_CHECK_CLOSE(answer("floor(1.7)"), 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(answer("ceil(1.2)"), 2.0f, 0.01f);
    BOOST_CHECK_CLOSE(answer("round(1.5)"), 2.0f, 0.01f);
    BOOST_CHECK_CLOSE(answer("sqrt(9)"), 3.0f, 0.01f);
    BOOST_CHECK_CLOSE(answer("pow(2, 10)"), 1024.0f, 0.01f);
    BOOST_CHECK_CLOSE(answer("exp(0)"), 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(answer("log(1)"), 0.0f, 0.01f);
    // the two argument form is a logarithm to a base rather than a second natural log
    BOOST_CHECK_CLOSE(answer("log(8, 2)"), 3.0f, 0.01f);
    BOOST_CHECK_CLOSE(answer("min(4, 7)"), 4.0f, 0.01f);
    BOOST_CHECK_CLOSE(answer("max(4, 7)"), 7.0f, 0.01f);
    BOOST_CHECK_CLOSE(answer("clamp(9, 0, 5)"), 5.0f, 0.01f);
    BOOST_CHECK_CLOSE(answer("mix(10, 20, 0.25)"), 12.5f, 0.01f);
    BOOST_CHECK_CLOSE(answer("radians(180)"), 3.14159f, 0.01f);
    BOOST_CHECK_CLOSE(answer("degrees(3.14159265)"), 180.0f, 0.01f);
    BOOST_CHECK_CLOSE(answer("sin(0) + cos(0)"), 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(answer("tan(0.7853981)"), 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(answer("degrees(asin(1))"), 90.0f, 0.01f);
    BOOST_CHECK_CLOSE(answer("degrees(acos(0))"), 90.0f, 0.01f);
    BOOST_CHECK_CLOSE(answer("degrees(atan(1))"), 45.0f, 0.01f);
    BOOST_CHECK_CLOSE(answer("degrees(atan(1, 1))"), 45.0f, 0.01f);
}

/**
 * RI's mod takes the sign of its divisor rather than of its dividend, which is what makes a
 * value walked backwards round a period stay inside it. C's fmod does the opposite, so this
 * is the one piece of arithmetic here that is not the obvious call.
 **/
BOOST_AUTO_TEST_CASE(sllibrary_mod_test) {
    BOOST_CHECK_CLOSE(answer("mod(7, 3)"), 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(answer("mod(-1, 3)"), 2.0f, 0.01f);
}

/**
 * step is a threshold and smoothstep is the Hermite between two of them, and the arguments
 * of each are the edges first and the value last.
 **/
BOOST_AUTO_TEST_CASE(sllibrary_step_test) {
    BOOST_CHECK_SMALL(answer("step(0.5, 0.2)"), 0.0001f);
    BOOST_CHECK_CLOSE(answer("step(0.5, 0.9)"), 1.0f, 0.01f);
    BOOST_CHECK_SMALL(answer("smoothstep(0, 1, -1)"), 0.0001f);
    BOOST_CHECK_CLOSE(answer("smoothstep(0, 1, 2)"), 1.0f, 0.01f);
    // the midpoint of the curve, which is the only value of it that is also the linear one
    BOOST_CHECK_CLOSE(answer("smoothstep(0, 1, 0.5)"), 0.5f, 0.01f);
    BOOST_CHECK_CLOSE(answer("smoothstep(0, 1, 0.25)"), 0.15625f, 0.01f);
}

/**
 * A componentwise built-in over a colour is that function of each of the three, and a float
 * argument beside a colour one is read for all three. That is RI's promotion rather than a
 * zero fill, and it is what "mix(Cs, Cl, 0.5)" leans on.
 **/
BOOST_AUTO_TEST_CASE(sllibrary_componentwise_over_a_colour_test) {
    const Shaded shaded("color answer = mix(color (0, 10, 20), color (10, 20, 40), 0.5);");
    BOOST_CHECK_CLOSE(shaded.triple("answer").r, 5.0f, 0.01f);
    BOOST_CHECK_CLOSE(shaded.triple("answer").g, 15.0f, 0.01f);
    BOOST_CHECK_CLOSE(shaded.triple("answer").b, 30.0f, 0.01f);
}

/**
 * The geometry that is a number out of directions.
 **/
BOOST_AUTO_TEST_CASE(sllibrary_geometry_test) {
    BOOST_CHECK_CLOSE(answer("length(vector (3, 4, 0))"), 5.0f, 0.01f);
    BOOST_CHECK_CLOSE(answer("distance(point (1, 0, 0), point (1, 3, 4))"), 5.0f, 0.01f);

    const Shaded shaded("vector answer = normalize(vector (0, 0, 5));");
    BOOST_CHECK_CLOSE(shaded.triple("answer").z, 1.0f, 0.01f);

    // a zero direction has no unit, and a shader that normalized one renders black rather
    // than rendering nothing at all
    const Shaded zero("vector answer = normalize(vector (0, 0, 0));");
    BOOST_CHECK_SMALL(zero.triple("answer").z, 0.0001f);
}

/**
 * faceforward turns a normal to the side its reference is on, hand worked: against an
 * incident direction coming from in front, the normal is left alone; from behind, it is
 * flipped. The two argument form has no Ng to hand and uses the normal as its own
 * reference.
 **/
BOOST_AUTO_TEST_CASE(sllibrary_faceforward_test) {
    const Shaded toward("normal answer = faceforward(normal (0, 0, 1), vector (0, 0, -1));");
    BOOST_CHECK_CLOSE(toward.triple("answer").z, 1.0f, 0.01f);

    const Shaded away("normal answer = faceforward(normal (0, 0, 1), vector (0, 0, 1));");
    BOOST_CHECK_CLOSE(away.triple("answer").z, -1.0f, 0.01f);

    // three arguments, where the reference disagrees with the normal and wins
    const Shaded reference(
        "normal answer = faceforward(normal (0, 0, 1), vector (0, 0, -1), normal (0, 0, -1));");
    BOOST_CHECK_CLOSE(reference.triple("answer").z, -1.0f, 0.01f);
}

/**
 * reflect and refract against hand-worked values. A ray coming down onto a surface facing
 * up reflects back up; through equal media it carries straight on; and at a grazing angle
 * into a denser one it turns toward the normal.
 **/
BOOST_AUTO_TEST_CASE(sllibrary_reflect_and_refract_test) {
    const Shaded mirrored("vector answer = reflect(vector (1, 0, -1), normal (0, 0, 1));");
    BOOST_CHECK_CLOSE(mirrored.triple("answer").x, 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(mirrored.triple("answer").z, 1.0f, 0.01f);

    // eta of one is no boundary at all, so the direction is unchanged
    const Shaded straight("vector answer = refract(vector (0, 0, -1), normal (0, 0, 1), 1);");
    BOOST_CHECK_CLOSE(straight.triple("answer").z, -1.0f, 0.01f);

    /*
        45 degrees into a medium of eta 0.5: sin of the refracted angle is half the sin of
        the incident one, so the answer is (0.35355, 0, -0.93541) to five places.
    */
    const Shaded bent(
        "vector answer = refract(vector (0.70710678, 0, -0.70710678), normal (0, 0, 1), 0.5);");
    BOOST_CHECK_CLOSE(bent.triple("answer").x, 0.35355f, 0.1f);
    BOOST_CHECK_CLOSE(bent.triple("answer").z, -0.93541f, 0.1f);

    // past the critical angle nothing is refracted at all, which is total internal reflection
    const Shaded trapped(
        "vector answer = refract(vector (0.99, 0, -0.141), normal (0, 0, 1), 2);");
    BOOST_CHECK_SMALL(trapped.triple("answer").x, 0.0001f);
    BOOST_CHECK_SMALL(trapped.triple("answer").z, 0.0001f);
}

/**
 * The component accessors and their setters, by name and by index. A setter writes the
 * value it was handed rather than answering one, which is the only shape in the library
 * that does.
 **/
BOOST_AUTO_TEST_CASE(sllibrary_components_test) {
    BOOST_CHECK_CLOSE(answer("xcomp(point (7, 8, 9))"), 7.0f, 0.01f);
    BOOST_CHECK_CLOSE(answer("ycomp(point (7, 8, 9))"), 8.0f, 0.01f);
    BOOST_CHECK_CLOSE(answer("zcomp(point (7, 8, 9))"), 9.0f, 0.01f);
    BOOST_CHECK_CLOSE(answer("comp(color (7, 8, 9), 1)"), 8.0f, 0.01f);

    const Shaded written(
        "point answer = point (1, 2, 3);\n"
        "setycomp(answer, 20);\n"
        "setcomp(answer, 2, 30);");
    BOOST_CHECK_CLOSE(written.triple("answer").x, 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(written.triple("answer").y, 20.0f, 0.01f);
    BOOST_CHECK_CLOSE(written.triple("answer").z, 30.0f, 0.01f);
}

/**
 * The matrix built-ins, over the identity. determinant is the one that says whether the
 * others built what they claimed.
 **/
BOOST_AUTO_TEST_CASE(sllibrary_matrix_test) {
    BOOST_CHECK_CLOSE(answer("determinant(matrix 1)"), 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(answer("determinant(scale(matrix 1, vector (2, 3, 4)))"), 24.0f, 0.01f);
    // a rotation and a translation each leave the volume alone, which a determinant of one says
    BOOST_CHECK_CLOSE(answer("determinant(translate(matrix 1, vector (5, 6, 7)))"), 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(answer("determinant(rotate(matrix 1, 0.5, vector (0, 1, 0)))"), 1.0f, 0.01f);
}

/**
 * A named coordinate space is the renderer's answer. With none attached the value arrives
 * in the space it was already in and the machine says so once, rather than a scene silently
 * rendering in the wrong place.
 **/
BOOST_AUTO_TEST_CASE(sllibrary_transform_without_a_renderer_test) {
    const Shaded shaded("point answer = ptransform(\"world\", point (1, 2, 3));");
    BOOST_CHECK_CLOSE(shaded.triple("answer").x, 1.0f, 0.01f);
    BOOST_REQUIRE_EQUAL(shaded.machine().reports().size(), 1u);
    BOOST_CHECK_EQUAL(shaded.machine().reports()[0],
        "the coordinate space \"world\" is not one this renderer knows");
}

/**
 * printf is how anyone debugs a shader, so it is a line per shading point rather than one
 * line however many points there are - which is the opposite of every other thing the
 * machine says, and is what the person who typed it asked for.
 **/
BOOST_AUTO_TEST_CASE(sllibrary_printf_test) {
    const Shaded shaded(
        "color tint = color (0.5, 0, 1);\n"
        "printf(\"s is %f and the tint is %c\\n\", s, tint);", 3);
    BOOST_REQUIRE_EQUAL(shaded.machine().printed().size(), 3u);
    BOOST_CHECK(shaded.machine().printed()[0].find("s is 0.000000") != std::string::npos);
    BOOST_CHECK(shaded.machine().printed()[0].find("(0.500000 0.000000 1.000000)") != std::string::npos);
    // no report: a printf that printed is not a thing the machine could not do
    BOOST_CHECK(shaded.machine().reports().empty());
}

/**
 * A conversion with nothing left to print says so rather than reading past the arguments,
 * which is the one printf mistake that would otherwise take a render down.
 **/
BOOST_AUTO_TEST_CASE(sllibrary_printf_missing_argument_test) {
    const Shaded shaded("printf(\"%f and %f\", 1);");
    BOOST_REQUIRE_EQUAL(shaded.machine().printed().size(), 1u);
    BOOST_CHECK_EQUAL(shaded.machine().printed()[0], "1.000000 and (missing)");
}

/**
 * A stub answers its default and says so exactly once, however many points ran it: a scene
 * that rendered nothing and a scene that was not understood look identical from outside,
 * and a 640 by 480 render must not print a million lines to tell them apart.
 **/
BOOST_AUTO_TEST_CASE(sllibrary_stubs_report_once_test) {
    const Shaded shaded(
        "float a = noise(s);\n"
        "float b = noise(t);\n"
        "color c = texture(\"nowhere.tx\");\n"
        "float d = shadow(\"nowhere.shd\", P);\n"
        "Ci = c * (a + b + d);", 16);

    BOOST_CHECK_SMALL(shaded.number("a"), 0.0001f);
    BOOST_CHECK_SMALL(shaded.triple("c").r, 0.0001f);

    const std::vector<std::string> & reports = shaded.machine().reports();
    BOOST_REQUIRE_EQUAL(reports.size(), 3u);
    // two calls to noise over sixteen points is one line, and the three names are three
    BOOST_CHECK_EQUAL(reports[0],
        "'noise' is declared and does nothing yet, so it answers its default");
    BOOST_CHECK_EQUAL(reports[1],
        "'texture' is declared and does nothing yet, so it answers its default");
    BOOST_CHECK_EQUAL(reports[2],
        "'shadow' is declared and does nothing yet, so it answers its default");
}
