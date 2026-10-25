/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "GridShader.h"

#include <api/render/offline/Texture.h>
#include <api/render/offline/sl/Globals.h>
#include <api/render/offline/sl/Instance.h>
#include <api/render/offline/sl/Placed.h>
#include <api/render/offline/sl/runtime/Renderer.h>
#include <api/render/offline/sl/runtime/Value.h>
#include <api/render/offline/trace/Scene.h>
#include <moya/libmoya/RenderContext.h>
#include <moya/libmoya/Vertex.h>

#include "MicroPolygonGrid.h"

#include <string>
#include <vector>

#include <glm/geometric.hpp>
#include <glm/mat3x3.hpp>
#include <glm/matrix.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::reyes {

namespace {

typedef v3d::render::offline::sl::runtime::Value Value;

};  // namespace

GridShader::GridShader(RenderContext* context) :
    context_(context), tracer_(&context->traced(), &context->textures()) {
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
    held.globals = v3d::render::offline::sl::Globals(shader->program());
    return held;
}

glm::vec3 GridShader::incident(const glm::vec3 & point) const {
    return context_->perspective() ? point : glm::vec3(0.0F, 0.0F, 1.0F);
}

bool GridShader::space(const std::string & name, glm::mat4x4* matrix) {
    // every matrix maps out of camera space, the reyes hider's current space. The context's
    // table holds world to camera and camera to screen, so "world" is the inverse of its
    // entry and "screen" is the entry itself
    if (name == "current" || name == "camera") {
        *matrix = glm::mat4x4(1.0F);
        return true;
    }
    if (name == "shader") {
        // the placement maps shader space into camera space. This matrix maps camera space
        // into shader space, so it is the inverse
        *matrix = glm::inverse(placement_);
        return true;
    }
    if (name == "world") {
        *matrix = glm::inverse(context_->coordinateSystem("camera"));
        return true;
    }
    // the projected spaces are the ones the ray hider answers with, so a shader that reads
    // one gives the same value under either hider
    const v3d::render::offline::trace::Scene::Projection & projection = context_->traced().projection();
    if (name == "screen") {
        *matrix = projection.screen;
        return true;
    }
    if (name == "raster") {
        *matrix = projection.raster;
        return true;
    }
    if (name == "NDC") {
        *matrix = projection.ndc;
        return true;
    }
    // "object" is the transform in force at the primitive rather than at the shader. The two
    // differ in a scene that transforms between Surface and Polygon
    if (name == "object") {
        *matrix = glm::inverse(object_);
        return true;
    }
    return false;
}

v3d::render::offline::sl::runtime::Renderer::Shape GridShader::shape() const {
    Shape grid;
    grid.layout = Shape::Layout::GRID;
    grid.along = size_;
    grid.across = size_;
    // the same step shade() gives the shader as du and dv
    const float span = size_ > 1 ? static_cast<float>(size_ - 1) : 1.0F;
    grid.du = 1.0F / span;
    grid.dv = grid.du;
    return grid;
}

