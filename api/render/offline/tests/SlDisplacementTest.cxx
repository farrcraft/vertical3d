/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/offline/sl/Compiler.h>
#include <api/render/offline/sl/Emitter.h>
#include <api/render/offline/sl/Globals.h>
#include <api/render/offline/sl/Parser.h>
#include <api/render/offline/sl/runtime/Machine.h>
#include <api/render/offline/sl/runtime/Program.h>
#include <api/render/offline/sl/syntax/Shader.h>

#include <sstream>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/vec3.hpp>

namespace {

/**
 * The error a displacement shader's source compiles with, or empty when it compiles.
 **/
std::string compiled(const std::string & source, v3d::render::offline::sl::runtime::Program* program) {
    std::istringstream stream(source);
    v3d::render::offline::sl::Parser parser(stream);
    std::vector<v3d::render::offline::sl::syntax::ShaderPtr> shaders = parser.parse();
    if (shaders.size() != 1) {
        return parser.error();
    }
    v3d::render::offline::sl::Compiler compiler(shaders[0]);
    if (!compiler.compile()) {
        return compiler.error();
    }
    v3d::render::offline::sl::Emitter emitter(shaders[0], compiler.symbols());
    return emitter.emit(program) ? "" : emitter.error();
}

/**
 * A displacement shader compiles, and P and N are written in it. Over a batch, it moves each
 * point along its normal by the amount it states.
 **/
BOOST_AUTO_TEST_CASE(sl_a_displacement_moves_p_along_n_test) {
    v3d::render::offline::sl::runtime::Program program;
    BOOST_REQUIRE_EQUAL(compiled(
        "displacement lift() {\n"
        "    P = P + 0.25 * normalize(N);\n"
        "    N = calculatenormal(P);\n"
        "}\n", &program), "");

    v3d::render::offline::sl::runtime::Machine machine;
    machine.prepare(program, 2);
    const v3d::render::offline::sl::Globals globals(program);
    const glm::vec3 normals[2] = { glm::vec3(0.0F, 0.0F, -2.0F), glm::vec3(0.0F, 3.0F, 0.0F) };
    for (unsigned int lane = 0; lane < 2; lane++) {
        v3d::render::offline::sl::Point point;
        point.position = glm::vec3(static_cast<float>(lane), 1.0F, 5.0F);
        point.normal = normals[lane];
        globals.surface(&machine, lane, point);
    }
    BOOST_REQUIRE_MESSAGE(machine.run(), machine.error());

    const int moved = program.symbol("P");
    BOOST_REQUIRE_GE(moved, 0);
    BOOST_CHECK_CLOSE(machine.value(moved).triple(0).z, 4.75F, 0.01F);
    BOOST_CHECK_CLOSE(machine.value(moved).triple(1).y, 1.25F, 0.01F);
    BOOST_CHECK_CLOSE(machine.value(moved).triple(1).x, 1.0F, 0.01F);
}

/**
 * A displacement shader writes P and N and nothing a surface shader writes. Ci is a surface's
 * and an imager's, so naming it in a displacement shader says whose it is.
 **/
BOOST_AUTO_TEST_CASE(sl_a_displacement_cannot_write_ci_test) {
    v3d::render::offline::sl::runtime::Program program;
    BOOST_CHECK_NE(compiled("displacement wrong() { Ci = 1; }\n", &program), "");
    BOOST_CHECK_EQUAL(compiled("displacement right() { N = N; P = P; }\n", &program), "");
}

};  // namespace
