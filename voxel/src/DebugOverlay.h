/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "engine/Aligned.h"

#include "../../api/gl/Program.h"
#include "../../api/asset/Manager.h"
#include "../../api/render/realtime/operation/TextureFont.h"
#include "../../api/font/TextureFontCache.h"

#include <boost/shared_ptr.hpp>

class Scene;

class DebugOverlay : public Aligned<16> {
 public:
    DebugOverlay(const boost::shared_ptr<Scene>& scene, const boost::shared_ptr<v3d::gl::Program>& shaderProgram,
        const boost::shared_ptr<v3d::asset::Manager>& assetManager, const boost::shared_ptr<v3d::log::Logger>& logger);

    void enable(bool status);
    void update(unsigned int delta);
    void resize(float width, float height);

    bool enabled() const;

    /**
     * The draw operation for the overlay text.
     * The caller adds it to the frame it is building - the operation uploads and draws
     * itself when the frame runs.
     */
    boost::shared_ptr<v3d::render::realtime::operation::TextureFont> operation() const;

 private:
    boost::shared_ptr<Scene> scene_;
    bool enabled_;
    v3d::font::TextureTextBuffer::Markup markup_;
    boost::shared_ptr<v3d::render::realtime::operation::TextureFont> renderer_;
    boost::shared_ptr<v3d::font::TextureFontCache> fontCache_;
    size_t frames_;
    size_t elapsed_;
};
