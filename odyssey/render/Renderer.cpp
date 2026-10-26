/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2026 Joshua Farr (josh@farrcraft.com)
 **/

#include "Renderer.h"

#include <api/asset/Manager.h>
#include <api/asset/media/kind/Image.h>
#include <api/grid/Sight.h>
#include <api/grid/TileCoord.h>
#include <api/grid/TileGrid.h>
#include <api/log/Logger.h>
#include <api/render/realtime/Pass.h>
#include <api/window/Window.h>
#include <odyssey/engine/Player.h>
#include <odyssey/engine/Unit.h>
#include <odyssey/tile/Map.h>
#include <odyssey/tile/Tile.h>
#include <odyssey/tile/World.h>

#include <cstddef>
#include <cstdint>
#include <string>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace odyssey::render {

namespace {
/**
 * The image file of the player's sprite, the only texture odyssey draws.
 **/
const char* const spriteName = "sample.png";

constexpr glm::vec4 clearColour(0.05F, 0.05F, 0.07F, 1.0F);
constexpr glm::vec4 white(1.0F, 1.0F, 1.0F, 1.0F);

constexpr glm::vec4 wallColour(0.35F, 0.35F, 0.40F, 1.0F);
constexpr glm::vec4 crateColour(0.45F, 0.33F, 0.18F, 1.0F);
constexpr glm::vec4 floorColour(0.14F, 0.15F, 0.18F, 1.0F);

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
constexpr float rememberedLight = 0.4F;

/**
 * A hairline of the clear colour around each tile, so a board of flat quads reads as a
 * grid rather than as one field of colour.
 **/
constexpr float tileGap = 1.0F;

};  // namespace

/**
 **/
Renderer::Renderer(const boost::shared_ptr<v3d::window::Window>& window, const boost::shared_ptr<v3d::log::Logger>& logger,
    const boost::shared_ptr<v3d::asset::Manager>& assetManager, entt::registry* registry) :
    logger_(logger),
    registry_(registry),
    engine_(logger) {
    engine_.initialize(window);
    engine_.clearColour(clearColour);

    const boost::shared_ptr<v3d::asset::media::kind::Image> asset =
        assetManager->load<v3d::asset::media::kind::Image>(spriteName);
    if (!asset || !asset->image()) {
        // the loader has already logged which file it could not read. sprite_ stays unset, and
        // an unset handle draws with the renderer's white texture, so the player is drawn as a
        // plain white square
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
        engine_.frame()->pass(v3d::render::realtime::WindowRenderer::colourPass);
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
        const v3d::grid::Sight& sight = world_->sight(region);
        const v3d::grid::TileGrid& grid = *map->grid();
        for (int y = 0; y < grid.height(); y++) {
            for (int x = 0; x < grid.width(); x++) {
                const v3d::grid::TileCoord tile{x, y};
                // the region's own tile plus the region's corner is the world tile
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
        static_cast<float>(odyssey::engine::unit::tile_width) - (2.0F * tileGap),
        static_cast<float>(odyssey::engine::unit::tile_height) - (2.0F * tileGap));
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

    // the position is a world tile. The canvas is in pixels with its origin at the top left,
    // and view is the world tile drawn at that corner
    const glm::vec2 min(
        static_cast<float>((position->x - view.x) * odyssey::engine::unit::tile_width),
        static_cast<float>((position->y - view.y) * odyssey::engine::unit::tile_height));
    const glm::vec2 max = min + glm::vec2(
        static_cast<float>(odyssey::engine::unit::tile_width),
        static_cast<float>(odyssey::engine::unit::tile_height));

    canvas_.rect(min, max, glm::vec2(0.0F, 0.0F), glm::vec2(1.0F, 1.0F), white, sprite_);
}

};  // namespace odyssey::render
