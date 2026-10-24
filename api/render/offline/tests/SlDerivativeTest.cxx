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
#include <api/render/offline/sl/runtime/Renderer.h>
#include <api/render/offline/sl/syntax/Shader.h>

#include <functional>
#include <sstream>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace {

typedef v3d::render::offline::sl::runtime::Renderer Renderer;

/**
 * A renderer that answers only with a shape, the way a hider's renderer would for its batch.
 **/
class Shaped final : public Renderer {
 public:
    explicit Shaped(const Shape & shape) : shape_(shape) {
    }

    bool space(const std::string & /* name */, glm::mat4x4* /* matrix */) override {
        return false;
    }

    Shape shape() const override {
        return shape_;
    }

 private:
    Shape shape_;
};

/**
 * A square grid of `size` points a side, spaced as a hider dices: u and v run over [0, 1], so
 * the step between neighbours is 1 / (size - 1).
 **/
Renderer::Shape grid(unsigned int size) {
    Renderer::Shape shape;
    shape.layout = Renderer::Shape::Layout::GRID;
    shape.along = size;
    shape.across = size;
    shape.du = 1.0F / static_cast<float>(size - 1);
    shape.dv = shape.du;
    return shape;
}

/**
 * A shader body run over one point for each lane, with P, s, t, u and v where `at` puts them,
 * and its symbols readable afterwards.
 **/
class Run final {
 public:
    Run(const std::string & body, const Renderer::Shape & shape,
        const std::function<v3d::render::offline::sl::Point(const glm::uvec2 & index)> & at) : renderer_(shape) {
        const std::string source = "surface test() {\n" + body + "\n}\n";
        std::istringstream stream(source);
        v3d::render::offline::sl::Parser parser(stream);
        std::vector<v3d::render::offline::sl::syntax::ShaderPtr> shaders = parser.parse();
        BOOST_REQUIRE_MESSAGE(shaders.size() == 1, parser.error() + " in: " + body);
        v3d::render::offline::sl::Compiler compiler(shaders[0]);
        BOOST_REQUIRE_MESSAGE(compiler.compile(), compiler.error() + " in: " + body);
        v3d::render::offline::sl::Emitter emitter(shaders[0], compiler.symbols());
        BOOST_REQUIRE_MESSAGE(emitter.emit(&program_), emitter.error() + " in: " + body);
        const unsigned int along = shape.layout == Renderer::Shape::Layout::STENCIL ? 3U : shape.along;
        const unsigned int across = shape.layout == Renderer::Shape::Layout::STENCIL ? 1U : shape.across;
        machine_.renderer(&renderer_);
        machine_.prepare(program_, along * across);
        const v3d::render::offline::sl::Globals globals(program_);
        for (unsigned int i = 0; i < along; i++) {
            for (unsigned int j = 0; j < across; j++) {
                globals.surface(&machine_, (i * across) + j, at(glm::uvec2(i, j)));
            }
        }
        BOOST_REQUIRE_MESSAGE(machine_.run(), machine_.error());
        BOOST_CHECK(machine_.reports().empty());
    }

    float number(const std::string & name, unsigned int point) const {
        const int reg = program_.symbol(name);
        BOOST_REQUIRE_MESSAGE(reg >= 0, "no symbol named " + name);
        return machine_.value(reg).number(point);
    }

    glm::vec3 triple(const std::string & name, unsigned int point) const {
        const int reg = program_.symbol(name);
        BOOST_REQUIRE_MESSAGE(reg >= 0, "no symbol named " + name);
        return machine_.value(reg).triple(point);
    }

 private:
    Shaped renderer_;
    v3d::render::offline::sl::runtime::Program program_;
    v3d::render::offline::sl::runtime::Machine machine_;
};

/** A point at a grid index of a grid of a given span, with u and v and s and t from it. **/
v3d::render::offline::sl::Point parameters(const glm::uvec2 & index, float span) {
    v3d::render::offline::sl::Point point;
    point.u = static_cast<float>(index.x) / span;
    point.v = static_cast<float>(index.y) / span;
    point.s = point.u;
    point.t = point.v;
    return point;
}

