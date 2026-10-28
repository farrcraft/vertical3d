/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "LitScene.h"

#include <api/render/realtime/DeviceContext.h>
#include <api/render/realtime/Frame.h>
#include <api/render/realtime/Grade.h>
#include <api/render/realtime/MeshRegistry.h>
#include <api/render/realtime/Pass.h>
#include <api/render/realtime/RenderTarget.h>
#include <api/render/realtime/Textures.h>
#include <api/render/realtime/entity/Meshes.h>
#include <api/render/realtime/lighting/SceneUniforms.h>
#include <api/render/realtime/lighting/Shadow.h>
#include <api/render/realtime/renderer/Lit.h>

#include <cstdint>
#include <exception>
#include <stdexcept>
#include <string>
#include <variant>

#include <boost/make_shared.hpp>

namespace v3d::render::realtime {

namespace {

/**
 * The scene target, after the checks every other member is built on.
 **/
boost::shared_ptr<RenderTarget> sceneTarget(const DeviceContext& context, const LitScene::Spec& spec) {
    if (spec.output == VK_FORMAT_UNDEFINED) {
        throw std::runtime_error("The lit scene " + spec.name + " names no format for its grade to draw into");
    }
    if (spec.cascades == 0 || spec.cascades > lighting::maximumCascades) {
        throw std::runtime_error("The lit scene " + spec.name + " asks for " + std::to_string(spec.cascades) +
            " cascades, and a shadow map holds 1 to " + std::to_string(lighting::maximumCascades));
    }
    return boost::make_shared<RenderTarget>(context,
        RenderTarget::Spec{.width = spec.width, .height = spec.height, .colour = spec.colour, .depth = true});
}

/**
 * The shadow map: a sampled depth layer per cascade, or nothing for a scene with no shadow.
 **/
boost::shared_ptr<RenderTarget> shadowMap(const DeviceContext& context, const LitScene::Spec& spec) {
    if (spec.shadowTexels == 0) {
        return boost::shared_ptr<RenderTarget>();
    }
    return boost::make_shared<RenderTarget>(context, RenderTarget::Spec{.width = spec.shadowTexels, .height = spec.shadowTexels,
        .colour = VK_FORMAT_UNDEFINED, .depth = true, .sampledDepth = true, .layers = spec.cascades});
}

/**
 * The lit pipelines, built against the scene target and the shadow map, if there is one.
 **/
renderer::Lit::Spec litSpec(const LitScene::Spec& spec, const RenderTarget& scene, const boost::shared_ptr<RenderTarget>& map) {
    return renderer::Lit::Spec{.colour = scene.format(), .depth = scene.depthFormat(),
        .shadow = map ? map->depthFormat() : VK_FORMAT_UNDEFINED, .shaders = spec.shaders, .front = spec.front};
}

};  // namespace

/**
 **/
LitScene::LitScene(const DeviceContext& context, const Spec& spec) :
    name_(spec.name),
    clearColour_(spec.clearColour),
    logger_(context.logger()),
    textures_(context.textures()),
    ring_(context.ring()),
    scene_(sceneTarget(context, spec)),
    map_(shadowMap(context, spec)),
    lit_(context, litSpec(spec, *scene_, map_)),
    grade_(context, Grade::Spec{.colour = spec.output, .depth = spec.outputDepth, .strip = spec.grade}) {
    if (map_) {
        mapTexture_ = textures_->depthTexture(*map_);
    }
    try {
        source_ = grade_.source(*scene_);
    } catch (...) {
        // no destructor runs for a constructor that throws
        if (mapTexture_.valid()) {
            textures_->release(mapTexture_);
        }
        throw;
    }
}

/**
 **/
LitScene::~LitScene() {
    // a destructor must not throw. A release only retires objects through the ring, which
    // throws only when out of memory. They then stay registered until the context is destroyed
    try {
        grade_.release(source_);
        if (mapTexture_.valid()) {
            textures_->release(mapTexture_);
        }
    } catch (const std::exception& error) {
        logger_->get()->error("The lit scene {} could not release its grade source and shadow map: {}", name_, error.what());
    }
}

/**
 **/
void LitScene::resize(uint32_t width, uint32_t height) {
    scene_->recreate(width, height);
    // the source names the old images, which recreate() retired, so the new ones are bound
    // before the old source goes, and a bind that throws leaves the old source in use
    const MaterialHandle replaced = source_;
    source_ = grade_.source(*scene_);
    grade_.release(replaced);
}

/**
 **/
Pass* LitScene::draw(Frame* frame, Pass* into, const entt::registry& registry, const MeshRegistry& meshes, const Shot& shot) {
    const lighting::Cascades* cascades = std::get_if<lighting::Cascades>(&shot.shadows);
    const glm::mat4* light = std::get_if<glm::mat4>(&shot.shadows);
    const bool shadowed = cascades != nullptr || light != nullptr;
    if (shadowed && !map_) {
        throw std::runtime_error("The lit scene " + name_ + " was built with no shadow map, and a frame named shadows for it");
    }
    if (cascades != nullptr && (cascades->count == 0 || cascades->count > map_->layers())) {
        throw std::runtime_error("The lit scene " + name_ + " has a shadow map of " + std::to_string(map_->layers()) +
            " layers, and a frame named " + std::to_string(cascades->count) + " cascades");
    }
    if (drawn_ == ring_->recording()) {
        throw std::runtime_error("The lit scene " + name_ + " was drawn into this frame already");
    }
    drawn_ = ring_->recording();

    // the scene is written first, because every draw below adds its object to it
    const float texel = map_ ? 1.0F / static_cast<float>(map_->extent().width) : 1.0F;
    const lighting::SceneUniforms uniforms = cascades != nullptr ? lighting::pack(shot.settings, *cascades, texel)
        : lighting::pack(shot.settings, light != nullptr ? *light : glm::mat4(1.0F), texel);
    VkDescriptorSet scene = lit_.scene(uniforms, shadowed ? mapTexture_ : TextureHandle(), shot.poses.palette());

    uint32_t passes = 0;
    if (cascades != nullptr) {
        passes = cascades->count;
    } else if (light != nullptr) {
        passes = 1;
    }
    for (uint32_t cascade = 0; cascade < passes; cascade++) {
        const boost::shared_ptr<Pass> casting = frame->pass(name_ + " shadow " + std::to_string(cascade));
        casting->target(map_);
        casting->layer(cascade);
        casting->depth(true);
        casting->scene(scene);
        casting->depthBias(shot.settings.constantBias, shot.settings.slopeBias);
        entity::Casting casts{.cascade = cascade};
        if (cascades != nullptr) {
            casts.covers = cascades->bounds[cascade];
            casts.towards = shot.settings.light;
        }
        entity::casters(registry, shot.alpha, meshes, &lit_, casting.get(), shot.poses, casts);
    }

    const boost::shared_ptr<Pass> drawn = frame->pass(name_);
    drawn->target(scene_);
    drawn->depth(true);
    if (shadowed) {
        drawn->reads(map_);
    }
    drawn->clearColour(clearColour_);
    drawn->camera(shot.view, shot.projection);
    drawn->scene(scene);
    entity::meshes(registry, shot.alpha, meshes, &lit_, shot.settings.outline, drawn.get(), shot.poses);

    into->reads(scene_);
    grade_.submit(source_, into);
    return drawn.get();
}

/**
 **/
const RenderTarget& LitScene::target() const noexcept {
    return *scene_;
}

/**
 **/
renderer::Lit& LitScene::lit() noexcept {
    return lit_;
}

/**
 **/
Grade& LitScene::grade() noexcept {
    return grade_;
}

};  // namespace v3d::render::realtime
