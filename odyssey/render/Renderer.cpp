/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2026 Joshua Farr (josh@farrcraft.com)
 **/

#include "Renderer.h"

#include <string>

#include "../engine/Unit.h"

#include "../../api/asset/Image.h"
#include "../../api/ecs/component/PositionFixed2D.h"

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace odyssey::render {

namespace {
/**
 * The one sprite odyssey draws, loaded and uploaded on its own.
 **/
const char* const spriteName = "sample.png";

const glm::vec4 clearColour(0.05f, 0.05f, 0.07f, 1.0f);
const glm::vec4 white(1.0f, 1.0f, 1.0f, 1.0f);

};  // namespace

/**
 **/
Renderer::Renderer(const boost::shared_ptr<v3d::render::realtime::Window>& window, const boost::shared_ptr<v3d::log::Logger>& logger,
    const boost::shared_ptr<v3d::asset::Manager>& assetManager, entt::registry* registry) :
    logger_(logger),
    registry_(registry),
    engine_(logger, assetManager, registry) {
    engine_.initialize(window);
    engine_.clearColour(clearColour);

    boost::shared_ptr<v3d::asset::Image> asset =
        boost::dynamic_pointer_cast<v3d::asset::Image>(assetManager->loadTypeFromExt(spriteName));
    if (!asset || !asset->image()) {
        // the loader has already said which file it could not read. An unset handle draws
        // against the renderer's white texture rather than nothing at all, so the sprite
        // becomes a plain square and the frame is still a frame
        logger_->get()->error("the player sprite is missing, so it is drawn untextured");
        return;
    }
    sprite_ = engine_.quads()->texture(asset->image());
}

/**
 **/
void Renderer::player(const boost::shared_ptr<odyssey::engine::Player>& player) {
    player_ = player;
}

/**
 **/
void Renderer::shutdown() {
    engine_.shutdown();
}

/**
 **/
void Renderer::draw() {
    const int width = engine_.window()->width();
    const int height = engine_.window()->height();
    if (width <= 0 || height <= 0) {
        // a minimized window: the engine skips the frame, and a canvas with no area has
        // no projection to build geometry against
        engine_.renderFrame();
        return;
    }
    if (canvas_.width() != static_cast<uint32_t>(width) || canvas_.height() != static_cast<uint32_t>(height)) {
        canvas_.resize(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
    }

    canvas_.clear();
    drawPlayer();

    boost::shared_ptr<v3d::render::realtime::Pass> pass =
        engine_.frame()->pass(v3d::render::realtime::Engine3D::colourPass);
    engine_.quads()->submit(canvas_, pass.get());

    engine_.renderFrame();
}

/**
 **/
void Renderer::drawPlayer() {
    if (!player_) {
        return;
    }
    const v3d::ecs::component::PositionFixed2D* position =
        registry_->try_get<v3d::ecs::component::PositionFixed2D>(player_->entity());
    if (position == nullptr) {
        return;
    }

    // the position is in tiles, and the canvas is in pixels with the origin at its top
    // left, so a tile of (0, 0) is the top left tile of the screen
    const glm::vec2 min(
        static_cast<float>(position->x() * odyssey::engine::unit::tile_width),
        static_cast<float>(position->y() * odyssey::engine::unit::tile_height));
    const glm::vec2 max = min + glm::vec2(
        static_cast<float>(odyssey::engine::unit::tile_width),
        static_cast<float>(odyssey::engine::unit::tile_height));

    canvas_.rect(min, max, glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f), white, sprite_);
}

};  // namespace odyssey::render
