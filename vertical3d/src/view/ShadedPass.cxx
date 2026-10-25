/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "ShadedPass.h"

#include <api/brep/BRep.h>
#include <api/render/realtime/DeviceContext.h>
#include <api/render/realtime/DrawItem.h>
#include <api/render/realtime/Handle.h>
#include <api/render/realtime/Pass.h>
#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/frame/FrameUniforms.h>
#include <api/render/realtime/vulkan/frame/Ring.h>
#include <api/render/realtime/vulkan/memory/Mesh.h>
#include <api/render/realtime/vulkan/pipeline/Builder.h>
#include <api/render/realtime/vulkan/pipeline/Resources.h>
#include <vertical3d/src/scene/Evaluation.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Mesh.h>
#include <vertical3d/src/scene/Scene.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <utility>
#include <vector>

#include "Triangulation.h"
#include "ViewPort.h"

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor {

namespace {

const uint32_t vertexShader[] =
#include "shaders/shaded.vert.inc"
;  // NOLINT(whitespace/semicolon) - the initialiser it terminates is the include above

const uint32_t fragmentShader[] =
#include "shaders/shaded.frag.inc"
;  // NOLINT(whitespace/semicolon) - the initialiser it terminates is the include above

/**
 * One vertex as the pipeline reads it.
 **/
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
};

/**
 * What the shaders read from the push constant: the item's placement and its colour.
 **/
struct Object {
    glm::mat4 model;
    glm::vec4 colour;
};

/**
 * The bias every surface is drawn at, in the depth format's units and in units of the surface's
 * slope. It pushes a surface back just far enough that a line drawn on it wins the depth test.
 **/
constexpr float BIAS_CONSTANT = 1.0F;
constexpr float BIAS_SLOPE = 1.0F;

/**
 * Which side a draw keeps, in the order the pipelines are held.
 **/
constexpr std::size_t FRONT = 0;
constexpr std::size_t BACK = 1;

VkPolygonMode polygon(ViewPort::Fill fill) {
    switch (fill) {
        case ViewPort::Fill::Points:
            return VK_POLYGON_MODE_POINT;
        case ViewPort::Fill::Lines:
            return VK_POLYGON_MODE_LINE;
        case ViewPort::Fill::Fill:
            break;
    }
    return VK_POLYGON_MODE_FILL;
}

};  // namespace

/**
 **/
ShadedPass::ShadedPass(const boost::shared_ptr<v3d::render::realtime::DeviceContext>& context) :
    context_(context),
    nonSolid_(context->device()->nonSolidFill()) {
    for (const std::size_t side : { FRONT, BACK }) {
        for (const ViewPort::Fill fill : { ViewPort::Fill::Points, ViewPort::Fill::Lines, ViewPort::Fill::Fill }) {
            // a fill the device cannot draw is drawn filled, so its pipeline is the filled one
            const VkPolygonMode mode = nonSolid_ ? polygon(fill) : VK_POLYGON_MODE_FILL;
            v3d::render::realtime::vulkan::pipeline::Builder builder(context_->device());
            builder.name(side == FRONT ? "editor-shaded-front" : "editor-shaded-back")
                .shader(VK_SHADER_STAGE_VERTEX_BIT, vertexShader, sizeof(vertexShader))
                .shader(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader, sizeof(fragmentShader))
                .vertexBinding({.binding = 0, .stride = sizeof(Vertex)})
                .vertexAttribute({.location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, position)})
                .vertexAttribute({.location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, normal)})
                .polygon(mode)
                // a triangle winds counter-clockwise seen from outside. The camera's projection
                // flips y, so what reaches the rasterizer winds clockwise
                .cull(side == FRONT ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_CLOCKWISE)
                .depth(true, true)
                .depthBias(true)
                .blend(false)
                .set(context_->frameUniforms()->layout())
                .push({.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, .size = sizeof(Object)})
                .colourFormat(context_->colourFormat())
                .depthFormat(context_->depthFormat());
            pipelines_[side][static_cast<std::size_t>(fill)] = context_->resources()->add(builder.build(context_->pipelineCache()));
        }
    }
}

