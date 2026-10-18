/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2026 Joshua Farr (josh@farrcraft.com)
 **/

#include "Renderer.h"

#include <api/asset/media/kind/Image.h>
#include <api/grid/TileCoord.h>
#include <api/grid/TileGrid.h>
#include <odyssey/engine/Unit.h>

#include <cstddef>
#include <string>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace odyssey::render {

namespace {
/**
 * The one sprite odyssey draws, loaded and uploaded on its own.
 **/
const char* const spriteName = "sample.png";

constexpr glm::vec4 clearColour(0.05f, 0.05f, 0.07f, 1.0f);
constexpr glm::vec4 white(1.0f, 1.0f, 1.0f, 1.0f);

constexpr glm::vec4 wallColour(0.35f, 0.35f, 0.40f, 1.0f);
constexpr glm::vec4 crateColour(0.45f, 0.33f, 0.18f, 1.0f);
constexpr glm::vec4 floorColour(0.14f, 0.15f, 0.18f, 1.0f);

/**
 * A tile is drawn as its kind's colour. There is no tile artwork, so the kind is the only
 * thing that tells tiles apart. An unset texture handle draws against the renderer's white
 * texture, so the quad comes out as the colour alone.
 **/
glm::vec4 tileColour(odyssey::tile::Kind kind) {
    switch (kind) {
    case odyssey::tile::Kind::Wall:
        return wallColour;
    case odyssey::tile::Kind::Crate:
        return crateColour;
    default:
        return floorColour;
    }
}

/**
 * How much of a tile's colour is left when it is drawn from memory rather than from sight.
 *
 * Dimmed rather than recoloured, so a remembered wall still looks like a wall but is
 * visibly out of date.
 **/
constexpr float rememberedLight = 0.4f;

/**
 * A hairline of the clear colour around each tile, so a board of flat quads reads as a
 * grid rather than as one field of colour.
 **/
constexpr float tileGap = 1.0f;

};  // namespace

/**
 **/
Renderer::Renderer(const boost::shared_ptr<v3d::render::realtime::Window>& window, const boost::shared_ptr<v3d::log::Logger>& logger,
    const boost::shared_ptr<v3d::asset::Manager>& assetManager, entt::registry* registry) :
    logger_(logger),
    registry_(registry),
    engine_(logger, assetManager) {
    engine_.initialize(window);
    engine_.clearColour(clearColour);

    const boost::shared_ptr<v3d::asset::media::kind::Image> asset =
        assetManager->load<v3d::asset::media::kind::Image>(spriteName);
    if (!asset || !asset->image()) {
        // the loader has already said which file it could not read. An unset handle draws
        // against the renderer's white texture rather than nothing at all, so the sprite
        // becomes a plain square and the frame is still a frame
        logger_->get()->error("the player sprite is missing, so it is drawn untextured");
        return;
    }
    sprite_ = engine_.textures()->texture(asset->image());
}

/**
 **/
void Renderer::player(const boost::shared_ptr<odyssey::engine::Player>& player) {
    player_ = player;
}

/**
 **/
void Renderer::world(const boost::shared_ptr<odyssey::tile::World>& world) {
    world_ = world;
}

/**
 **/
void Renderer::shutdown() {
    engine_.shutdown();
}

/**
 **/
void Renderer::draw() {
    glm::ivec2 size;
    if (!engine_.beginFrame(&size)) {
        return;
    }
    if (canvas_.width() != static_cast<uint32_t>(size.x) || canvas_.height() != static_cast<uint32_t>(size.y)) {
        canvas_.resize(glm::uvec2(static_cast<uint32_t>(size.x), static_cast<uint32_t>(size.y)));
    }

    canvas_.clear();
    const v3d::grid::TileCoord topLeft = view();
    drawWorld(topLeft);
    drawPlayer(topLeft);

    const boost::shared_ptr<v3d::render::realtime::Pass> pass =
        engine_.frame()->pass(v3d::render::realtime::Engine3D::colourPass);
    engine_.quads()->submit(canvas_, pass.get());

    engine_.renderFrame();
}

/**
 **/
v3d::grid::TileCoord Renderer::view() const {
    v3d::grid::TileCoord player{0, 0};
    if (player_ != nullptr) {
        const v3d::grid::TileCoord* position = registry_->try_get<v3d::grid::TileCoord>(player_->entity());
        if (position != nullptr) {
            player = *position;
        }
    }
    if (!world_) {
        return v3d::grid::TileCoord{0, 0};
    }
    return world_->view(player,
        v3d::grid::TileCoord{odyssey::engine::unit::screen_tile_width, odyssey::engine::unit::screen_tile_height});
}

/**
 **/
void Renderer::drawWorld(v3d::grid::TileCoord view) {
    if (!world_) {
        return;
    }
    for (std::size_t region = 0; region < world_->regions(); region++) {
        const boost::shared_ptr<const odyssey::tile::Map> map = world_->map(region);
        if (!map) {
            continue;
        }
        const odyssey::tile::Sight& sight = world_->sight(region);
        const v3d::grid::TileGrid& grid = *map->grid();
        for (int y = 0; y < grid.height(); y++) {
            for (int x = 0; x < grid.width(); x++) {
                const v3d::grid::TileCoord tile{x, y};
                // the region's grid is placed by its corner, so its own tile becomes a world
                // position, and that is a world tile because a tile is one world unit
                const v3d::grid::TileCoord placed = world_->world(region, tile);
                const int column = placed.x - view.x;
                const int row = placed.y - view.y;
                if (column < 0 || row < 0 || column >= odyssey::engine::unit::screen_tile_width ||
                    row >= odyssey::engine::unit::screen_tile_height) {
                    continue;
                }
                glm::vec4 colour = tileColour(map->kind(tile));
                if (!sight.visible(tile)) {
                    if (!sight.remembered(tile)) {
                        continue;
                    }
                    colour = glm::vec4(glm::vec3(colour) * rememberedLight, colour.a);
                }
                drawTile(column, row, colour);
            }
        }
    }
}

/**
 **/
void Renderer::drawTile(int column, int row, const glm::vec4& colour) {
    const glm::vec2 min(
        static_cast<float>(column * odyssey::engine::unit::tile_width) + tileGap,
        static_cast<float>(row * odyssey::engine::unit::tile_height) + tileGap);
    const glm::vec2 max = min + glm::vec2(
        static_cast<float>(odyssey::engine::unit::tile_width) - (2.0f * tileGap),
        static_cast<float>(odyssey::engine::unit::tile_height) - (2.0f * tileGap));
    canvas_.rect(min, max, colour);
}

/**
 **/
void Renderer::drawPlayer(v3d::grid::TileCoord view) {
    if (!player_) {
        return;
    }
    const v3d::grid::TileCoord* position =
        registry_->try_get<v3d::grid::TileCoord>(player_->entity());
    if (position == nullptr) {
        return;
    }

    // the position is a world tile, and the canvas is in pixels with the origin at its top
    // left, so the view's tile is the top left tile of the screen
    const glm::vec2 min(
        static_cast<float>((position->x - view.x) * odyssey::engine::unit::tile_width),
        static_cast<float>((position->y - view.y) * odyssey::engine::unit::tile_height));
    const glm::vec2 max = min + glm::vec2(
        static_cast<float>(odyssey::engine::unit::tile_width),
        static_cast<float>(odyssey::engine::unit::tile_height));

    canvas_.rect(min, max, glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f), white, sprite_);
}

};  // namespace odyssey::render
