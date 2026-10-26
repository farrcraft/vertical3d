/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/ecs/component/Emitter.h>
#include <api/ecs/component/Transform.h>
#include <api/image/Compare.h>
#include <api/image/Image.h>
#include <api/image/reader/Png.h>
#include <api/render/realtime/Handle.h>
#include <api/render/realtime/canvas/DepthOrder.h>
#include <api/render/realtime/Frame.h>
#include <api/render/realtime/entity/Particles.h>
#include <api/render/realtime/Pass.h>
#include <api/render/realtime/entity/Sprites.h>
#include <api/render/realtime/canvas/World.h>
#include <api/render/realtime/component/Particles.h>
#include <api/render/realtime/component/Sprite.h>
#include <api/render/realtime/vulkan/frame/Capture.h>
#include <api/render/realtime/RenderTarget.h>
#include <api/type/animation/SpriteClip.h>
#include <api/type/animation/Track.h>
#include <api/type/camera/Camera.h>
#include <api/type/camera/Isometric.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

#include <entt/entt.hpp>
#include <glm/geometric.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "Headless.h"

using v3d::ecs::component::Emitter;
using v3d::ecs::component::Transform;
using v3d::render::realtime::canvas::DepthOrder;
using v3d::render::realtime::Frame;
using v3d::render::realtime::Pass;
using v3d::render::realtime::TextureHandle;
using v3d::render::realtime::component::Particles;
using v3d::render::realtime::component::Sprite;
using v3d::render::realtime::vulkan::frame::Capture;
using v3d::render::realtime::RenderTarget;
using v3d::type::animation::SpriteClip;
using v3d::type::animation::Track;

namespace canvas = v3d::render::realtime::canvas;

