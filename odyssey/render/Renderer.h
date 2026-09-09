/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2026 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include <api/asset/Manager.h>
#include <api/log/Logger.h>
#include <api/render/realtime/Canvas.h>
#include <api/render/realtime/Engine3D.h>
#include <odyssey/engine/Player.h>
#include <odyssey/tile/Map.h>

#include <string>

#include <boost/shared_ptr.hpp>
#include <entt/entt.hpp>

namespace odyssey::render {
/**
 * Everything odyssey draws, built as one canvas of quads and handed to the render engine.
 *
 * A tile and a sprite are the same primitive per ADR-0005, so a screen of them reaches
 * the device as one batch per texture. There is one painter ordered pass and no depth
 * buffer, which is what a 2D game wants and what the engine gives it by default - a pass
 * is drawn in submission order unless it asks to be sorted, and allocates depth only when
 * it asks for that.
 **/
class Renderer final {
 public:
    /**
     * @throw std::runtime_error if the device or the swapchain cannot be built
     **/
    Renderer(const boost::shared_ptr<v3d::render::realtime::Window>& window, const boost::shared_ptr<v3d::log::Logger>& logger,
        const boost::shared_ptr<v3d::asset::Manager>& assetManager, entt::registry* registry);

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
     * The board to draw under everything else, or null to draw none.
     **/
    void map(const boost::shared_ptr<odyssey::tile::Map>& map);

    /**
     * Wait for everything in flight, before the window the device draws to goes away.
     **/
    void shutdown();

 private:
    /**
     * The board, one untextured quad per tile. They are drawn first and the pass is in
     * submission order, so everything else lands on top of them.
     **/
    void drawMap();

    /**
     * The sprite, at the tile its entity's position names.
     **/
    void drawPlayer();

    boost::shared_ptr<v3d::log::Logger> logger_;
    boost::shared_ptr<odyssey::engine::Player> player_;
    boost::shared_ptr<odyssey::tile::Map> map_;
    entt::registry* registry_;

    v3d::render::realtime::Canvas canvas_;
    v3d::render::realtime::Engine3D engine_;

    v3d::render::realtime::TextureHandle sprite_;
};

};  // namespace odyssey::render
