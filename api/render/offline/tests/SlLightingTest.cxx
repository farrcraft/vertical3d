/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/offline/sl/Compiler.h>
#include <api/render/offline/sl/Emitter.h>
#include <api/render/offline/sl/Parser.h>
#include <api/render/offline/sl/runtime/Machine.h>
#include <api/render/offline/sl/runtime/Renderer.h>

#include <sstream>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace {

typedef v3d::render::offline::sl::runtime::Machine Machine;
typedef v3d::render::offline::sl::runtime::Program Program;
typedef v3d::render::offline::sl::runtime::Value Value;
typedef v3d::render::offline::sl::runtime::Opcode Opcode;

bool build(const std::string & source, Program* program, std::string* error) {
    std::istringstream stream(source);
    v3d::render::offline::sl::Parser parser(stream);
    std::vector<v3d::render::offline::sl::ShaderPtr> shaders = parser.parse();
    if (shaders.size() != 1) {
        *error = parser.error();
        return false;
    }
    v3d::render::offline::sl::Compiler compiler(shaders[0]);
    if (!compiler.compile()) {
        *error = compiler.error();
        return false;
    }
    v3d::render::offline::sl::Emitter emitter(shaders[0], compiler.symbols());
    if (!emitter.emit(program)) {
        *error = emitter.error();
        return false;
    }
    return true;
}

/**
 * One light shader, compiled and ready to run over a batch.
 *
 * A light with neither `illuminate` nor `solar` in it is an ambient one, which is what
 * keeps it out of an illuminance loop and inside `ambient()`. The program says so, which
 * is what the renderers will read in step 9 rather than asking the source again.
 **/
class Lamp final {
 public:
    Lamp(const std::string & source, unsigned int batch) {
        std::string error;
        BOOST_REQUIRE_MESSAGE(build(source, &program_, &error), error);
        machine_.prepare(program_, batch);
        for (const v3d::render::offline::sl::runtime::Instruction & instruction : program_.instructions) {
            if (instruction.opcode == Opcode::ILLUMINATE || instruction.opcode == Opcode::SOLAR) {
                directional_ = true;
            }
        }
    }

    bool shine(const Value & surface, Value* direction, Value* colour,
        std::vector<char>* reached, bool* ambient) {
        const int where = program_.symbol("Ps");
        if (where >= 0) {
            for (unsigned int point = 0; point < machine_.batch(); point++) {
                machine_.value(where).triple(point, surface.triple(point));
            }
        }
        if (!machine_.run(program_)) {
            return false;
        }
        const int away = program_.symbol("L");
        const int tint = program_.symbol("Cl");
        for (unsigned int point = 0; point < machine_.batch(); point++) {
            direction->triple(point, machine_.value(away).triple(point));
            colour->triple(point, machine_.value(tint).triple(point));
        }
        *reached = machine_.lit();
        *ambient = !directional_;
        return true;
    }

 private:
    Program program_;
    Machine machine_;
    bool directional_ = false;
};

/**
 * The renderer's half of the message passing: it holds the light shader instances a scene
 * named, and running one is what an illuminance loop asks it for.
 **/
class Scene final : public v3d::render::offline::sl::runtime::Renderer {
 public:
    void add(const std::string & source, unsigned int batch) {
        lamps_.push_back(boost::make_shared<Lamp>(source, batch));
    }

    bool space(const std::string &, glm::mat4x4*) override {
        return false;
    }

    unsigned int lights() override {
        return static_cast<unsigned int>(lamps_.size());
    }

    bool light(unsigned int index, const Value & surface, Value* direction, Value* colour,
        std::vector<char>* reached, bool* ambient) override {
        return lamps_[index]->shine(surface, direction, colour, reached, ambient);
    }

 private:
    std::vector<boost::shared_ptr<Lamp> > lamps_;
};

/**
 * A surface shader run over a batch under a scene's lights, with N and P written first
 * because a light model is a function of both.
 **/
