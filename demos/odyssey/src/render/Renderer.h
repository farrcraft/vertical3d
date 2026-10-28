/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2026 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include <api/asset/Manager.h>
#include <api/grid/TileCoord.h>
#include <api/log/Logger.h>
#include <api/render/realtime/WindowRenderer.h>
#include <api/shell/Shell.h>
#include <api/ui/shell/Screen.h>
#include <demos/odyssey/src/engine/Player.h>
#include <demos/odyssey/src/tile/World.h>

#include <string>

#include <boost/shared_ptr.hpp>
#include <entt/entt.hpp>
#include <glm/vec4.hpp>

namespace odyssey::render {
/**
 * Everything odyssey draws, built as one canvas of quads and handed to the window renderer. The
 * canvas is the shell's screen, so the menu and the statistics overlay are drawn over the world.
 *
 * Tiles and sprites both go through the batched quad pipeline, so a screen of them reaches
 * the device as one batch per texture. There is one pass, drawn in submission order, and no
 * depth buffer. This is the engine's default: a pass is drawn in submission order unless
 * it asks to be sorted, and allocates depth only when it asks for it.
 **/
class Renderer final {
 public:
    /**
     * @param shell the app's shell, which builds its screen over this renderer's engine. It
     *        outlives the renderer
     * @throw std::runtime_error if the device or the swapchain cannot be built
     **/
    Renderer(const boost::shared_ptr<v3d::window::Window>& window, const boost::shared_ptr<v3d::log::Logger>& logger,
        const boost::shared_ptr<v3d::asset::Manager>& assetManager, entt::registry* registry, v3d::shell::Shell* shell);

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    /**
     * Build the frame and present it.
     **/
    void draw();

    /**
     * The player whose position the sprite is drawn at, or null to draw none.
     **/
    void player(const boost::shared_ptr<odyssey::engine::Player>& player);

    /**
     * The world to draw under everything else, or null to draw none. Each loaded region is drawn
     * with what the player can see of it: a tile out of sight is drawn from memory, and one never
     * seen is not drawn at all.
     **/
    void world(const boost::shared_ptr<odyssey::tile::World>& world);

    /**
     * Wait for everything in flight, before the window the device draws to goes away.
     **/
    void shutdown();

 private:
    /**
     * The loaded regions, one untextured quad per tile on screen. They are drawn first and the
     * pass is in submission order, so everything else lands on top of them.
     *
     * A tile never seen is not drawn, so unexplored ground and a region not loaded yet both show
     * the clear colour.
     **/
    void drawWorld(v3d::grid::TileCoord view);

    /**
     * One tile's quad, at a column and row of the screen, inset by a hairline on every side.
     **/
    void drawTile(int column, int row, const glm::vec4& colour);

    /**
     * The player's sprite, at the player's world tile.
     **/
    void drawPlayer(v3d::grid::TileCoord view);

    /**
     * @return the world tile at the screen's top left, which follows the player
     **/
    v3d::grid::TileCoord view() const;

    boost::shared_ptr<v3d::log::Logger> logger_;
    boost::shared_ptr<odyssey::engine::Player> player_;
    boost::shared_ptr<odyssey::tile::World> world_;
    entt::registry* registry_;
    v3d::shell::Shell* shell_;

    v3d::render::realtime::WindowRenderer engine_;

    v3d::render::realtime::TextureHandle sprite_;

    // the shell's screen, built over engine_
    v3d::ui::shell::Screen* screen_ = nullptr;
};

};  // namespace odyssey::render