/**
 **/
ShadedPass::~ShadedPass() {
    for (auto& [key, upload] : uploads_) {
        retire(&upload);
    }
}

/**
 **/
void ShadedPass::submit(const Scene& scene, const ViewPort::Display& display, v3d::render::realtime::Pass* pass) {
    if (pass == nullptr) {
        return;
    }
    const bool front = display.cull == ViewPort::Cull::None || display.cull == ViewPort::Cull::Back;
    const bool back = display.cull == ViewPort::Cull::None || display.cull == ViewPort::Cull::Front;
    if (!front && !back) {
        return;
    }
    pass->depthBias(BIAS_CONSTANT, BIAS_SLOPE);

    const Evaluation evaluation = scene.evaluate();
    for (const boost::shared_ptr<Item>& item : scene.items()) {
        const boost::shared_ptr<Mesh> mesh = as<Mesh>(item);
        if (!mesh) {
            continue;
        }
        const Upload* uploaded = upload(mesh->geometry(), display.shading);
        if (uploaded == nullptr) {
            continue;
        }

        v3d::render::realtime::DrawItem draw;
        uploaded->mesh->describe(&draw);
        draw.instances = 1;
        const Object object{ .model = evaluation.placed(mesh->id()).world, .colour = surface };
        std::memcpy(draw.push.data(), &object, sizeof(object));
        draw.pushSize = sizeof(object);
        if (front) {
            draw.pipeline = pipelines_[FRONT][static_cast<std::size_t>(display.front)];
            pass->submit(draw);
        }
        if (back) {
            draw.pipeline = pipelines_[BACK][static_cast<std::size_t>(display.back)];
            pass->submit(draw);
        }
    }
}

/**
 **/
const ShadedPass::Upload* ShadedPass::upload(const boost::shared_ptr<v3d::brep::BRep>& geometry, Shading shading) {
    Upload& upload = uploads_[std::make_pair(geometry.get(), shading)];
    upload.drawn = true;
    // a mesh freed and another made at its address is another mesh, so the pointer alone is
    // not enough to say an upload is current
    if (upload.mesh && upload.geometry.lock() == geometry && upload.changes == geometry->changes()) {
        return &upload;
    }
    retire(&upload);
    upload.geometry = geometry;
    upload.changes = geometry->changes();

    const Triangles triangles = triangulate(*geometry, shading);
    if (triangles.indices.empty()) {
        return nullptr;
    }
    std::vector<Vertex> vertices;
    vertices.reserve(triangles.positions.size());
    for (std::size_t index = 0; index < triangles.positions.size(); index++) {
        vertices.push_back({ .position = triangles.positions[index], .normal = triangles.normals[index] });
    }
    built_++;
    upload.mesh = boost::make_shared<v3d::render::realtime::vulkan::memory::Mesh>(context_->device(), context_->uploader(),
        std::as_bytes(std::span<const Vertex>(vertices)), static_cast<std::uint32_t>(vertices.size()),
        std::span<const std::uint32_t>(triangles.indices));
    return &upload;
}

/**
 **/
void ShadedPass::collect() {
    for (auto it = uploads_.begin(); it != uploads_.end();) {
        if (!it->second.drawn || it->second.geometry.expired()) {
            retire(&it->second);
            it = uploads_.erase(it);
            continue;
        }
        it->second.drawn = false;
        ++it;
    }
}

/**
 **/
std::size_t ShadedPass::uploaded() const noexcept {
    return uploads_.size();
}

/**
 **/
std::size_t ShadedPass::built() const noexcept {
    return built_;
}

/**
 **/
bool ShadedPass::nonSolid() const noexcept {
    return nonSolid_;
}

/**
 **/
void ShadedPass::retire(Upload* upload) {
    if (!upload->mesh) {
        return;
    }
    // a frame still in flight may read the buffers, so they go when the ring says it is safe
    boost::shared_ptr<v3d::render::realtime::vulkan::memory::Mesh> mesh = std::move(upload->mesh);
    context_->ring()->retire([mesh]() mutable { mesh.reset(); });
}

};  // namespace v3d::editor