class Lit final {
 public:
    Lit(const std::string & body, Scene* scene, unsigned int batch) {
        std::string error;
        BOOST_REQUIRE_MESSAGE(build("surface test() {\n" + body + "\n}\n", &program_, &error), error);
        machine_.renderer(scene);
        machine_.prepare(program_, batch);
    }

    void normal(unsigned int point, const glm::vec3 & value) {
        machine_.value(program_.symbol("N")).triple(point, value);
    }

    void position(unsigned int point, const glm::vec3 & value) {
        machine_.value(program_.symbol("P")).triple(point, value);
    }

    void run() {
        BOOST_REQUIRE_MESSAGE(machine_.run(program_), machine_.error());
    }

    glm::vec3 colour(unsigned int point) const {
        return machine_.value(program_.symbol("Ci")).triple(point);
    }

    const Machine & machine() const {
        return machine_;
    }

 private:
    Program program_;
    Machine machine_;
};

/** A distant light straight down the negative z axis, one unit of white. **/
const char* const OVERHEAD = R"(
light overhead() {
    solar(vector (0, 0, -1), 0) {
        Cl = color (1, 1, 1);
    }
}
)";

};  // namespace

/**
 * A light shader's `solar` sets L against the direction the light travels, so that L points
 * from the surface toward the light in the illuminance body that reads it. Every point of
 * the batch is lit, which is what makes a light at infinity distant.
 **/
