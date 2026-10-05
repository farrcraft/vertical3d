/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/FrameBuffer.h>
#include <api/render/offline/Sampling.h>
#include <api/render/offline/Texture.h>

#include <vector>

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
     * Cast a primary ray through every sample of every pixel and write what the film
     * makes of them.
     *
     * The camera is given the framebuffer's size and its matrices are rebuilt here,
     * since Camera::ray() reads the cached ones. Nothing is drawn without a format().
     */
    void render();

    Scene & scene();
    const Scene & scene() const;

    /** The images the scene's shaders read, each once. **/
    v3d::render::offline::Textures & textures();

    /**
     * The imager shader run over the finished frame, which is how a scene says what a
     * ray that hit nothing is worth. Empty until a scene names one.
     */
    void imager(const v3d::render::offline::sl::Placed & shader);

    boost::shared_ptr<v3d::render::offline::FrameBuffer> framebuffer() const;

    /**
     * How the frame is sampled, as the scene asked for it.
     */
    v3d::render::offline::Sampling & sampling();
    const v3d::render::offline::Sampling & sampling() const;

    /**
     * How many samples a pixel took in the last render: PixelSamples' count, or more where
     * a PixelVariance asked for them.
     */
    unsigned int samplesTaken(unsigned int column, unsigned int row) const;

 private:
    boost::shared_ptr<v3d::render::offline::FrameBuffer> framebuffer_;
    Scene scene_;
    v3d::render::offline::Textures textures_;
    v3d::render::offline::sl::Placed imager_;
    v3d::render::offline::Sampling sampling_;
    std::vector<unsigned int> taken_;
};


};  // namespace v3d::talyn
