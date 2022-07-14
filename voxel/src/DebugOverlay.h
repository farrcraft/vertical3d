/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "engine/Aligned.h"

#include "../../v3dlibs/gl/Program.h"
#include "../../v3dlibs/gl/TextureFontRenderer.h"
#include "../../v3dlibs/font/TextureFontCache.h"

#include <boost/shared_ptr.hpp>

class AssetLoader;
class Scene;

class DebugOverlay : public Aligned<16> {
 public:
    DebugOverlay(boost::shared_ptr<Scene> scene, boost::shared_ptr<v3d::gl::Program> shaderProgram, 
        boost::shared_ptr<AssetLoader> loader, const boost::shared_ptr<v3d::core::Logger> & logger);

    void enable(bool status);
    void render();
    void update(unsigned int delta);
    void resize(float width, float height);

    bool enabled() const;

 private:
    boost::shared_ptr<Scene> scene_;
    bool enabled_;
    v3d::font::TextureTextBuffer::Markup markup_;
    boost::shared_ptr<v3d::gl::TextureFontRenderer> renderer_;
    boost::shared_ptr<v3d::font::TextureFontCache> fontCache_;
    size_t frames_;
    size_t elapsed_;
};
