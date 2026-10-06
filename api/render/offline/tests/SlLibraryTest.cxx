/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/image/Image.h>
#include <api/render/offline/Noise.h>
#include <api/render/offline/Texture.h>
#include <api/render/offline/sl/Builtins.h>
#include <api/render/offline/sl/Compiler.h>
#include <api/render/offline/sl/Emitter.h>
#include <api/render/offline/sl/Parser.h>
#include <api/render/offline/sl/runtime/Machine.h>
#include <api/render/offline/sl/runtime/Renderer.h>
#include <api/render/offline/sl/syntax/Shader.h>

#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace {

typedef v3d::render::offline::sl::runtime::Machine Machine;
typedef v3d::render::offline::sl::runtime::Program Program;

/**
 * A shader body, run over a batch of one, with its symbols readable afterwards. Every case
 * here checks that a built-in returns what the standard says, which needs a source string
 * and a register rather than a hand-built program.
 **/
class Shaded final {
 public:
    explicit Shaded(const std::string & body, unsigned int batch = 1,
        v3d::render::offline::sl::runtime::Renderer* renderer = nullptr) {
        const std::string source = "surface test() {\n" + body + "\n}\n";
        std::istringstream stream(source);
        v3d::render::offline::sl::Parser parser(stream);
        std::vector<v3d::render::offline::sl::syntax::ShaderPtr> shaders = parser.parse();
        BOOST_REQUIRE_MESSAGE(shaders.size() == 1, parser.error() + " in: " + body);
        v3d::render::offline::sl::Compiler compiler(shaders[0]);
        BOOST_REQUIRE_MESSAGE(compiler.compile(), compiler.error() + " in: " + body);
        v3d::render::offline::sl::Emitter emitter(shaders[0], compiler.symbols());
        BOOST_REQUIRE_MESSAGE(emitter.emit(&program_), emitter.error() + " in: " + body);
        machine_.renderer(renderer);
        machine_.prepare(program_, batch);
        BOOST_REQUIRE_MESSAGE(machine_.run(), machine_.error());
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
 * The float a one line expression evaluates to. Most of the library is tested through this.
 **/
float answer(const std::string & expression) {
    return Shaded("float answer = " + expression + ";").number("answer");
}

};  // namespace

/**
 * The maths, each against the value the standard gives it. They share one loop over the
 * components in the machine, so a case per name checks that each name reaches its own body
 * rather than its neighbour in the table.
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
 * RI's mod takes the sign of its divisor rather than of its dividend, so a value decreasing
 * past zero stays inside the period. C's fmod does the opposite, so mod is not a plain call
 * to it.
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
 * argument beside a colour one is read for all three, as RI promotes it, rather than a zero
 * fill. "mix(Cs, Cl, 0.5)" relies on this.
 **/
BOOST_AUTO_TEST_CASE(sllibrary_componentwise_over_a_colour_test) {
    const Shaded shaded("color answer = mix(color (0, 10, 20), color (10, 20, 40), 0.5);");
    BOOST_CHECK_CLOSE(shaded.triple("answer").r, 5.0f, 0.01f);
    BOOST_CHECK_CLOSE(shaded.triple("answer").g, 15.0f, 0.01f);
    BOOST_CHECK_CLOSE(shaded.triple("answer").b, 30.0f, 0.01f);
}

/**
 * The result of a componentwise built-in is the type its arguments promote to, so a float
 * first does not make the result a float. "max(0, Ci)" is three maxima rather than one
 * repeated three times. The one argument maths takes a triple as well as a float.
 **/
BOOST_AUTO_TEST_CASE(sllibrary_componentwise_promotes_test) {
    const Shaded shaded(
        "color floored = max(0, color (-1, 0.5, 2));\n"
        "color mixed = mix(0, color (2, 4, 6), 0.5);\n"
        "color absolute = abs(color (-1, -2, 3));\n"
        "color squared = pow(color (1, 2, 3), 2);\n"
        "point rooted = sqrt(point (4, 9, 16));");

    BOOST_CHECK_SMALL(shaded.triple("floored").r, 0.0001f);
    BOOST_CHECK_CLOSE(shaded.triple("floored").g, 0.5f, 0.01f);
    BOOST_CHECK_CLOSE(shaded.triple("floored").b, 2.0f, 0.01f);
    BOOST_CHECK_CLOSE(shaded.triple("mixed").r, 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(shaded.triple("mixed").b, 3.0f, 0.01f);
    BOOST_CHECK_CLOSE(shaded.triple("absolute").r, 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(shaded.triple("absolute").g, 2.0f, 0.01f);
    BOOST_CHECK_CLOSE(shaded.triple("squared").b, 9.0f, 0.01f);
    BOOST_CHECK_CLOSE(shaded.triple("rooted").x, 2.0f, 0.01f);
    BOOST_CHECK_CLOSE(shaded.triple("rooted").z, 4.0f, 0.01f);
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
 * flipped. The two argument form has no Ng and uses the normal as its own reference.
 **/
BOOST_AUTO_TEST_CASE(sllibrary_faceforward_test) {
    const Shaded toward("normal answer = faceforward(normal (0, 0, 1), vector (0, 0, -1));");
    BOOST_CHECK_CLOSE(toward.triple("answer").z, 1.0f, 0.01f);

    const Shaded away("normal answer = faceforward(normal (0, 0, 1), vector (0, 0, 1));");
    BOOST_CHECK_CLOSE(away.triple("answer").z, -1.0f, 0.01f);

    // three arguments, where the reference faces the other way from the normal and decides
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
        the incident one, so the result is (0.35355, 0, -0.93541) to five places.
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
 * fresnel against the dielectric formulas, worked by hand for glass of index 1.5 seen from
 * air, which is an eta of 1 / 1.5.
 *
 * Straight on, the reflectance is ((1 - eta) / (1 + eta)) squared, 0.04, whichever way
 * the light crosses. At 45 degrees the two polarisations differ, 0.0920 and 0.0085, and the
 * result is their mean. Past the critical angle all of it is reflected.
 **/
BOOST_AUTO_TEST_CASE(sllibrary_fresnel_test) {
    const Shaded straight("float kr = 0; float kt = 0; vector R = 0; vector T = 0;\n"
        "fresnel(vector (0, 0, -1), normal (0, 0, 1), 1 / 1.5, kr, kt, R, T);");
    BOOST_CHECK_CLOSE(straight.number("kr"), 0.04f, 0.1f);
    BOOST_CHECK_CLOSE(straight.number("kt"), 0.96f, 0.1f);
    BOOST_CHECK_CLOSE(straight.triple("R").z, 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(straight.triple("T").z, -1.0f, 0.01f);

    const Shaded angled("float kr = 0; float kt = 0;\n"
        "fresnel(vector (1, 0, -1), normal (0, 0, 1), 1 / 1.5, kr, kt);");
    BOOST_CHECK_CLOSE(angled.number("kr"), 0.050241f, 0.1f);
    BOOST_CHECK_CLOSE(angled.number("kt"), 1.0f - 0.050241f, 0.1f);

    const Shaded trapped("float kr = 0; float kt = 0; vector R = 0; vector T = 0;\n"
        "fresnel(vector (0.99, 0, -0.141), normal (0, 0, 1), 2, kr, kt, R, T);");
    BOOST_CHECK_CLOSE(trapped.number("kr"), 1.0f, 0.01f);
    BOOST_CHECK_SMALL(trapped.number("kt"), 0.0001f);
    BOOST_CHECK_SMALL(glm::length(trapped.triple("T")), 0.0001f);
}

/**
 * The component accessors and their setters, by name and by index. A setter writes the
 * value it was given rather than returning one; no other built-in does that.
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
 * The matrix built-ins, over the identity. determinant checks that the others built the
 * matrices they should.
 **/
BOOST_AUTO_TEST_CASE(sllibrary_matrix_test) {
    BOOST_CHECK_CLOSE(answer("determinant(matrix 1)"), 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(answer("determinant(scale(matrix 1, vector (2, 3, 4)))"), 24.0f, 0.01f);
    // a rotation and a translation each preserve volume, so the determinant is one
    BOOST_CHECK_CLOSE(answer("determinant(translate(matrix 1, vector (5, 6, 7)))"), 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(answer("determinant(rotate(matrix 1, 0.5, vector (0, 1, 0)))"), 1.0f, 0.01f);
}

/**
 * The renderer resolves a named coordinate space. With none attached the value arrives in
 * the space it was already in and the machine reports it once, rather than a scene silently
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
 * printf is for debugging a shader, so it writes a line per shading point rather than one
 * line however many points there are. Every other message from the machine is reported
 * once.
 **/
BOOST_AUTO_TEST_CASE(sllibrary_printf_test) {
    const Shaded shaded(
        "color tint = color (0.5, 0, 1);\n"
        "printf(\"s is %f and the tint is %c\\n\", s, tint);", 3);
    BOOST_REQUIRE_EQUAL(shaded.machine().printed().size(), 3u);
    BOOST_CHECK(shaded.machine().printed()[0].find("s is 0.000000") != std::string::npos);
    BOOST_CHECK(shaded.machine().printed()[0].find("(0.500000 0.000000 1.000000)") != std::string::npos);
    // no report: reports are only for what the machine could not do
    BOOST_CHECK(shaded.machine().reports().empty());
}

/**
 * A conversion with nothing left to print is reported rather than reading past the
 * arguments, which would crash the render.
 **/
BOOST_AUTO_TEST_CASE(sllibrary_printf_missing_argument_test) {
    const Shaded shaded("printf(\"%f and %f\", 1);");
    BOOST_REQUIRE_EQUAL(shaded.machine().printed().size(), 1u);
    BOOST_CHECK_EQUAL(shaded.machine().printed()[0], "1.000000 and (missing)");
}

/**
 * A stub returns its default and is reported exactly once, however many points ran it.
 **/
BOOST_AUTO_TEST_CASE(sllibrary_stubs_report_once_test) {
    const Shaded shaded(
        "float a = shadow(\"nowhere.shd\", P);\n"
        "float b = shadow(\"nowhere.shd\", P + 1);\n"
        "Ci = a + b;", 16);

    BOOST_CHECK_SMALL(shaded.number("a"), 0.0001f);

    const std::vector<std::string> & reports = shaded.machine().reports();
    BOOST_REQUIRE_EQUAL(reports.size(), 1u);
    BOOST_CHECK_EQUAL(reports[0],
        "'shadow' is declared and does nothing yet, so it answers its default");
}

namespace {

/**
 * Two by two texels: red and green across the top, blue and white across the bottom.
 **/
v3d::render::offline::Texture quartered() {
    v3d::image::Image image(2, 2, 24);
    const unsigned char texels[12] = {
        255, 0, 0,   0, 255, 0,
        0, 0, 255,   255, 255, 255
    };
    for (unsigned int i = 0; i < 12; i++) {
        image[i] = texels[i];
    }
    return v3d::render::offline::Texture(image);
}

/**
 * A renderer holding one texture, under the name "quarters".
 **/
class Textured final : public v3d::render::offline::sl::runtime::Renderer {
 public:
    bool space(const std::string & /* name */, glm::mat4x4* /* matrix */) override {
        return false;
    }

    const v3d::render::offline::Texture* texture(const std::string & name) override {
        return name == "quarters" ? &texture_ : nullptr;
    }

 private:
    v3d::render::offline::Texture texture_ = quartered();
};

};  // namespace

/**
 * texture() reads the renderer's image at the s and t it is given, or at the shader's own
 * when it is given none, and a cast to float takes the first channel.
 **/
BOOST_AUTO_TEST_CASE(sllibrary_texture_test) {
    Textured renderer;
    const Shaded shaded(
        "color placed = texture(\"quarters\", 0.75, 0.25);\n"
        "color between = texture(\"quarters\", 0.5, 0.75);\n"
        "color own = texture(\"quarters\");\n"
        "float red = float texture(\"quarters\", 0.25, 0.25);", 1, &renderer);

    BOOST_CHECK_SMALL(glm::length(shaded.triple("placed") - glm::vec3(0.0f, 1.0f, 0.0f)), 0.0001f);
    BOOST_CHECK_SMALL(glm::length(shaded.triple("between") - glm::vec3(0.5f, 0.5f, 1.0f)), 0.0001f);
    // a batch of one has s and t at zero, which is the corner the four texels meet at
    BOOST_CHECK_SMALL(glm::length(shaded.triple("own") - glm::vec3(0.5f, 0.5f, 0.5f)), 0.0001f);
    BOOST_CHECK_CLOSE(shaded.number("red"), 1.0f, 0.01f);
    BOOST_CHECK(shaded.machine().reports().empty());
}

/**
 * A name that cannot be read returns black, and is reported once however many points read it.
 **/
BOOST_AUTO_TEST_CASE(sllibrary_missing_texture_test) {
    Textured renderer;
    const Shaded shaded(
        "color a = texture(\"nowhere.png\", s, t);\n"
        "color b = texture(\"nowhere.png\");", 16, &renderer);

    BOOST_CHECK_SMALL(glm::length(shaded.triple("a", 7)), 0.0001f);
    const std::vector<std::string> & reports = shaded.machine().reports();
    BOOST_REQUIRE_EQUAL(reports.size(), 1u);
    BOOST_CHECK_EQUAL(reports[0], "the texture \"nowhere.png\" cannot be read, so it answers black");
}

/**
 * noise() of a float, a pair and a point is a float, and a cast to colour gives three, each
 * its own pattern: a colour of noise is not grey.
 **/
BOOST_AUTO_TEST_CASE(sllibrary_noise_test) {
    const Shaded shaded(
        "float line = noise(1.3);\n"
        "float plane = noise(1.3, 0);\n"
        "float space = noise(point (1.3, 0.7, 2.1));\n"
        "color tint = color noise(point (1.3, 0.7, 2.1));\n"
        "point moved = point noise(2.6);");

    BOOST_CHECK_EQUAL(shaded.number("line"), shaded.number("plane"));
    BOOST_CHECK_EQUAL(shaded.number("line"), v3d::render::offline::noise(glm::vec3(1.3f, 0.0f, 0.0f)));
    BOOST_CHECK_EQUAL(shaded.number("space"), v3d::render::offline::noise(glm::vec3(1.3f, 0.7f, 2.1f)));
    const glm::vec3 tint = shaded.triple("tint");
    BOOST_CHECK_EQUAL(tint.r, shaded.number("space"));
    BOOST_CHECK_NE(tint.r, tint.g);
    BOOST_CHECK_NE(tint.g, tint.b);
    for (int i = 0; i < 3; i++) {
        BOOST_CHECK_GE(shaded.triple("moved")[i], 0.0f);
        BOOST_CHECK_LE(shaded.triple("moved")[i], 1.0f);
    }
    BOOST_CHECK(shaded.machine().reports().empty());
}

/**
 * Two strings are equal by their text, so a shader can check whether it was given a texture
 * name at all.
 **/
BOOST_AUTO_TEST_CASE(sllibrary_string_comparison_test) {
    const Shaded shaded(
        "string name = \"blocks.png\";\n"
        "float named = name != \"\";\n"
        "float empty = name == \"\";\n"
        "float same = name == \"blocks.png\";");
    BOOST_CHECK_EQUAL(shaded.number("named"), 1.0f);
    BOOST_CHECK_EQUAL(shaded.number("empty"), 0.0f);
    BOOST_CHECK_EQUAL(shaded.number("same"), 1.0f);
}

namespace {

/**
 * A renderer that defines two spaces, so that the transforming built-ins have something to
 * transform through.
 *
 * "world" both scales one axis and translates, so the three transforms give different
 * results. Under a rotation or a uniform scale a normal and a vector transform the same way;
 * under a non-uniform scale they do not.
 **/
class Spaces final : public v3d::render::offline::sl::runtime::Renderer {
 public:
    bool space(const std::string & name, glm::mat4x4* matrix) override {
        if (name == "world") {
            *matrix = glm::translate(glm::mat4x4(1.0f), glm::vec3(2.0f, 3.0f, 4.0f)) *
                glm::scale(glm::mat4x4(1.0f), glm::vec3(1.0f, 1.0f, 2.0f));
            return true;
        }
        if (name == "NDC") {
            *matrix = glm::scale(glm::mat4x4(1.0f), glm::vec3(1.0f, 1.0f, 0.1f));
            return true;
        }
        return false;
    }
};

};  // namespace

/**
 * The three transforms against one matrix that scales an axis and translates. A point
 * translates, a vector does not, and a normal goes by the inverse transpose. Using the wrong
 * one is invisible under any uniform scale.
 **/
BOOST_AUTO_TEST_CASE(sllibrary_transforms_test) {
    Spaces renderer;
    const Shaded shaded(
        "point moved = ptransform(\"world\", point (1, 1, 1));\n"
        "vector turned = vtransform(\"world\", vector (1, 1, 1));\n"
        "normal tilted = ntransform(\"world\", normal (0, 0, 1));\n"
        "float away = depth(point (0, 0, 5));", 1, &renderer);

    // scaled to (1, 1, 2) and then moved by (2, 3, 4)
    BOOST_CHECK_CLOSE(shaded.triple("moved").x, 3.0f, 0.01f);
    BOOST_CHECK_CLOSE(shaded.triple("moved").z, 6.0f, 0.01f);
    // the same scale with no move, because a direction has no position to move
    BOOST_CHECK_CLOSE(shaded.triple("turned").x, 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(shaded.triple("turned").z, 2.0f, 0.01f);
    // the inverse transpose, which is the reciprocal of that scale rather than the scale
    BOOST_CHECK_CLOSE(shaded.triple("tilted").z, 0.5f, 0.01f);
    BOOST_CHECK_CLOSE(shaded.number("away"), 0.5f, 0.01f);
    BOOST_CHECK(shaded.machine().reports().empty());
}

/**
 * Two space names is out of the first and into the second, so naming one space twice is
 * the identity however far from the shader's own space it is.
 **/
BOOST_AUTO_TEST_CASE(sllibrary_transform_between_two_spaces_test) {
    Spaces renderer;
    const Shaded shaded(
        "point same = ptransform(\"world\", \"world\", point (1, 1, 1));\n"
        "point out = ptransform(\"world\", \"current\", point (3, 4, 6));", 1, &renderer);

    BOOST_CHECK_CLOSE(shaded.triple("same").x, 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(shaded.triple("same").z, 1.0f, 0.01f);
    // back the other way, which undoes the case above
    BOOST_CHECK_CLOSE(shaded.triple("out").x, 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(shaded.triple("out").z, 1.0f, 0.01f);
}

/**
 * The order of matrix composition, against literals written as RenderMan writes them. A
 * literal is row major, and a point is a row vector on the left, so its last row is the
 * translation. The two scales and the translation are chosen so that every order gives a
 * different literal.
 *
 * "world" is checked first, against the point ptransform moves, so the literals are read the
 * way the renderer's transforms are. A * B applies A and then B, and A / B is A times the
 * inverse of B. translate, rotate and scale apply their own transform before the matrix they
 * are given, as ConcatTransform does. mtransform applies its matrix before the space change.
 **/
BOOST_AUTO_TEST_CASE(sllibrary_matrix_order_test) {
    Spaces renderer;
    const Shaded shaded(
        "matrix world = mtransform(\"world\", matrix 1);\n"
        "matrix moved = translate(matrix 1, vector (1, 0, 0));\n"
        "matrix doubled = scale(matrix 1, vector (2, 2, 2));\n"
        "float worldIsLiteral = world == matrix (1, 0, 0, 0,  0, 1, 0, 0,  0, 0, 2, 0,  2, 3, 4, 1);\n"
        "float moveThenDouble = moved * doubled == matrix (2, 0, 0, 0,  0, 2, 0, 0,  0, 0, 2, 0,  2, 0, 0, 1);\n"
        "float doubleThenMove = doubled * moved == matrix (2, 0, 0, 0,  0, 2, 0, 0,  0, 0, 2, 0,  1, 0, 0, 1);\n"
        "float moveThenHalve = moved / doubled == matrix (0.5, 0, 0, 0,  0, 0.5, 0, 0,  0, 0, 0.5, 0,  0.5, 0, 0, 1);\n"
        "float translateFirst = translate(doubled, vector (1, 0, 0)) == moved * doubled;\n"
        "float scaleFirst = scale(moved, vector (2, 2, 2)) == doubled * moved;\n"
        "float rotateFirst = rotate(moved, radians(90), vector (0, 0, 1)) ==\n"
        "    rotate(matrix 1, radians(90), vector (0, 0, 1)) * moved;\n"
        "float spaceLast = mtransform(\"world\", doubled) ==\n"
        "    matrix (2, 0, 0, 0,  0, 2, 0, 0,  0, 0, 4, 0,  2, 3, 4, 1);", 1, &renderer);

    // ptransform moves (1, 1, 1) to (3, 4, 6) through "world": scaled by (1, 1, 2), then moved
    // by (2, 3, 4), which is the literal's last row
    BOOST_CHECK_EQUAL(shaded.number("worldIsLiteral"), 1.0f);
    BOOST_CHECK_EQUAL(shaded.number("moveThenDouble"), 1.0f);
    BOOST_CHECK_EQUAL(shaded.number("doubleThenMove"), 1.0f);
    BOOST_CHECK_EQUAL(shaded.number("moveThenHalve"), 1.0f);
    BOOST_CHECK_EQUAL(shaded.number("translateFirst"), 1.0f);
    BOOST_CHECK_EQUAL(shaded.number("scaleFirst"), 1.0f);
    BOOST_CHECK_EQUAL(shaded.number("rotateFirst"), 1.0f);
    BOOST_CHECK_EQUAL(shaded.number("spaceLast"), 1.0f);
    BOOST_CHECK(shaded.machine().reports().empty());
}

/**
 * A matrix through a space is the two composed, and a colour through one is the colour:
 * there is one colour space here and it is the one a framebuffer holds, so a scene naming
 * another gets its colours back unchanged and a report. With two spaces named, either one
 * that is not rgb is reported.
 **/
BOOST_AUTO_TEST_CASE(sllibrary_matrix_and_colour_spaces_test) {
    Spaces renderer;
    const Shaded shaded(
        "float bulk = determinant(mtransform(\"world\", matrix 1));\n"
        "color kept = ctransform(\"rgb\", color (0.25, 0.5, 0.75));\n"
        "color other = ctransform(\"hsv\", color (0.25, 0.5, 0.75));\n"
        "color both = ctransform(\"rgb\", \"rgb\", color (0.25, 0.5, 0.75));\n"
        "color into = ctransform(\"rgb\", \"xyz\", color (0.25, 0.5, 0.75));", 1, &renderer);

    // the world matrix scales one axis by two and nothing else changes a volume
    BOOST_CHECK_CLOSE(shaded.number("bulk"), 2.0f, 0.01f);
    BOOST_CHECK_CLOSE(shaded.triple("kept").g, 0.5f, 0.01f);
    BOOST_CHECK_CLOSE(shaded.triple("other").g, 0.5f, 0.01f);
    BOOST_CHECK_CLOSE(shaded.triple("both").g, 0.5f, 0.01f);
    BOOST_CHECK_CLOSE(shaded.triple("into").g, 0.5f, 0.01f);
    BOOST_REQUIRE_EQUAL(shaded.machine().reports().size(), 2u);
    BOOST_CHECK_EQUAL(shaded.machine().reports()[0],
        "the colour space \"hsv\" is not one this renderer knows");
    BOOST_CHECK_EQUAL(shaded.machine().reports()[1],
        "the colour space \"xyz\" is not one this renderer knows");
}

/**
 * A space in front of a colour cast is a colour space, never a coordinate space. "rgb" is
 * the space a colour is already in, so nothing is asked of the renderer and nothing is
 * reported. Any other colour space is converted as ctransform converts it.
 **/
BOOST_AUTO_TEST_CASE(sllibrary_colour_cast_space_test) {
    const Shaded rgb("color red = color \"rgb\" (1, 0, 0);");
    BOOST_CHECK_CLOSE(rgb.triple("red").r, 1.0f, 0.01f);
    BOOST_CHECK(rgb.machine().reports().empty());

    const Shaded hsv("color other = color \"hsv\" (0.25, 0.5, 0.75);");
    BOOST_CHECK_CLOSE(hsv.triple("other").g, 0.5f, 0.01f);
    BOOST_REQUIRE_EQUAL(hsv.machine().reports().size(), 1u);
    BOOST_CHECK_EQUAL(hsv.machine().reports()[0],
        "the colour space \"hsv\" is not one this renderer knows");
}

/**
 * calculatenormal needs the derivatives of the grid it is shading, which no renderer
 * supplies. It is a stub like the other three rather than returning a plausible value,
 * because a plausible value would make a scene render wrong silently.
 **/
BOOST_AUTO_TEST_CASE(sllibrary_calculatenormal_is_a_stub_test) {
    const Shaded shaded("normal answer = calculatenormal(P);", 4);
    BOOST_CHECK_SMALL(shaded.triple("answer").z, 0.0001f);
    BOOST_REQUIRE_EQUAL(shaded.machine().reports().size(), 1u);
    BOOST_CHECK_EQUAL(shaded.machine().reports()[0],
        "'calculatenormal' is declared and does nothing yet, so it answers its default");
}

/**
 * A function is declared with what runs it, so the table and the machine always agree: the
 * functions written in the language are exactly those declared as source, and every other
 * body is one the machine runs or a stub it reports.
 **/
BOOST_AUTO_TEST_CASE(sllibrary_bodies_test) {
    std::set<std::string> written;
    for (const v3d::render::offline::sl::syntax::Function & function : v3d::render::offline::sl::sources()) {
        written.insert(function.name);
    }
    std::set<std::string> declared;
    std::set<std::string> stubs;
    for (const v3d::render::offline::sl::Signature & signature : v3d::render::offline::sl::builtins()) {
        if (signature.body == v3d::render::offline::sl::Signature::Body::SOURCE) {
            declared.insert(signature.name);
        } else if (signature.body == v3d::render::offline::sl::Signature::Body::STUB) {
            stubs.insert(signature.name);
        }
    }
    BOOST_CHECK(declared == written);
    BOOST_CHECK(stubs == std::set<std::string>({ "calculatenormal", "shadow" }));
}
