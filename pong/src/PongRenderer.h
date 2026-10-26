/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/render/realtime/WindowRenderer.h>
#include <api/shell/Shell.h>

#include <string>

#include "PongScene.h"

#include <boost/shared_ptr.hpp>
#include <entt/entt.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

/**
 * Everything pong draws, built as quads and handed to the render engine.
 *
 * The board and the scores are drawn on a canvas in court units. The menu and the statistics
 * are drawn on the screen's canvas, in front of it. Both go through the batched quad pipeline,
 * so the whole frame reaches the device as one upload and a draw per texture.
 **/
class PongRenderer final {
 public:
    /**
     * @param shell the app's shell, which builds its screen over this renderer's engine. It
     *        outlives the renderer
     **/
    PongRenderer(const boost::shared_ptr<v3d::window::Window>& window, const boost::shared_ptr<v3d::log::Logger>& logger,
        v3d::shell::Shell* shell);

    /**
     * @param alpha how far the frame is between the last simulation step and the next
     **/
    void draw(float alpha);

    void scene(const boost::shared_ptr<PongScene>& scene);

    /**
     * Wait for everything in flight, before the window the device draws to goes away.
     **/
    void shutdown();

 private:
    void drawBoard();
    void drawScores();
    void drawBall(float alpha);
    void drawPaddle(const Paddle& paddle, float alpha);

    boost::shared_ptr<PongScene> scene_;
    v3d::shell::Shell* shell_;

    v3d::render::realtime::WindowRenderer engine_;
    // the court, in its own units, under the screen's canvas
    v3d::render::realtime::canvas::Canvas court_;
};
