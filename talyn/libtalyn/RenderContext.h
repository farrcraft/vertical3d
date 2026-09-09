/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/FrameBuffer.h>

#include <boost/shared_ptr.hpp>

#include "Scene.h"

namespace v3d::talyn {
/**
 * What all does a rc encapsulate? What does the RISpec say about rendering with multiple renderers?
 * Only one RC can be active at a time.
 * The rc holds the current graphics state.
 * How do you share common state between multiple rc's?
 **/
class RenderContext {
 public:
    RenderContext();

    /**
        * Set the size of the framebuffer
        * @param width the width of the image
        * @param height the height of the image
        */
    void format(unsigned int width, unsigned int height);

    /**
     * Cast a primary ray through the centre of every pixel and write what it finds.
     *
     * The camera is given the framebuffer's size and its matrices are rebuilt here,
     * since Camera::ray() reads the cached ones. Nothing is drawn without a format().
     */
    void render();

    Scene & scene();
    const Scene & scene() const;

    boost::shared_ptr<v3d::render::offline::FrameBuffer> framebuffer() const;

 private:
    boost::shared_ptr<v3d::render::offline::FrameBuffer> framebuffer_;
    Scene scene_;
};


};  // namespace v3d::talyn
