/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Renderer.h"

#include <api/asset/Manager.h>
#include <api/ecs/component/Transform.h>
#include <api/log/Logger.h>
#include <api/render/realtime/DeviceContext.h>
#include <api/render/realtime/Frame.h>
#include <api/render/realtime/Handle.h>
#include <api/render/realtime/LitScene.h>
#include <api/render/realtime/MeshRegistry.h>
#include <api/render/realtime/Pass.h>
#include <api/render/realtime/WindowRenderer.h>
#include <api/render/realtime/component/Mesh.h>
#include <api/render/realtime/vulkan/frame/Ring.h>
#include <api/render/realtime/lighting/Shadow.h>
#include <api/shell/Shell.h>
#include <api/type/camera/Camera.h>
#include <api/type/camera/Isometric.h>
#include <api/ui/shell/Screen.h>
#include <api/window/Window.h>
#include <demos/diorama/src/Layout.h>

#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace diorama {

namespace {

/** What the lit pass clears to, a pale blue sky, in linear light. **/
constexpr glm::vec4 SKY(0.45F, 0.62F, 0.85F, 1.0F);

/** The width of the shadow map, in texels. **/
constexpr uint32_t SHADOW_TEXELS = 2048;

/**
 * How far past the casters the shadow map reaches, for the height of a tree and the length of
 * its shadow, in world units.
 **/
constexpr float SHADOW_MARGIN = 3.0F;

/**
 * The camera's near and far planes. The orbit's eye is 30 units from the target, so the far
 * plane takes in the whole ground from any azimuth.
 **/
constexpr glm::vec2 CLIPPING(0.1F, 100.0F);

};  // namespace

/**
 **/
Renderer::Renderer(const boost::shared_ptr<v3d::window::Window>& window, const boost::shared_ptr<v3d::log::Logger>& logger,
    const boost::shared_ptr<v3d::asset::Manager>& assets, v3d::shell::Shell* shell) :
    logger_(logger),
    shell_(shell),
    engine_(logger) {
    engine_.initialize(window);
    meshes_ = boost::make_shared<v3d::render::realtime::MeshRegistry>(logger, engine_.context(), assets);
    camera_.profile().clipping(CLIPPING);

    // after the engine is initialized, because the screen's atlas is uploaded through it
    shell_->present(&engine_, v3d::ui::shell::Screen::Options());
    screen_ = shell_->screen();
}

/**
 **/
bool Renderer::place(entt::registry* registry, const std::vector<Placement>& placements) {
    std::map<std::string, v3d::render::realtime::MeshHandle> loaded;
    for (const Placement& placement : placements) {
        if (!loaded.contains(placement.model)) {
            try {
                loaded[placement.model] = meshes_->load(placement.model);
            } catch (const std::runtime_error& error) {
                logger_->get()->error("unable to load the model {}: {}", placement.model, error.what());
                return false;
            }
            if (meshes_->resolve(loaded[placement.model]) == nullptr) {
                logger_->get()->error("unable to load the model {}", placement.model);
                return false;
            }
        }
        const entt::entity entity = registry->create();
        v3d::ecs::component::Transform transform;
        transform.position = placement.position;
        transform.rotation = v3d::ecs::component::aboutY(placement.yaw);
        registry->emplace<v3d::ecs::component::Transform>(entity, transform);
        registry->emplace<v3d::render::realtime::component::Mesh>(entity,
            v3d::render::realtime::component::Mesh{loaded[placement.model], placement.castsShadow});
    }
    bounds_ = v3d::render::realtime::lighting::fitShadow(*registry, {}, {.margin = SHADOW_MARGIN});
    return true;
}

/**
 **/
void Renderer::resize(uint32_t width, uint32_t height) {
    if (!scene_) {
        const boost::shared_ptr<v3d::render::realtime::DeviceContext> context = engine_.context();
        scene_ = boost::make_shared<v3d::render::realtime::LitScene>(*context, v3d::render::realtime::LitScene::Spec{
            .width = width, .height = height, .output = context->colourFormat(), .shadowTexels = SHADOW_TEXELS,
            .clearColour = SKY});
    } else {
        scene_->resize(width, height);
    }
    camera_.profile().pixelAspect(static_cast<float>(width) / static_cast<float>(height));
}

/**
 **/
void Renderer::draw(const v3d::type::camera::Isometric& orbit, const glm::vec3& light, float alpha,
    const entt::registry& registry) {
    if (!screen_->begin(shell_->ui().get())) {
        return;
    }
    if (screen_->resized()) {
        resize(screen_->canvas().width(), screen_->canvas().height());
    }

    orbit.apply(&camera_);
    camera_.createProjection();
    camera_.createView();

    settings_.light = light;
    v3d::render::realtime::LitScene::Shot shot{.view = camera_.view(), .projection = camera_.projection(),
        .settings = settings_, .alpha = alpha};
    if (bounds_) {
        shot.shadows = v3d::render::realtime::lighting::shadowLight(light, *bounds_);
    }

    // the grade covers the whole of the colour pass, so the scene goes into it first, and the
    // menu and the statistics are drawn over it in submission order
    const boost::shared_ptr<v3d::render::realtime::Frame> frame = engine_.frame();
    const boost::shared_ptr<v3d::render::realtime::Pass> colour = frame->pass(v3d::render::realtime::WindowRenderer::colourPass);
    scene_->draw(frame.get(), colour.get(), registry, *meshes_, shot);

    shell_->draw();
    engine_.quads()->submit(screen_->canvas(), colour.get());

    engine_.renderFrame();
}

/**
 **/
const v3d::type::camera::Camera& Renderer::camera() const noexcept {
    return camera_;
}

/**
 **/
void Renderer::shutdown() {
    // the scene's targets and the models' buffers may still be read by a frame in flight, so
    // the device finishes every frame before they are released
    const boost::shared_ptr<v3d::render::realtime::DeviceContext> context = engine_.context();
    if (context) {
        context->ring()->waitIdleNoThrow();
    }
    scene_.reset();
    meshes_.reset();
    engine_.shutdown();
}

};  // namespace diorama
