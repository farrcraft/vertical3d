/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "PongScene.h"

#include "../../api/gl/Canvas.h"

#include "../../api/font/TextureTextBuffer.h"
#include "../../api/font/TextureFontCache.h"

#include "../../api/render/realtime/Engine3D.h"
#include "../../api/render/realtime/operation/TextureFont.h"
#include "../../api/asset/Manager.h"

#include <boost/shared_ptr.hpp>
#include <entt/entt.hpp>

class PongRenderer final {
 public:
    PongRenderer(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<v3d::asset::Manager>& assetManager, entt::registry& registry);

    void draw();
    void resize(int width, int height);

    void drawBall();
    void drawPaddle(const Paddle & paddle);

 private:
    boost::shared_ptr<PongScene> scene_;
    boost::shared_ptr<v3d::gl::Canvas> canvas_;
    v3d::font::TextureTextBuffer::Markup markup_;
    boost::shared_ptr<v3d::gl::Program> canvasProgram_;
    boost::shared_ptr<v3d::render::realtime::operation::TextureFont> fontRenderer_;
    boost::shared_ptr<v3d::font::TextureFontCache> fontCache_;
    v3d::render::realtime::Engine3D engine_;
};
