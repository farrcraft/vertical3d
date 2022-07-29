/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "PongScene.h"

#include "../../api/ui/Window.h"
#include "../../api/gl/Canvas.h"
#include "../../api/gl/TextureFontRenderer.h"
#include "../../api/font/TextureFontCache.h"
#include "../../api/font/FontCache.h"

#include <boost/shared_ptr.hpp>

class AssetLoader;

class PongRenderer {
 public:
    PongRenderer(boost::shared_ptr<PongScene> scene, const boost::shared_ptr<AssetLoader> & loader, const boost::shared_ptr<v3d::log::Logger> & logger);

    void draw(boost::shared_ptr<v3d::ui::Window> window);
    void resize(int width, int height);

    void drawBall();
    void drawPaddle(const Paddle & paddle);

    boost::shared_ptr<v3d::font::FontCache> fonts() const;

 private:
    boost::shared_ptr<PongScene> scene_;
    boost::shared_ptr<v3d::font::FontCache> fonts_;
    boost::shared_ptr<v3d::gl::Canvas> canvas_;
    v3d::font::TextureTextBuffer::Markup markup_;
    boost::shared_ptr<v3d::gl::TextureFontRenderer> fontRenderer_;
    boost::shared_ptr<v3d::font::TextureFontCache> fontCache_;
};
