/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/asset/Manager.h>
#include <api/render/realtime/Canvas.h>
#include <api/render/realtime/Engine3D.h>
#include <api/ui/ComponentRenderer.h>
#include <api/ui/Engine.h>
#include <api/ui/StatisticsOverlay.h>
#include <api/ui/TextRenderer.h>

#include <string>

#include "PongScene.h"

#include <boost/shared_ptr.hpp>
#include <entt/entt.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

/**
 * Everything pong draws, built as one canvas of quads and handed to the render engine.
 *
 * The board, the scoreboard and the menu are all the same primitive - see ADR-0005 - so the
 * whole frame reaches the device as one upload and a draw per texture.
 **/
class PongRenderer final {
 public:
    PongRenderer(const boost::shared_ptr<v3d::render::realtime::Window>& window, const boost::shared_ptr<v3d::log::Logger>& logger,
        const boost::shared_ptr<v3d::asset::Manager>& assetManager, entt::registry* registry);

    /**
     * @param statistics what the loop measured about the frame being drawn
     **/
    void draw(const v3d::ui::StatisticsOverlay::Sample& statistics);
    void resize(int width, int height);

    void scene(const boost::shared_ptr<PongScene>& scene);

    /**
     * The ui whose containers are drawn over the game, or null to draw none.
     **/
    void ui(const boost::shared_ptr<v3d::ui::Engine>& ui);

    /**
     * The frame statistics drawn over the game, for whatever shows and hides them.
     **/
    const boost::shared_ptr<v3d::ui::StatisticsOverlay>& statistics() const;

    /**
     * Wait for everything in flight, before the window the device draws to goes away.
     **/
    void shutdown();

 private:
    void drawBoard();
    void drawScores();
    void drawBall();
    void drawPaddle(const Paddle& paddle);

    boost::shared_ptr<PongScene> scene_;
    boost::shared_ptr<v3d::ui::Engine> ui_;

    v3d::render::realtime::Canvas canvas_;
    v3d::render::realtime::Engine3D engine_;

    boost::shared_ptr<v3d::ui::TextRenderer> text_;
    boost::shared_ptr<v3d::ui::StatisticsOverlay> statistics_;
    boost::shared_ptr<v3d::ui::ComponentRenderer> uiRenderer_;
};