BOOST_AUTO_TEST_CASE(sllighting_solar_test) {
    Scene scene;
    scene.add(OVERHEAD, 3);

    Value surface;
    surface.reset(v3d::render::offline::sl::Type::POINT,
        v3d::render::offline::sl::Storage::VARYING, 3);
    Value direction;
    direction.reset(v3d::render::offline::sl::Type::VECTOR,
        v3d::render::offline::sl::Storage::VARYING, 3);
    Value colour;
    colour.reset(v3d::render::offline::sl::Type::COLOR,
        v3d::render::offline::sl::Storage::VARYING, 3);
    std::vector<char> reached;
    bool ambient = true;

    BOOST_REQUIRE(scene.light(0, surface, &direction, &colour, &reached, &ambient));
    BOOST_CHECK(!ambient);
    BOOST_CHECK_CLOSE(direction.triple(2).z, 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(colour.triple(2).r, 1.0f, 0.01f);
    BOOST_REQUIRE_EQUAL(reached.size(), 3u);
    BOOST_CHECK(reached[0] != 0 && reached[2] != 0);
}

/**
 * A light with a position aims a cone, and the points outside it are not lit at all. That
 * is the light's own end of the mask: L is written for every point it reaches and the ones
 * it misses come back as points the surface never runs the body for.
 **/
BOOST_AUTO_TEST_CASE(sllighting_illuminate_cone_test) {
    Scene scene;
    scene.add(
        "light spot() {\n"
        // a light two units up, aimed down, with a narrow cone
        "    illuminate(point (0, 0, 2), vector (0, 0, -1), 0.4) {\n"
        "        Cl = color (1, 0, 0);\n"
        "    }\n"
        "}\n", 2);

    Value surface;
    surface.reset(v3d::render::offline::sl::Type::POINT,
        v3d::render::offline::sl::Storage::VARYING, 2);
    // one point under the light and one well off to the side of the cone
    surface.triple(0, glm::vec3(0.0f, 0.0f, 0.0f));
    surface.triple(1, glm::vec3(5.0f, 0.0f, 0.0f));
    Value direction;
    direction.reset(v3d::render::offline::sl::Type::VECTOR,
        v3d::render::offline::sl::Storage::VARYING, 2);
    Value colour;
    colour.reset(v3d::render::offline::sl::Type::COLOR,
        v3d::render::offline::sl::Storage::VARYING, 2);
    std::vector<char> reached;
    bool ambient = true;

    BOOST_REQUIRE(scene.light(0, surface, &direction, &colour, &reached, &ambient));
    BOOST_CHECK(!ambient);
    // L points at the light, two units up from the point below it
    BOOST_CHECK_CLOSE(direction.triple(0).z, 2.0f, 0.01f);
    BOOST_REQUIRE_EQUAL(reached.size(), 2u);
    BOOST_CHECK(reached[0] != 0);
    BOOST_CHECK(reached[1] == 0);
    // the point outside the cone never had Cl set, because the body did not run for it
    BOOST_CHECK_SMALL(colour.triple(1).r, 0.0001f);
}

/**
 * The done-when of the step: `diffuse` over one distant light is the cosine of the angle
 * between the surface and the light, and it is one line of the language rather than a
 * built-in with privileged access to the lights.
 **/
BOOST_AUTO_TEST_CASE(sllighting_diffuse_is_the_cosine_test) {
    Scene scene;
    scene.add(OVERHEAD, 3);

    Lit lit("Ci = diffuse(N);", &scene, 3);
    lit.normal(0, glm::vec3(0.0f, 0.0f, 1.0f));
    // 60 degrees off the light, whose cosine is a half
    lit.normal(1, glm::vec3(0.8660254f, 0.0f, 0.5f));
    // facing away, which the illuminance cone of PI/2 keeps out of the sum entirely
    lit.normal(2, glm::vec3(0.0f, 0.0f, -1.0f));
    lit.run();

    BOOST_CHECK_CLOSE(lit.colour(0).r, 1.0f, 0.1f);
    BOOST_CHECK_CLOSE(lit.colour(1).g, 0.5f, 0.1f);
    BOOST_CHECK_SMALL(lit.colour(2).b, 0.0001f);
}

/**
 * The other done-when: a two light scene runs the body twice, with that light's own L and
 * Cl each time. Two colours that do not overlap say which light each component came from,
 * so a body that ran once with the last light's values would fail rather than pass by
 * halves.
 **/
BOOST_AUTO_TEST_CASE(sllighting_two_lights_test) {
    Scene scene;
    scene.add(OVERHEAD, 1);
    scene.add(
        "light sideways() {\n"
        "    solar(vector (-1, 0, 0), 0) {\n"
        "        Cl = color (0, 0.25, 0);\n"
        "    }\n"
        "}\n", 1);

    Lit lit(
        "color sum = 0;\n"
        "illuminance(P) {\n"
        "    sum += Cl * (normalize(L) . N);\n"
        "}\n"
        "Ci = sum;", &scene, 1);
    // halfway between the two lights, so each contributes its own cosine
    lit.normal(0, glm::vec3(0.70710678f, 0.0f, 0.70710678f));
    lit.run();

    // the white light overhead, at 45 degrees
    BOOST_CHECK_CLOSE(lit.colour(0).r, 0.70710678f, 0.1f);
    // the white one plus the green one, each at 45 degrees
    BOOST_CHECK_CLOSE(lit.colour(0).g, 0.70710678f + 0.25f * 0.70710678f, 0.1f);
}

/**
 * An illuminance cone keeps a light out of the sum, which is what stops a surface being
 * lit from behind. The same scene with no cone sums both.
 **/
BOOST_AUTO_TEST_CASE(sllighting_illuminance_cone_test) {
    Scene scene;
    scene.add(OVERHEAD, 1);
    scene.add(
        "light underneath() {\n"
        "    solar(vector (0, 0, 1), 0) {\n"
        "        Cl = color (0.5, 0.5, 0.5);\n"
        "    }\n"
        "}\n", 1);

    Lit facing("color sum = 0;\nilluminance(P, N, 1.5707963) { sum += Cl; }\nCi = sum;", &scene, 1);
    facing.normal(0, glm::vec3(0.0f, 0.0f, 1.0f));
    facing.run();
    // only the one overhead is in front of the surface
    BOOST_CHECK_CLOSE(facing.colour(0).r, 1.0f, 0.1f);

    Lit both("color sum = 0;\nilluminance(P) { sum += Cl; }\nCi = sum;", &scene, 1);
    both.normal(0, glm::vec3(0.0f, 0.0f, 1.0f));
    both.run();
    BOOST_CHECK_CLOSE(both.colour(0).r, 1.5f, 0.1f);
}

/**
 * `ambient()` sums the lights an illuminance loop cannot see. A light with neither
 * `illuminate` nor `solar` has no direction to test against a cone, which is exactly what
 * makes it ambient and exactly why it needs its own built-in.
 **/
BOOST_AUTO_TEST_CASE(sllighting_ambient_test) {
    Scene scene;
    scene.add(OVERHEAD, 1);
    scene.add(
        "light fill() {\n"
        "    Cl = color (0.2, 0.2, 0.2);\n"
        "}\n", 1);

    Lit lit("Ci = ambient();", &scene, 1);
    lit.normal(0, glm::vec3(0.0f, 0.0f, 1.0f));
    lit.run();
    // the distant light overhead is not in it, however brightly it shines
    BOOST_CHECK_CLOSE(lit.colour(0).r, 0.2f, 0.1f);

    Lit summed("color sum = 0;\nilluminance(P) { sum += Cl; }\nCi = sum;", &scene, 1);
    summed.normal(0, glm::vec3(0.0f, 0.0f, 1.0f));
    summed.run();
    // and the ambient one is not in the illuminance loop
    BOOST_CHECK_CLOSE(summed.colour(0).r, 1.0f, 0.1f);
}

/**
 * A renderer that cannot answer a shadow lets all the light through and says so once. That
 * is moya until it has a shadow map, and the difference between a scene that rendered
 * without shadows and one that was not understood.
 **/
BOOST_AUTO_TEST_CASE(sllighting_transmission_without_a_renderer_test) {
    Scene scene;
    Lit lit("Ci = transmission(P, point (0, 0, 10)) * color (1, 1, 1);", &scene, 4);
    lit.run();

    BOOST_CHECK_CLOSE(lit.colour(0).r, 1.0f, 0.1f);
    BOOST_REQUIRE_EQUAL(lit.machine().reports().size(), 1u);
    BOOST_CHECK_EQUAL(lit.machine().reports()[0],
        "'transmission' has nothing to cast a shadow here, so all the light gets through");
}

/**
 * The phase 6 hook exists and reports that it is one. A ray comes back black rather than
 * coming back with something plausible, because a plausible answer is the failure mode
 * phase 1 named.
 **/
BOOST_AUTO_TEST_CASE(sllighting_trace_is_a_hook_test) {
    Scene scene;
    Lit lit("Ci = trace(P, vector (0, 0, 1));", &scene, 4);
    lit.run();

    BOOST_CHECK_SMALL(lit.colour(0).r, 0.0001f);
    BOOST_REQUIRE_EQUAL(lit.machine().reports().size(), 1u);
    BOOST_CHECK_EQUAL(lit.machine().reports()[0],
        "'trace' is not answered by this renderer, so a ray comes back black");
}

/**
 * `specular` and `phong` are the language too, and each is adopted into the shader that
 * calls it rather than being a built-in. specular reaches specularbrdf, which is what says
 * the adoption is transitive.
 **/
BOOST_AUTO_TEST_CASE(sllighting_specular_test) {
    Scene scene;
    scene.add(OVERHEAD, 1);

    // the surface faces the light and the viewer is where the light is, so the half angle
    // vector is the normal and the highlight is at its brightest whatever the roughness
    Lit lit("Ci = specular(N, vector (0, 0, 1), 0.1);", &scene, 1);
    lit.normal(0, glm::vec3(0.0f, 0.0f, 1.0f));
    lit.run();
    BOOST_CHECK_CLOSE(lit.colour(0).r, 1.0f, 0.1f);

    Lit dulled("Ci = phong(N, vector (0, 0, 1), 4);", &scene, 1);
    dulled.normal(0, glm::vec3(0.0f, 0.0f, 1.0f));
    dulled.run();
    BOOST_CHECK_CLOSE(dulled.colour(0).r, 1.0f, 0.1f);
}