const v3d::render::offline::Texture* GridShader::texture(const std::string & name) {
    return context_->textures().find(name);
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the signature is Renderer::message
bool GridShader::message(const std::string & shader, const std::string & name, Value* value) {
    const Run* from = nullptr;
    if (shader == "surface") {
        from = surfaceRun_;
    } else if (shader == "displacement") {
        from = displacementRun_;
    } else if (shader == "lightsource") {
        from = lightRun_;
    }
    return from != nullptr && from->machine.output(name, value);
}

bool GridShader::transmission(const Value & from, const Value & to, Value* fraction) {
    for (unsigned int point = 0; point < batch_; point++) {
        const glm::vec3 source(toWorld_ * glm::vec4(from.triple(point), 1.0F));
        const glm::vec3 target(toWorld_ * glm::vec4(to.triple(point), 1.0F));
        fraction->triple(point, tracer_.transmitted({.origin = source, .direction = target - source},
            point < planes_.size() ? planes_[point] : glm::vec3(0.0F)));
    }
    return true;
}

bool GridShader::trace(const Value & origin, const Value & direction, Value* colour) {
    for (unsigned int point = 0; point < batch_; point++) {
        const glm::vec3 start(toWorld_ * glm::vec4(origin.triple(point), 1.0F));
        const glm::vec3 along = glm::mat3(toWorld_) * direction.triple(point);
        colour->triple(point, tracer_.traced({.origin = start, .direction = along},
            point < planes_.size() ? planes_[point] : glm::vec3(0.0F)));
    }
    return true;
}

unsigned int GridShader::lights() {
    return shading_ == nullptr ? 0U : v3d::render::offline::sl::shining(shading_->lights);
}

bool GridShader::light(unsigned int index, const Value & surface, const v3d::render::offline::sl::runtime::Cone & gathering,
    Value* direction,
    Value* colour, std::vector<char>* reached, bool* ambient) {
    if (shading_ == nullptr) {
        return false;
    }
    unsigned int sample = 0;
    const v3d::render::offline::sl::Placed* lit = v3d::render::offline::sl::which(shading_->lights, index, &sample);
    if (lit == nullptr) {
        return false;
    }
    const v3d::render::offline::sl::Placed & shining = *lit;
    Run & held = run(shining.shader, batch_);
    // an area light's samples are taken in world space, so a vertex and a traced hit at one
    // point sample the light alike, and brought into camera space to run the light there
    std::vector<glm::vec3> from;
    if (shining.area) {
        const glm::mat4x4 toCamera = context_->coordinateSystem("camera");
        for (unsigned int point = 0; point < batch_; point++) {
            const glm::vec3 world(toWorld_ * glm::vec4(surface.triple(point), 1.0F));
            from.emplace_back(toCamera * glm::vec4(shining.area->point(world, sample), 1.0F));
        }
    }

    // the points being lit are in camera space. "shader" space is the light's placement
    // while it runs, and the surface's again afterwards
    const glm::mat4x4 was = placement_;
    placement_ = shining.placement;
    const bool ran = held.globals.shine(shining, &held.machine, batch_, surface, gathering, direction, colour, reached,
        ambient, shining.area ? &from : nullptr);
    placement_ = was;
    // the illuminance body that runs next for this light reads its output parameters
    lightRun_ = ran ? &held : nullptr;
    if (ran && shining.area) {
        // each sample carries its share of the light
        for (unsigned int point = 0; point < batch_; point++) {
            colour->triple(point, colour->triple(point) / static_cast<float>(shining.area->samples()));
        }
    }
    return ran;
}

void GridShader::displace(const Shading & shading, MicroPolygonGrid* grid) {
    // a grid with no displacement shader has none for its surface to read
    displacementRun_ = nullptr;
    if (!shading.displacement.shader || grid == nullptr) {
        return;
    }
    const unsigned int size = grid->size();
    const unsigned int batch = size * size;
    if (batch == 0) {
        return;
    }
    // a displacement shader names no lights, so shading_ stays null while it runs
    placement_ = shading.displacement.placement;
    object_ = shading.object;
    batch_ = batch;
    size_ = size;
    Run & held = run(shading.displacement.shader, batch);
    if (!shading.displacement.shader->write(&held.machine, shading.displacement.placement)) {
        return;
    }
    const float span = size > 1 ? static_cast<float>(size - 1) : 1.0F;
    for (unsigned int i = 0; i < size; i++) {
        for (unsigned int j = 0; j < size; j++) {
            const Vertex vert = grid->vertex(i, j);
            v3d::render::offline::sl::Point point;
            point.position = vert.point();
            point.normal = vert.normal();
            point.geometric = vert.geometricNormal();
            point.incident = incident(vert.point());
            const glm::vec2 st = vert.hasTexCoord() ? vert.st() :
                glm::vec2(static_cast<float>(i) / span, static_cast<float>(j) / span);
            point.s = st.x;
            point.t = st.y;
            point.u = static_cast<float>(i) / span;
            point.v = static_cast<float>(j) / span;
            point.du = 1.0F / span;
            point.dv = 1.0F / span;
            held.globals.surface(&held.machine, (i * size) + j, point);
        }
    }
    held.globals.eye(&held.machine, glm::vec3(0.0F));
    if (!held.machine.run()) {
        return;
    }
    displacementRun_ = &held;
    for (unsigned int i = 0; i < size; i++) {
        for (unsigned int j = 0; j < size; j++) {
            Vertex vert = grid->vertex(i, j);
            const unsigned int lane = (i * size) + j;
            vert.point(held.globals.position(held.machine, lane, vert.point()));
            vert.normal(held.globals.normal(held.machine, lane, vert.normal()));
            grid->addVertex(vert, i, j);
        }
    }
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
    placement_ = shading.placement;
    object_ = shading.object;
    batch_ = batch;
    size_ = size;
    toWorld_ = glm::inverse(context_->coordinateSystem("camera"));
    const glm::mat3 toWorldNormal = glm::transpose(glm::inverse(glm::mat3(toWorld_)));
    planes_.assign(batch, glm::vec3(0.0F));
    tracer_.time(context_->sampling().shutter.x);

    Run & held = run(shading.surface, batch);
    if (!shading.surface->write(&held.machine, shading.placement)) {
        shading_ = nullptr;
        return;
    }

    // the grid parameters dicing uses: vertex (i, j) is at u = i / span and v = j / span, and
    // the spacing between two neighbours is the step a derivative divides by
    const float span = size > 1 ? static_cast<float>(size - 1) : 1.0F;

    for (unsigned int i = 0; i < size; i++) {
        for (unsigned int j = 0; j < size; j++) {
            const unsigned int lane = (i * size) + j;
            const Vertex vert = grid->vertex(i, j);
            v3d::render::offline::sl::Point point;
            point.position = vert.point();
            point.normal = vert.normal();
            point.geometric = vert.geometricNormal();
            point.incident = incident(vert.point());
            point.colour = vert.color();
            point.opacity = shading.opacity;
            const glm::vec2 st = vert.hasTexCoord() ? vert.st() :
                glm::vec2(static_cast<float>(i) / span, static_cast<float>(j) / span);
            point.s = st.x;
            point.t = st.y;
            point.u = static_cast<float>(i) / span;
            point.v = static_cast<float>(j) / span;
            point.du = 1.0F / span;
            point.dv = 1.0F / span;
            held.globals.surface(&held.machine, lane, point);

            const glm::vec3 plane = toWorldNormal * vert.geometricNormal();
            planes_[lane] = glm::length(plane) > 0.0F ? glm::normalize(plane) : plane;
        }
    }
    held.globals.eye(&held.machine, glm::vec3(0.0F));

    surfaceRun_ = &held;
    lightRun_ = nullptr;
    const bool ran = held.machine.run();
    surfaceRun_ = nullptr;
    lightRun_ = nullptr;
    // the displacement belongs to this grid, and the next grid sets its own
    displacementRun_ = nullptr;
    if (!ran) {
        shading_ = nullptr;
        return;
    }

    // Ci and Oi go back on each vertex. A shader that never writes Oi leaves Os there
    for (unsigned int i = 0; i < size; i++) {
        for (unsigned int j = 0; j < size; j++) {
            Vertex vert = grid->vertex(i, j);
            vert.color(held.globals.colour(held.machine, (i * size) + j, vert.color()));
            vert.opacity(held.globals.opacity(held.machine, (i * size) + j, shading.opacity));
            grid->addVertex(vert, i, j);
        }
    }
    shading_ = nullptr;
}

};  // namespace v3d::moya::reyes
