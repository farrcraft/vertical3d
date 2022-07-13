/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "PongScene.h"

#include <vertical3d/hookah/Window.h>
#include <vertical3d/gl/Canvas.h>
#include <vertical3d/gl/TextureFontRenderer.h>
#include <vertical3d/font/TextureFontCache.h>
#include <vertical3d/font/FontCache.h>

#include <boost/shared_ptr.hpp>

class AssetLoader;

class PongRenderer {
 public:
    PongRenderer(boost::shared_ptr<PongScene> scene, const boost::shared_ptr<AssetLoader> & loader);

    void draw(Hookah::Window * window);
    void resize(int width, int height);

    void drawBall();
    void drawPaddle(const Paddle & paddle);

    boost::shared_ptr<v3d::FontCache> fonts() const;

 private:
    boost::shared_ptr<PongScene> scene_;
    boost::shared_ptr<v3d::FontCache> fonts_;
    boost::shared_ptr<v3d::Canvas> canvas_;
    v3d::TextureTextBuffer::Markup markup_;
    boost::shared_ptr<v3d::TextureFontRenderer> fontRenderer_;
    boost::shared_ptr<v3d::TextureFontCache> fontCache_;
};

