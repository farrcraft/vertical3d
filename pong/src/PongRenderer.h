/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include "PongScene.h"

#include "../../api/font/TextureFontCache.h"
#include "../../api/font/TextureTextBuffer.h"

#include "../../api/asset/Manager.h"
#include "../../api/render/realtime/Canvas.h"
#include "../../api/render/realtime/Engine3D.h"
#include "../../api/ui/Engine.h"
#include "../../api/ui/ComponentRenderer.h"

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

    void draw();
    void resize(int width, int height);

    void scene(const boost::shared_ptr<PongScene>& scene);

    /**
     * The ui whose containers are drawn over the game, or null to draw none.
     **/
    void ui(const boost::shared_ptr<v3d::ui::Engine>& ui);

 private:
    /**
     * Load the font and pack the glyphs pong draws into one atlas, then upload it.
     **/
    void loadFont(const boost::shared_ptr<v3d::asset::Manager>& assetManager, const boost::shared_ptr<v3d::log::Logger>& logger);

    void drawBoard();
    void drawScores();
    void drawBall();
    void drawPaddle(const Paddle& paddle);

    /**
     * Lay a string out at the pen and append its glyphs to the canvas.
     **/
    void drawText(const std::string& text, const glm::vec2& pen, const glm::vec4& colour);

    boost::shared_ptr<PongScene> scene_;
    boost::shared_ptr<v3d::ui::Engine> ui_;

    v3d::render::realtime::Canvas canvas_;
    v3d::render::realtime::Engine3D engine_;

    boost::shared_ptr<v3d::font::TextureFontCache> fontCache_;
    boost::shared_ptr<v3d::font::TextureTextBuffer> text_;
    v3d::font::TextureTextBuffer::Markup markup_;
    v3d::render::realtime::TextureHandle atlas_;

    boost::shared_ptr<v3d::ui::ComponentRenderer> uiRenderer_;
};
