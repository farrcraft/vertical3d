/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Renderer.h"

#include <api/asset/Manager.h>
#include <api/ecs/component/Transform.h>
#include <api/font/Face.h>
#include <api/font/Family.h>
#include <api/font/FallbackChain.h>
#include <api/font/Library.h>
#include <api/font/Paragraph.h>
#include <api/font/ShapingCache.h>
#include <api/font/TextStyle.h>
#include <api/log/Logger.h>
#include <api/render/realtime/DeviceContext.h>
#include <api/render/realtime/Frame.h>
#include <api/render/realtime/Handle.h>
#include <api/render/realtime/LitScene.h>
#include <api/render/realtime/MeshRegistry.h>
#include <api/render/realtime/Pass.h>
#include <api/render/realtime/Textures.h>
#include <api/render/realtime/WindowRenderer.h>
#include <api/render/realtime/component/Mesh.h>
#include <api/render/realtime/vulkan/frame/Ring.h>
#include <api/render/realtime/lighting/Shadow.h>
#include <api/render/realtime/canvas/World.h>
#include <api/render/realtime/renderer/World.h>
#include <api/shell/Shell.h>
#include <api/type/camera/Camera.h>
#include <api/type/camera/Isometric.h>
#include <api/ui/paint/CurvePages.h>
#include <api/ui/paint/CurveText.h>
#include <api/ui/paint/GlyphPages.h>
#include <api/ui/shell/Screen.h>
#include <api/window/Window.h>
#include <demos/diorama/src/Layout.h>

#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace diorama {

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
    shell_->present(&engine_, v3d::shell::Shell::game());
    screen_ = shell_->screen();

    // the sign's face, opened through a library of its own and held by the paragraph's style
    const boost::shared_ptr<v3d::font::Family> family = boost::make_shared<v3d::font::Family>("sign");
    family->add(v3d::font::Face::open(boost::make_shared<v3d::font::Library>(), assets->path(SIGN_FONT), logger));
    if (family->empty()) {
        logger->get()->warn("{} could not be opened, so the well has no sign", SIGN_FONT);
        return;
    }
    v3d::font::TextStyle style;
    style.fonts = boost::make_shared<v3d::font::FallbackChain>();
    style.fonts->add(family);
    style.size = SIGN_SIZE;
    style.colour = SIGN_COLOUR;
    // a shadow beneath an outline, so the sign reads against the grass and the sky alike
    style.effects = {
        v3d::font::TextEffect{.colour = SIGN_SHADOW_COLOUR, .width = SIGN_OUTLINE_WIDTH, .softness = SIGN_SHADOW_SOFTNESS, .offset = SIGN_SHADOW_OFFSET},
        v3d::font::TextEffect{.colour = SIGN_OUTLINE_COLOUR, .width = SIGN_OUTLINE_WIDTH}};
    shaping_ = boost::make_shared<v3d::font::ShapingCache>("", logger);
    sign_.emplace(shaping_, SIGN, style);

    // the sign's curves, and the fields its effects are drawn from, are textures on the device
    const boost::shared_ptr<v3d::render::realtime::Textures> textures = engine_.textures();
    signText_ = boost::make_shared<v3d::ui::paint::CurveText>(v3d::ui::paint::CurvePages(
        [textures](const float* texels, uint32_t width, uint32_t height) { return textures->texture({.data = texels, .width = width, .height = height}); },
        [textures](const v3d::render::realtime::TextureHandle& page, const glm::uvec2& at, const float* texels, const glm::uvec2& size) {
            textures->update(page, at, {.data = texels, .width = size.x, .height = size.y});
        },
        [textures](const v3d::render::realtime::TextureHandle& page) { textures->release(page); }),
        v3d::ui::paint::GlyphPages(
        [textures](const unsigned char* pixels, uint32_t side, uint32_t channels) { return textures->texture(pixels, side, side, channels); },
        [textures](const v3d::render::realtime::TextureHandle& page, const glm::uvec2& at, const unsigned char* pixels, const glm::uvec2& size,
            uint32_t channels) {
            textures->update(page, at, {.data = pixels, .width = size.x, .height = size.y, .channels = channels});
        },
        [textures](const v3d::render::realtime::TextureHandle& page) { textures->release(page); }));
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
        // the sign is drawn into the lit pass, so its quads are built for that pass's target
        signs_ = boost::make_shared<v3d::render::realtime::renderer::World>(*context, v3d::render::realtime::renderer::World::Spec{
            .colour = scene_->target().format(), .depth = scene_->target().depthFormat()});
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
    v3d::render::realtime::Pass* lit = scene_->draw(frame.get(), colour.get(), registry, *meshes_, shot);
    drawSign(lit);

    shell_->draw();
    engine_.quads()->submit(screen_->canvas(), colour.get());

    engine_.renderFrame();
}

/**
 **/
void Renderer::drawSign(v3d::render::realtime::Pass* lit) {
    if (!sign_ || !signText_ || !signs_ || lit == nullptr) {
        return;
    }
    // the sign's pixels run along the camera's right and down the world's up, so it faces the
    // camera however the view is turned, and its middle stands over the well
    const glm::vec3 right = glm::normalize(glm::vec3(glm::inverse(camera_.view())[0]));
    const glm::vec3 up(0.0F, 1.0F, 0.0F);
    const float across = sign_->size().x * SIGN_SCALE;
    glm::mat4 standing(1.0F);
    standing[0] = glm::vec4(right * SIGN_SCALE, 0.0F);
    standing[1] = glm::vec4(-up * SIGN_SCALE, 0.0F);
    standing[2] = glm::vec4(glm::cross(right, up), 0.0F);
    standing[3] = glm::vec4(WELL + (up * SIGN_HEIGHT) - (right * (across / 2.0F)), 1.0F);

    v3d::render::realtime::canvas::World world;
    signText_->beginFrame();
    signText_->draw(&world, *sign_, standing);
    signs_->submit(world, lit);
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
    signs_.reset();
    signText_.reset();
    meshes_.reset();
    engine_.shutdown();
}

};  // namespace diorama
