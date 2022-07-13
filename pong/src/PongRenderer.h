/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "PongScene.h"

#include "../../v3dlibs/hookah/Window.h"
#include "../../v3dlibs/gl/Canvas.h"
#include "../../v3dlibs/gl/TextureFontRenderer.h"
#include "../../v3dlibs/font/TextureFontCache.h"
#include "../../v3dlibs/font/FontCache.h"

#include <boost/shared_ptr.hpp>

class AssetLoader;

class PongRenderer {
 public:
    PongRenderer(boost::shared_ptr<PongScene> scene, const boost::shared_ptr<AssetLoader> & loader, const boost::shared_ptr<v3d::core::Logger> & logger);

    void draw(Hookah::Window * window);
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