/** The edge of a tilted plane across the whole of u. **/
glm::vec3 acrossU() {
    return {2.0F, 0.5F, 1.0F};
}

/** The same plane's edge across the whole of v. **/
glm::vec3 acrossV() {
    return {-0.5F, 3.0F, 0.25F};
}

/**
 * On a grid over a plane, Du(P) and Dv(P) are the plane's edges across the whole of u and of
 * v, because P changes by an edge as u goes from 0 to 1. calculatenormal(P) is their cross
 * product, which is the plane's normal and points the way Du(P) ^ Dv(P) does.
 **/
BOOST_AUTO_TEST_CASE(sl_derivatives_over_a_plane_test) {
    const unsigned int size = 5;
    const float span = static_cast<float>(size - 1);
    const Run run("vector across = Du(P);\nvector down = Dv(P);\nnormal facing = calculatenormal(P);",
        grid(size), [span](const glm::uvec2 & index) {
            v3d::render::offline::sl::Point point = parameters(index, span);
            point.position = glm::vec3(1.0F, 2.0F, 5.0F) + (acrossU() * point.u) + (acrossV() * point.v);
            return point;
        });
    const glm::vec3 normal = glm::cross(acrossU(), acrossV());
    // a corner, a point inside, and the last point, which differences backwards both ways
    for (const unsigned int point : { 0U, 12U, 24U }) {
        BOOST_TEST_CONTEXT("point " << point) {
            BOOST_TEST(glm::length(run.triple("across", point) - acrossU()) < 1.0e-4F);
            BOOST_TEST(glm::length(run.triple("down", point) - acrossV()) < 1.0e-4F);
            BOOST_TEST(glm::length(run.triple("facing", point) - normal) < 1.0e-3F);
        }
    }
}

/**
 * On a curved patch the derivative is the difference to the next point. The last row and
 * column have no next point, so they difference to the previous one instead.
 *
 * The patch is z = x * x over x = 4u. At u index 1 of a grid of span 4 the forward difference
 * runs from x = 1 to x = 2, so Du(z) is (4 - 1) * 4 = 12. At u index 4 the backward difference
 * runs from x = 3 to x = 4, so Du(z) is (16 - 9) * 4 = 28.
 **/
BOOST_AUTO_TEST_CASE(sl_derivatives_on_the_last_row_difference_backwards_test) {
    const unsigned int size = 5;
    const float span = static_cast<float>(size - 1);
    const Run run("vector across = Du(P);\nvector down = Dv(P);", grid(size),
        [span](const glm::uvec2 & index) {
            v3d::render::offline::sl::Point point = parameters(index, span);
            const float x = static_cast<float>(index.x);
            point.position = glm::vec3(x, static_cast<float>(index.y), x * x);
            return point;
        });
    // point (1, 2) is batch point 1 * 5 + 2 and point (4, 2) is 4 * 5 + 2
    BOOST_CHECK_CLOSE(run.triple("across", 7).z, 12.0F, 0.01F);
    BOOST_CHECK_CLOSE(run.triple("across", 22).z, 28.0F, 0.01F);
    BOOST_CHECK_CLOSE(run.triple("across", 22).x, 4.0F, 0.01F);
    // along v the patch is flat in z, and on the last column it differences backwards too
    BOOST_CHECK_SMALL(run.triple("down", 24).z, 1.0e-4F);
    BOOST_CHECK_CLOSE(run.triple("down", 24).y, 4.0F, 0.01F);
}

/**
 * Deriv(f, u) is Du(f) / Du(u), because u does not change along v and that term is dropped.
 * area(P) is the area of the patch one step along u and one along v span: the length of the
 * cross product of Du(P) * du and Dv(P) * dv.
 **/