namespace {

const VkFormat colourFormat = VK_FORMAT_R8G8B8A8_UNORM;
const uint32_t width = 128;
const uint32_t height = 128;
const float step = 1.0F / 60.0F;

/**
 * A strip of four soft round puffs, each smaller than the one before, sixteen texels square:
 * a flame's frames as it gutters, made here so that no art is committed for it.
 **/
TextureHandle puffs(v3d::test::Headless* headless) {
    const uint32_t frame = 16;
    std::vector<unsigned char> pixels(static_cast<std::size_t>(frame) * 4 * frame * 4);
    for (uint32_t y = 0; y < frame; y++) {
        for (uint32_t x = 0; x < frame * 4; x++) {
            const uint32_t puff = x / frame;
            const float radius = 7.5F - (static_cast<float>(puff) * 1.5F);
            const float dx = static_cast<float>(x % frame) - 7.5F;
            const float dy = static_cast<float>(y) - 7.5F;
            const float inside = std::clamp(1.0F - (std::sqrt((dx * dx) + (dy * dy)) / radius), 0.0F, 1.0F);
            unsigned char* at = pixels.data() + (((static_cast<std::size_t>(y) * frame * 4) + x) * 4);
            at[0] = 255;
            at[1] = 255;
            at[2] = 255;
            at[3] = static_cast<unsigned char>(inside * 255.0F);
        }
    }
    return headless->context->textures()->texture(pixels.data(), frame * 4, frame, 4);
}

/**
 * The four puffs as a clip that runs once over a flame's life.
 **/
boost::shared_ptr<const SpriteClip> guttering() {
    std::vector<SpriteClip::Frame> frames;
    for (int frame = 0; frame < 4; frame++) {
        const float left = static_cast<float>(frame) * 0.25F;
        frames.push_back(SpriteClip::Frame{glm::vec2(left, 0.0F), glm::vec2(left + 0.25F, 1.0F), 0.25F});
    }
    return boost::make_shared<const SpriteClip>(frames, false);
}

/**
 * A flame rising from where it stands, yellow to red and fading.
 **/
void flame(entt::registry* registry, const glm::vec3& at, const TextureHandle& texture) {
    const entt::entity entity = registry->create();
    registry->emplace<Transform>(entity).position = at;
    Emitter& emitter = registry->emplace<Emitter>(entity, 11U);
    emitter.description.rate = 40.0F;
    emitter.description.shape = v3d::type::effect::Emitter::Shape::Sphere;
    emitter.description.extent = glm::vec3(0.15F);
    emitter.description.spread = 0.3F;
    emitter.description.speedMin = 0.8F;
    emitter.description.speedMax = 1.4F;
    emitter.description.lifeMin = 0.5F;
    emitter.description.lifeMax = 0.9F;
    emitter.description.size = Track<float>({{0.0F, 0.9F}, {1.0F, 0.3F}});
    emitter.description.colour = Track<glm::vec4>({{0.0F, glm::vec4(1.0F, 0.9F, 0.3F, 1.0F)},
        {1.0F, glm::vec4(0.9F, 0.2F, 0.0F, 0.0F)}});
    Particles look;
    look.texture = texture;
    look.clip = guttering();
    look.overLife = true;
    registry->emplace<Particles>(entity, look);
}

/**
 * Smoke drifting up above the flame, grey and thinning.
 **/
void smoke(entt::registry* registry, const glm::vec3& at, const TextureHandle& texture) {
    const entt::entity entity = registry->create();
    registry->emplace<Transform>(entity).position = at;
    Emitter& emitter = registry->emplace<Emitter>(entity, 12U);
    emitter.description.rate = 8.0F;
    emitter.description.spread = 0.4F;
    emitter.description.speedMin = 0.3F;
    emitter.description.speedMax = 0.5F;
    emitter.description.lifeMin = 1.5F;
    emitter.description.lifeMax = 2.0F;
    emitter.description.sway = 0.1F;
    emitter.description.swayRate = 0.5F;
    emitter.description.size = Track<float>({{0.0F, 0.5F}, {1.0F, 1.2F}});
    emitter.description.colour = Track<glm::vec4>({{0.0F, glm::vec4(0.5F, 0.5F, 0.5F, 0.6F)},
        {1.0F, glm::vec4(0.6F, 0.6F, 0.6F, 0.0F)}});
    Particles look;
    look.texture = texture;
    look.uv0 = glm::vec2(0.0F);
    look.uv1 = glm::vec2(0.25F, 1.0F);
    registry->emplace<Particles>(entity, look);
}

void figure(entt::registry* registry, const glm::vec3& at, const glm::vec4& colour) {
    const entt::entity entity = registry->create();
    registry->emplace<Transform>(entity).position = at;
    Sprite sprite;
    sprite.size = glm::vec2(0.6F, 1.4F);
    sprite.tint = colour;
    registry->emplace<Sprite>(entity, sprite);
}

/**
 * Draw what the registry holds, alpha of the way into the step, and read the picture back.
 **/
boost::shared_ptr<v3d::image::Image> draw(v3d::test::Headless* headless, const entt::registry& registry,
    const v3d::type::camera::Camera& camera, float alpha, const std::string& name) {
    const glm::vec3 right = camera.profile().right();
    const glm::vec3 up = camera.profile().up();
    const glm::vec3 forward = camera.profile().direction();
    const glm::vec3 ground = glm::normalize(glm::vec3(forward.x, 0.0F, forward.z));

    DepthOrder order;
    v3d::render::realtime::entity::sprites(registry, alpha, {.right = right, .up = up, .depthAxis = ground}, &order);
    v3d::render::realtime::entity::particles(registry, alpha, {.right = right, .up = up, .depthAxis = ground}, &order);
    canvas::World canvas;
    // dusk
    canvas.tint(glm::vec4(1.0F, 0.75F, 0.55F, 1.0F));
    order.into(&canvas);

    const boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(*headless->context,
        RenderTarget::Spec{.width = width, .height = height, .colour = colourFormat, .depth = false});
    Frame frame;
    const boost::shared_ptr<Pass> pass = frame.pass("world");
    pass->target(target);
    pass->clearColour(glm::vec4(0.15F, 0.2F, 0.3F, 1.0F));
    pass->camera(camera.view(), camera.projection());
    headless->context->worldQuads()->submit(canvas, pass.get());

    Capture capture(headless->device, headless->context->ring(), headless->logger);
    headless->draw(frame, *target, &capture);

    const std::string path = "data_out/" + name + ".png";
    if (!capture.write(path)) {
        return boost::shared_ptr<v3d::image::Image>();
    }
    v3d::image::reader::Png png(headless->logger);
    return png.read(path);
}

BOOST_AUTO_TEST_SUITE(effects_test)

/**
 * A fire in a 2D world under a dusk tint. A flame plays its clip over each particle's life and
 * smoke sways above it, sorted among a figure in front of the fire and one behind.
 * Filtering and blending differ between conformant drivers, so there is no reference picture.
 * The case checks that the validation layer reports no errors and that the fire moves between
 * frames. The frames go to data_out/fire_*.png for a person to look at.
 **/
BOOST_AUTO_TEST_CASE(a_fire_is_drawn_among_sprites) {
    v3d::test::Headless headless(colourFormat, width, height);
    const TextureHandle texture = puffs(&headless);

    v3d::type::camera::Isometric orbit;
    orbit.target(glm::vec3(0.0F, 0.5F, 0.0F));
    orbit.zoom(1.6F);
    v3d::type::camera::Camera camera;
    camera.profile().clipping(glm::vec2(0.1F, 100.0F));
    orbit.apply(&camera);
    camera.createProjection();
    camera.createView();

    const glm::vec3 forward = camera.profile().direction();
    const glm::vec3 ground = glm::normalize(glm::vec3(forward.x, 0.0F, forward.z));
    const glm::vec3 across = glm::normalize(glm::vec3(camera.profile().right().x, 0.0F, camera.profile().right().z));

    // one behind the fire and to its left, one in front and to its right, each overlapping it
    entt::registry registry;
    figure(&registry, (ground * 0.8F) - (across * 0.25F), glm::vec4(0.3F, 0.5F, 0.8F, 1.0F));
    figure(&registry, (ground * -0.8F) + (across * 0.3F), glm::vec4(0.8F, 0.4F, 0.6F, 1.0F));
    flame(&registry, glm::vec3(0.0F), texture);
    smoke(&registry, glm::vec3(0.0F, 0.9F, 0.0F), texture);

    std::vector<boost::shared_ptr<v3d::image::Image>> frames;
    for (int shot = 0; shot < 3; shot++) {
        for (int stepped = 0; stepped < 20; stepped++) {
            v3d::ecs::component::emit(registry, step);
        }
        frames.push_back(draw(&headless, registry, camera, 0.5F, "fire_" + std::to_string(shot)));
        BOOST_REQUIRE(frames.back());
    }
    BOOST_CHECK(headless.silent());
    BOOST_CHECK(!v3d::image::compare(*frames[0], *frames[1], 0).match);
    BOOST_CHECK(!v3d::image::compare(*frames[1], *frames[2], 0).match);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
