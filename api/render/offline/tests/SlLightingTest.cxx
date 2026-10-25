/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/log/Logger.h>
#include <api/render/offline/sl/Compiler.h>
#include <api/render/offline/sl/Emitter.h>
#include <api/render/offline/sl/Parser.h>
#include <api/render/offline/sl/Types.h>
#include <api/render/offline/sl/runtime/Instruction.h>
#include <api/render/offline/sl/runtime/Machine.h>
#include <api/render/offline/sl/runtime/Program.h>
#include <api/render/offline/sl/runtime/Renderer.h>
#include <api/render/offline/sl/runtime/Value.h>
#include <api/render/offline/sl/syntax/Shader.h>

#include <cmath>
#include <cstdio>
#include <fstream>
#include <iterator>
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
    std::vector<v3d::render::offline::sl::syntax::ShaderPtr> shaders = parser.parse();
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
 * A light with neither `illuminate` nor `solar` in it is an ambient one, so it is left out
 * of an illuminance loop and summed by `ambient()`. The program records this, so a renderer
 * reads it there rather than from the source.
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

    bool shine(const Value & surface, const v3d::render::offline::sl::runtime::Cone & gathering, Value* direction,
        Value* colour, std::vector<char>* reached, bool* ambient) {
        std::vector<glm::vec3> axes;
        std::vector<float> angles;
        if (gathering.axis != nullptr && gathering.angle != nullptr) {
            for (unsigned int point = 0; point < machine_.batch(); point++) {
                axes.push_back(gathering.axis->triple(point));
                angles.push_back(gathering.angle->number(point));
            }
        }
        machine_.gather(axes, angles);
        const int where = program_.symbol("Ps");
        if (where >= 0) {
            for (unsigned int point = 0; point < machine_.batch(); point++) {
                machine_.value(where).triple(point, surface.triple(point));
            }
        }
        if (!machine_.run()) {
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
 * The Renderer an illuminance loop calls to run a light. It holds the light shader instances
 * a scene named.
 **/
class Scene final : public v3d::render::offline::sl::runtime::Renderer {
 public:
    void add(const std::string & source, unsigned int batch) {
        lamps_.push_back(boost::make_shared<Lamp>(source, batch));
    }

    bool space(const std::string & /* name */, glm::mat4x4* /* matrix */) override {
        return false;
    }

    unsigned int lights() override {
        return static_cast<unsigned int>(lamps_.size());
    }

    bool light(unsigned int index, const Value & surface, const v3d::render::offline::sl::runtime::Cone & gathering,
        Value* direction, Value* colour, std::vector<char>* reached, bool* ambient) override {
        return lamps_[index]->shine(surface, gathering, direction, colour, reached, ambient);
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
        BOOST_REQUIRE_MESSAGE(machine_.run(), machine_.error());
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

/**
 * A light shader's `solar` sets L against the direction the light travels, so that L points
 * from the surface toward the light in the illuminance body that reads it. Every point of
 * the batch is lit, because a light at infinity reaches everything from one direction.
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

    BOOST_REQUIRE(scene.light(0, surface, v3d::render::offline::sl::runtime::Cone(), &direction, &colour, &reached, &ambient));
    BOOST_CHECK(!ambient);
    BOOST_CHECK_CLOSE(direction.triple(2).z, 1.0F, 0.01F);
    BOOST_CHECK_CLOSE(colour.triple(2).r, 1.0F, 0.01F);
    BOOST_REQUIRE_EQUAL(reached.size(), 3U);
    BOOST_CHECK(reached[0] != 0 && reached[2] != 0);
}

/**
 * The L a solar light with an angle lights a point from, given the cone the point's surface
 * gathers from. Its axis travels along -z, so L is around +z, half a radian either way.
 **/
glm::vec3 solarFrom(const std::vector<glm::vec3> & gathering) {
    std::string error;
    Program program;
    BOOST_REQUIRE_MESSAGE(build(
        "light wide() {\n"
        "    solar(vector (0, 0, -1), 0.5) {\n"
        "        Cl = color (1, 1, 1);\n"
        "    }\n"
        "}\n", &program, &error), error);
    Machine machine;
    machine.prepare(program, 1);
    machine.gather(gathering, std::vector<float>(gathering.size(), 1.5707963F));
    BOOST_REQUIRE(machine.run());
    BOOST_CHECK(machine.reports().empty());
    return machine.value(program.symbol("L")).triple(0);
}

/**
 * A solar light with an angle chooses L inside its cone, nearest the surface's illuminance
 * axis, as RenderMan does. An axis inside the cone is L itself. An axis outside it gives the
 * cone's edge nearest it. With no cone to choose against, L is the light's axis.
 **/
BOOST_AUTO_TEST_CASE(sllighting_solar_with_an_angle_chooses_its_direction_test) {
    const glm::vec3 inside = solarFrom({ glm::vec3(0.2F, 0.0F, 1.0F) });
    const glm::vec3 axis = glm::normalize(glm::vec3(0.2F, 0.0F, 1.0F));
    BOOST_CHECK_SMALL(glm::length(inside - axis), 1.0e-5F);

    const glm::vec3 edge = solarFrom({ glm::vec3(1.0F, 0.0F, 0.0F) });
    BOOST_CHECK_CLOSE(edge.x, std::sin(0.5F), 0.01F);
    BOOST_CHECK_SMALL(edge.y, 1.0e-6F);
    BOOST_CHECK_CLOSE(edge.z, std::cos(0.5F), 0.01F);

    const glm::vec3 none = solarFrom({});
    BOOST_CHECK_CLOSE(none.z, 1.0F, 0.01F);
    BOOST_CHECK_SMALL(none.x, 1.0e-6F);
}

/**
 * An illuminance loop hands its cone to each light it runs. A surface gathering from a cone
 * around +x, under the solar light half a radian around +z, is lit along the light's cone edge
 * nearest +x. The loop's body reads that L.
 **/
BOOST_AUTO_TEST_CASE(sllighting_an_illuminance_cone_reaches_the_light_test) {
    Scene scene;
    scene.add(
        "light wide() {\n"
        "    solar(vector (0, 0, -1), 0.5) {\n"
        "        Cl = color (1, 1, 1);\n"
        "    }\n"
        "}\n", 1);
    Lit lit(
        "color sum = 0;\n"
        "illuminance(P, vector (1, 0, 0), 1.5707963) {\n"
        "    sum += color (xcomp(L), ycomp(L), zcomp(L));\n"
        "}\n"
        "Ci = sum;", &scene, 1);
    lit.run();
    BOOST_CHECK_CLOSE(lit.colour(0).r, std::sin(0.5F), 0.01F);
    BOOST_CHECK_CLOSE(lit.colour(0).b, std::cos(0.5F), 0.01F);
}

/**
 * A machine given a logger writes each report to it once, as a warning, so a report reaches
 * someone reading the log rather than only a caller that asks for reports().
 **/
BOOST_AUTO_TEST_CASE(sllighting_a_report_reaches_the_log_test) {
    const std::string path = "sl_reports_test.log";
    std::remove(path.c_str());
    // the log is pointed back at its default however the case ends, so a failed check here
    // does not leave every later case writing to this file
    struct Restore final {
        ~Restore() {
            // a destructor may not throw, and a log that cannot be reopened only costs the
            // later cases their log lines
            try {
                v3d::log::Logger::open("v3d.log");
            } catch (...) {
                return;
            }
        }
    };
    const Restore restore{};
    BOOST_REQUIRE(v3d::log::Logger::open(path));
    {
        std::string error;
        Program program;
        // shadow() is a stub, which reports itself whenever it is called
        BOOST_REQUIRE_MESSAGE(build(
            "light shaded() {\n"
            "    float blocked = shadow(\"map\", Ps);\n"
            "    Cl = color (1, 1, 1) * blocked;\n"
            "}\n", &program, &error), error);
        Machine machine;
        machine.logger(boost::make_shared<v3d::log::Logger>());
        machine.prepare(program, 2);
        BOOST_REQUIRE(machine.run());
        BOOST_REQUIRE(machine.run());
        v3d::log::Logger().get()->flush();
    }
    // back to the default, which lets go of the test's file before it is read and removed
    v3d::log::Logger::open("v3d.log");

    std::ifstream file(path);
    const std::string contents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    const std::string line = "'shadow' is declared and does nothing yet";
    const std::size_t first = contents.find(line);
    BOOST_CHECK(first != std::string::npos);
    BOOST_CHECK(contents.find(line, first + 1) == std::string::npos);
    std::remove(path.c_str());
}

/**
 * A light with a position aims a cone, and the points outside it are not lit at all. The light
 * shader writes L for every point the cone reaches. The surface's illuminance body never runs
 * for the points the cone misses.
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
    surface.triple(0, glm::vec3(0.0F, 0.0F, 0.0F));
    surface.triple(1, glm::vec3(5.0F, 0.0F, 0.0F));
    Value direction;
    direction.reset(v3d::render::offline::sl::Type::VECTOR,
        v3d::render::offline::sl::Storage::VARYING, 2);
    Value colour;
    colour.reset(v3d::render::offline::sl::Type::COLOR,
        v3d::render::offline::sl::Storage::VARYING, 2);
    std::vector<char> reached;
    bool ambient = true;

    BOOST_REQUIRE(scene.light(0, surface, v3d::render::offline::sl::runtime::Cone(), &direction, &colour, &reached, &ambient));
    BOOST_CHECK(!ambient);
    // L points at the light, two units up from the point below it
    BOOST_CHECK_CLOSE(direction.triple(0).z, 2.0F, 0.01F);
    BOOST_REQUIRE_EQUAL(reached.size(), 2U);
    BOOST_CHECK(reached[0] != 0);
    BOOST_CHECK(reached[1] == 0);
    // the point outside the cone never had Cl set, because the body did not run for it
    BOOST_CHECK_SMALL(colour.triple(1).r, 0.0001F);
}

/**
 * `diffuse` over one distant light is the cosine of the angle between the surface and the
 * light, and it is one line of the language rather than a built-in with privileged access
 * to the lights.
 **/
BOOST_AUTO_TEST_CASE(sllighting_diffuse_is_the_cosine_test) {
    Scene scene;
    scene.add(OVERHEAD, 3);

    Lit lit("Ci = diffuse(N);", &scene, 3);
    lit.normal(0, glm::vec3(0.0F, 0.0F, 1.0F));
    // 60 degrees off the light, whose cosine is a half
    lit.normal(1, glm::vec3(0.8660254F, 0.0F, 0.5F));
    // facing away, so the illuminance cone of PI/2 leaves the light out of the sum entirely
    lit.normal(2, glm::vec3(0.0F, 0.0F, -1.0F));
    lit.run();

    BOOST_CHECK_CLOSE(lit.colour(0).r, 1.0F, 0.1F);
    BOOST_CHECK_CLOSE(lit.colour(1).g, 0.5F, 0.1F);
    BOOST_CHECK_SMALL(lit.colour(2).b, 0.0001F);
}

/**
 * A two light scene runs the body twice, with that light's own L and Cl each time. Two
 * colours that do not overlap show which light each component came from, so a body that ran
 * once with the last light's values fails.
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
    lit.normal(0, glm::vec3(0.70710678F, 0.0F, 0.70710678F));
    lit.run();

    // the white light overhead, at 45 degrees
    BOOST_CHECK_CLOSE(lit.colour(0).r, 0.70710678F, 0.1F);
    // the white one plus the green one, each at 45 degrees
    BOOST_CHECK_CLOSE(lit.colour(0).g, 0.70710678F + (0.25F * 0.70710678F), 0.1F);
}

/**
 * An illuminance cone leaves a light out of the sum, so a surface is not lit from behind.
 * The same scene with no cone sums both.
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
    facing.normal(0, glm::vec3(0.0F, 0.0F, 1.0F));
    facing.run();
    // only the one overhead is in front of the surface
    BOOST_CHECK_CLOSE(facing.colour(0).r, 1.0F, 0.1F);

    Lit both("color sum = 0;\nilluminance(P) { sum += Cl; }\nCi = sum;", &scene, 1);
    both.normal(0, glm::vec3(0.0F, 0.0F, 1.0F));
    both.run();
    BOOST_CHECK_CLOSE(both.colour(0).r, 1.5F, 0.1F);
}

/**
 * `ambient()` sums the lights an illuminance loop skips. A light with neither `illuminate`
 * nor `solar` is ambient: it has no direction to test against a cone, so it needs its own
 * built-in.
 **/
BOOST_AUTO_TEST_CASE(sllighting_ambient_test) {
    Scene scene;
    scene.add(OVERHEAD, 1);
    scene.add(
        "light fill() {\n"
        "    Cl = color (0.2, 0.2, 0.2);\n"
        "}\n", 1);

    Lit lit("Ci = ambient();", &scene, 1);
    lit.normal(0, glm::vec3(0.0F, 0.0F, 1.0F));
    lit.run();
    // the distant light overhead is not in it, however brightly it shines
    BOOST_CHECK_CLOSE(lit.colour(0).r, 0.2F, 0.1F);

    Lit summed("color sum = 0;\nilluminance(P) { sum += Cl; }\nCi = sum;", &scene, 1);
    summed.normal(0, glm::vec3(0.0F, 0.0F, 1.0F));
    summed.run();
    // the ambient one is not in the illuminance loop
    BOOST_CHECK_CLOSE(summed.colour(0).r, 1.0F, 0.1F);
}

/**
 * A renderer that cannot compute a shadow lets all the light through, and the machine
 * reports it once. The report tells a scene that rendered without shadows apart from one
 * that was not understood.
 **/
BOOST_AUTO_TEST_CASE(sllighting_transmission_without_a_renderer_test) {
    Scene scene;
    Lit lit("Ci = transmission(P, point (0, 0, 10)) * color (1, 1, 1);", &scene, 4);
    lit.run();

    BOOST_CHECK_CLOSE(lit.colour(0).r, 1.0F, 0.1F);
    BOOST_REQUIRE_EQUAL(lit.machine().reports().size(), 1U);
    BOOST_CHECK(lit.machine().reports()[0].ends_with(
        "'transmission' has nothing to cast a shadow here, so all the light gets through"));
}

/**
 * When a renderer cannot trace, the machine reports it. A ray returns black rather than something
 * plausible, because a plausible value would hide the failure.
 **/
BOOST_AUTO_TEST_CASE(sllighting_trace_without_a_renderer_test) {
    Scene scene;
    Lit lit("Ci = trace(P, vector (0, 0, 1));", &scene, 4);
    lit.run();

    BOOST_CHECK_SMALL(lit.colour(0).r, 0.0001F);
    BOOST_REQUIRE_EQUAL(lit.machine().reports().size(), 1U);
    BOOST_CHECK(lit.machine().reports()[0].ends_with(
        "'trace' is not answered by this renderer, so a ray comes back black"));
}

/**
 * A program records whether it calls trace or transmission, so a renderer knows whether it
 * needs its traced scene. A call inside the shader's own function counts, because the
 * function is inlined into the shader.
 **/
BOOST_AUTO_TEST_CASE(sllighting_a_program_records_that_it_traces_test) {
    Program program;
    std::string error;

    BOOST_REQUIRE_MESSAGE(build(
        "surface s() {\n"
        "    color shadowed(point from) {\n"
        "        return transmission(from, point (0, 0, 10));\n"
        "    }\n"
        "    Ci = shadowed(P);\n"
        "}\n", &program, &error), error);
    BOOST_CHECK(program.traces);

    BOOST_REQUIRE_MESSAGE(build(
        "surface s() {\n"
        "    color reflected(point from) {\n"
        "        return trace(from, vector (0, 0, 1));\n"
        "    }\n"
        "    Ci = reflected(P);\n"
        "}\n", &program, &error), error);
    BOOST_CHECK(program.traces);

    // a function that traces nothing, and a library function, leave the mark unset on a
    // program that was marked before
    BOOST_REQUIRE_MESSAGE(build(
        "surface s() {\n"
        "    color lit(normal toward) {\n"
        "        return diffuse(toward);\n"
        "    }\n"
        "    Ci = lit(normalize(N)) + ambient();\n"
        "}\n", &program, &error), error);
    BOOST_CHECK(!program.traces);
}

/**
 * `specular` and `phong` are written in the language too, and each is added to the shader
 * that calls it rather than being a built-in. specular calls specularbrdf, which shows that
 * adding them is transitive.
 **/
BOOST_AUTO_TEST_CASE(sllighting_specular_test) {
    Scene scene;
    scene.add(OVERHEAD, 1);

    // the surface faces the light and the viewer is where the light is, so the half angle
    // vector is the normal and the highlight is at its brightest whatever the roughness
    Lit lit("Ci = specular(N, vector (0, 0, 1), 0.1);", &scene, 1);
    lit.normal(0, glm::vec3(0.0F, 0.0F, 1.0F));
    lit.run();
    BOOST_CHECK_CLOSE(lit.colour(0).r, 1.0F, 0.1F);

    Lit dulled("Ci = phong(N, vector (0, 0, 1), 4);", &scene, 1);
    dulled.normal(0, glm::vec3(0.0F, 0.0F, 1.0F));
    dulled.run();
    BOOST_CHECK_CLOSE(dulled.colour(0).r, 1.0F, 0.1F);
}

/**
 * A lane that returns from inside an illuminance loop has finished, and so has one that
 * breaks out of it. Neither runs the body again for the next light.
 **/
BOOST_AUTO_TEST_CASE(sllighting_return_and_break_leave_illuminance_test) {
    Scene scene;
    scene.add(OVERHEAD, 1);
    scene.add(
        "light sideways() {\n"
        "    solar(vector (-1, 0, 0), 0) {\n"
        "        Cl = color (0, 0.25, 0);\n"
        "    }\n"
        "}\n", 1);

    Lit returned(
        "color first() {\n"
        "    illuminance(P) {\n"
        "        return Cl;\n"
        "    }\n"
        "    return color (9, 9, 9);\n"
        "}\n"
        "Ci = first();", &scene, 1);
    returned.normal(0, glm::vec3(0.0F, 0.0F, 1.0F));
    returned.run();
    BOOST_CHECK_CLOSE(returned.colour(0).r, 1.0F, 0.1F);
    BOOST_CHECK_CLOSE(returned.colour(0).g, 1.0F, 0.1F);

    Lit broken("color sum = 0;\nilluminance(P) { sum += Cl; break; }\nCi = sum;", &scene, 1);
    broken.normal(0, glm::vec3(0.0F, 0.0F, 1.0F));
    broken.run();
    BOOST_CHECK_CLOSE(broken.colour(0).r, 1.0F, 0.1F);
    BOOST_CHECK_CLOSE(broken.colour(0).g, 1.0F, 0.1F);
}

/**
 * Lanes of one batch leave an illuminance loop at different lights. A continue skips one light
 * for the lanes that take it, and a break ends the loop for those lanes alone. The other lanes
 * of the batch go on through every light.
 **/
BOOST_AUTO_TEST_CASE(sllighting_lanes_leave_illuminance_at_different_lights_test) {
    Scene scene;
    scene.add(OVERHEAD, 2);
    scene.add(
        "light sideways() {\n"
        "    solar(vector (-1, 0, 0), 0) {\n"
        "        Cl = color (0, 0.25, 0);\n"
        "    }\n"
        "}\n", 2);

    Lit skipped("color sum = 0;\nilluminance(P) { if (xcomp(P) > 0) { continue; } sum += Cl; }\nCi = sum;", &scene, 2);
    Lit broken("color sum = 0;\nilluminance(P) { sum += Cl; if (xcomp(P) > 0) { break; } }\nCi = sum;", &scene, 2);
    for (Lit* lit : { &skipped, &broken }) {
        lit->position(0, glm::vec3(1.0F, 0.0F, 0.0F));
        lit->position(1, glm::vec3(-1.0F, 0.0F, 0.0F));
        lit->normal(0, glm::vec3(0.0F, 0.0F, 1.0F));
        lit->normal(1, glm::vec3(0.0F, 0.0F, 1.0F));
        lit->run();
    }

    // the first lane continues past every light, and the second sums both
    BOOST_CHECK_SMALL(skipped.colour(0).g, 0.0001F);
    BOOST_CHECK_CLOSE(skipped.colour(1).g, 1.25F, 0.1F);
    // the first lane breaks after the first light, and the second sums both
    BOOST_CHECK_CLOSE(broken.colour(0).g, 1.0F, 0.1F);
    BOOST_CHECK_CLOSE(broken.colour(1).g, 1.25F, 0.1F);
}

};  // namespace