BOOST_AUTO_TEST_CASE(sl_deriv_and_area_test) {
    const unsigned int size = 5;
    const float span = static_cast<float>(size - 1);
    const Run run("float f = s * s;\nfloat rate = Deriv(f, u);\nfloat ratio = Du(f) / Du(u);\n"
        "float patch = area(P);\nfloat byv = Deriv(f, v);", grid(size),
        [span](const glm::uvec2 & index) {
            v3d::render::offline::sl::Point point = parameters(index, span);
            point.position = (acrossU() * point.u) + (acrossV() * point.v);
            return point;
        });
    for (const unsigned int point : { 0U, 7U, 23U }) {
        BOOST_TEST_CONTEXT("point " << point) {
            BOOST_CHECK_CLOSE(run.number("rate", point), run.number("ratio", point), 0.01F);
            // f does not change along v, so its derivative by v is zero
            BOOST_CHECK_SMALL(run.number("byv", point), 1.0e-4F);
        }
    }
    // at u index 1, Du(s * s) is ((2/4)^2 - (1/4)^2) * 4 = 0.75, and Du(u) is one
    BOOST_CHECK_CLOSE(run.number("rate", 7), 0.75F, 0.01F);
    const float expected = glm::length(glm::cross(acrossU(), acrossV())) / (span * span);
    BOOST_CHECK_CLOSE(run.number("patch", 7), expected, 0.01F);
}

/**
 * A uniform value is the same at every point, so its derivative is zero. A batch of lone
 * points has no neighbours, so every derivative there is zero too, and none is reported.
 **/
BOOST_AUTO_TEST_CASE(sl_derivatives_of_a_uniform_and_of_lone_points_test) {
    const unsigned int size = 3;
    const float span = static_cast<float>(size - 1);
    const auto placed = [span](const glm::uvec2 & index) {
        v3d::render::offline::sl::Point point = parameters(index, span);
        point.position = glm::vec3(point.u, point.v, 0.0F);
        return point;
    };
    const Run uniform("uniform float k = 3;\nfloat flat = Du(k);\ncolor tint = Dv(color (1, 2, 3));",
        grid(size), placed);
    BOOST_CHECK_SMALL(uniform.number("flat", 4), 1.0e-6F);
    BOOST_CHECK_SMALL(uniform.triple("tint", 4).y, 1.0e-6F);

    Renderer::Shape lone;
    lone.along = size;
    lone.across = size;
    const Run alone("vector across = Du(P);\nnormal facing = calculatenormal(P);", lone, placed);
    BOOST_CHECK_SMALL(glm::length(alone.triple("across", 4)), 1.0e-6F);
    BOOST_CHECK_SMALL(glm::length(alone.triple("facing", 4)), 1.0e-6F);
}

/**
 * A stencil is a point and its two neighbours, one a step along u and one a step along v.
 * Every point of it takes the derivatives the first point has.
 **/
BOOST_AUTO_TEST_CASE(sl_derivatives_over_a_stencil_test) {
    Renderer::Shape stencil;
    stencil.layout = Renderer::Shape::Layout::STENCIL;
    stencil.du = 0.01F;
    stencil.dv = 0.02F;
    const Run run("vector across = Du(P);\nvector down = Dv(P);", stencil,
        [](const glm::uvec2 & index) {
            v3d::render::offline::sl::Point point;
            const glm::vec3 offsets[3] = { glm::vec3(0.0F), acrossU() * 0.01F, acrossV() * 0.02F };
            point.position = glm::vec3(1.0F, 1.0F, 1.0F) + offsets[index.x];
            return point;
        });
    for (const unsigned int point : { 0U, 1U, 2U }) {
        BOOST_TEST_CONTEXT("point " << point) {
            BOOST_TEST(glm::length(run.triple("across", point) - acrossU()) < 1.0e-3F);
            BOOST_TEST(glm::length(run.triple("down", point) - acrossV()) < 1.0e-3F);
        }
    }
}

};  // namespace
