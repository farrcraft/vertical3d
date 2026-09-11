/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "GridShader.h"

#include <string>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/matrix.hpp>
#include <glm/vec3.hpp>

#include "RenderContext.h"

namespace v3d::moya {

namespace {

typedef v3d::render::offline::sl::runtime::Machine Machine;
typedef v3d::render::offline::sl::runtime::Value Value;

/**
 * Write one triple into a register, if the shader has that global at all.
 *
 * A shader that never mentions `Ng` still has the symbol - the compiler declares every
 * global of the shader's type - so this is really about a program that failed to compile
 * and was substituted, which may be a different shader entirely.
 **/
void put(Machine* machine, int reg, unsigned int point, const glm::vec3 & value) {
    if (reg >= 0) {
        machine->value(reg).triple(point, value);
    }
}

void put(Machine* machine, int reg, unsigned int point, float value) {
    if (reg >= 0) {
        machine->value(reg).number(point, value);
    }
}

};  // namespace

GridShader::GridShader(RenderContext* context) : context_(context) {
}

GridShader::Run & GridShader::run(const v3d::render::offline::sl::InstancePtr & shader,
    unsigned int batch) {
    Run & held = runs_[&shader->program()];
    if (held.program == &shader->program() && held.batch == batch) {
        return held;
    }
    held.program = &shader->program();
    held.batch = batch;
    held.machine.renderer(this);
    held.machine.prepare(shader->program(), batch);
    return held;
}

bool GridShader::space(const std::string & name, glm::mat4x4* matrix) {
    /*
        Every one of these is out of camera space, because that is where the machine
        already is: moya's first pass works there and so its current space is that one.
        talyn's is world space, which is why this is a callback rather than a table the
        library holds. The context's own table runs the other way - it holds world to
        camera and camera to screen - so half of these are an inverse of it.
    */
    if (name == "current" || name == "camera") {
        *matrix = glm::mat4x4(1.0f);
        return true;
    }
    if (name == "shader") {
        // the transform that was in force when the scene instanced the shader, which is
        // what a "point \"shader\" (0, 0, 1)" in it is stated against
        *matrix = shading_ == nullptr ? glm::mat4x4(1.0f) : shading_->placement;
        return true;
    }
    if (name == "world") {
        *matrix = glm::inverse(context_->coordinateSystem("camera"));
        return true;
    }
    const glm::mat4x4 screen = context_->coordinateSystem("screen");
    if (name == "screen") {
        *matrix = screen;
        return true;
    }
    if (name == "raster") {
        *matrix = context_->coordinateSystem("raster") * screen;
        return true;
    }
    if (name == "NDC") {
        // screen space is the canonical volume over [-1, 1] and NDC is the same volume
        // over [0, 1], which is the half and the shift between them
        glm::mat4x4 normalised = glm::scale(glm::mat4x4(1.0f), glm::vec3(0.5f));
        normalised = glm::translate(normalised, glm::vec3(1.0f));
        *matrix = normalised * screen;
        return true;
    }
    // "object" is the transform in force at the primitive rather than at the shader, and
    // a primitive does not carry it: a scene that transforms between Surface and Polygon
    // would get the shader's answer to a question about the geometry's
    return false;
}

unsigned int GridShader::lights() {
    return shading_ == nullptr ? 0u : static_cast<unsigned int>(shading_->lights.size());
}

bool GridShader::light(unsigned int index, const Value & surface, Value* direction,
    Value* colour, std::vector<char>* reached, bool* ambient) {
    if (shading_ == nullptr || index >= shading_->lights.size()) {
        return false;
    }
    const v3d::render::offline::sl::Placed & shining = shading_->lights[index];
    Run & held = run(shining.shader, batch_);
    const v3d::render::offline::sl::runtime::Program & program = shining.shader->program();

    /*
        A light's own space is where the scene put it, and its parameters are stated
        there; the batch it is lighting is in camera space. Writing the parameters through
        the placement is what makes a light placed by a transform land where the scene put
        it rather than at the origin.
    */
    shining.shader->write(&held.machine, shining.placement);

    const int where = program.symbol("Ps");
    const int origin = program.symbol("P");
    const glm::vec3 position = v3d::render::offline::sl::ptransform(shining.placement,
        glm::vec3(0.0f));
    for (unsigned int point = 0; point < batch_; point++) {
        put(&held.machine, where, point, surface.triple(point));
        put(&held.machine, origin, point, position);
    }
    if (!held.machine.run(program)) {
        return false;
    }

    const int away = program.symbol("L");
    const int tint = program.symbol("Cl");
    for (unsigned int point = 0; point < batch_; point++) {
        direction->triple(point, away < 0 ? glm::vec3(0.0f) : held.machine.value(away).triple(point));
        colour->triple(point, tint < 0 ? glm::vec3(0.0f) : held.machine.value(tint).triple(point));
    }
    *reached = held.machine.lit();
    *ambient = shining.shader->ambient();
    return true;
}

void GridShader::shade(const Shading & shading, MicroPolygonGrid* grid) {
    if (!shading.surface || grid == nullptr) {
        return;
    }
    const unsigned int size = grid->size();
    const unsigned int batch = size * size;
    if (batch == 0) {
        return;
    }
    shading_ = &shading;
    batch_ = batch;

    Run & held = run(shading.surface, batch);
    const v3d::render::offline::sl::runtime::Program & program = shading.surface->program();
    shading.surface->write(&held.machine, shading.placement);

    const int position = program.symbol("P");
    const int normal = program.symbol("N");
    const int geometric = program.symbol("Ng");
    const int incident = program.symbol("I");
    const int eye = program.symbol("E");
    const int surfaceColor = program.symbol("Cs");
    const int surfaceOpacity = program.symbol("Os");
    const int s = program.symbol("s");
    const int t = program.symbol("t");
    const int u = program.symbol("u");
    const int v = program.symbol("v");
    const int du = program.symbol("du");
    const int dv = program.symbol("dv");

    // the grid parameters dicing already walks: vertex (i, j) is at i and j over the span,
    // and the spacing between two of them is what a derivative would divide by
    const float span = size > 1 ? static_cast<float>(size - 1) : 1.0f;
    const glm::vec3 opacity = shading.opacity;

    for (unsigned int i = 0; i < size; i++) {
        for (unsigned int j = 0; j < size; j++) {
            const unsigned int point = i * size + j;
            const Vertex vert = grid->vertex(i, j);
            put(&held.machine, position, point, vert.point());
            put(&held.machine, normal, point, vert.normal());
            put(&held.machine, geometric, point, vert.geometricNormal());
            // the eye is the origin of camera space, so the direction the surface is seen
            // along is the point itself
            put(&held.machine, incident, point, vert.point());
            put(&held.machine, surfaceColor, point, vert.color());
            put(&held.machine, surfaceOpacity, point, opacity);
            put(&held.machine, s, point, static_cast<float>(i) / span);
            put(&held.machine, t, point, static_cast<float>(j) / span);
            put(&held.machine, u, point, static_cast<float>(i) / span);
            put(&held.machine, v, point, static_cast<float>(j) / span);
            put(&held.machine, du, point, 1.0f / span);
            put(&held.machine, dv, point, 1.0f / span);
        }
    }
    put(&held.machine, eye, 0, glm::vec3(0.0f));

    if (!held.machine.run(program)) {
        shading_ = nullptr;
        return;
    }

    const int result = program.symbol("Ci");
    if (result >= 0) {
        for (unsigned int i = 0; i < size; i++) {
            for (unsigned int j = 0; j < size; j++) {
                Vertex vert = grid->vertex(i, j);
                vert.color(held.machine.value(result).triple(i * size + j));
                grid->addVertex(vert, i, j);
            }
        }
    }
    shading_ = nullptr;
}

};  // namespace v3d::moya
