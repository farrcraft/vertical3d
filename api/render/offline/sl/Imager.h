/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/FrameBuffer.h>
#include <api/render/offline/sl/Instance.h>
#include <api/render/offline/sl/runtime/Machine.h>
#include <api/render/offline/sl/runtime/Renderer.h>

namespace v3d::render::offline::sl {

/**
 * Runs an imager shader over a finished framebuffer.
 *
 * **A batch here is a row of pixels.** The machine needs no special case for it, as it needs
 * none for a micropolygon grid or a ray hit.
 *
 * It runs the same way after either of moya's hiders, after the last bucket or the last ray,
 * because both hiders produce a framebuffer.
 *
 * A scene uses an imager to set the value of a pixel that nothing was drawn into. Without
 * one, the scene would need a backdrop polygon.
 **/
class Imager final {
 public:
    Imager(const InstancePtr & shader, runtime::Renderer* renderer);

    /**
     * Run it over every pixel.
     *
     * The first three planes are the colour, which the shader reads as `Ci` and writes
     * back. `Oi` is the coverage replicated, because neither hider keeps a separate
     * opacity. The framebuffer holds the colour and how much of the pixel was covered,
     * and when each sample either hits or misses, that coverage is the opacity.
     *
     * @param coverage which plane holds per-pixel coverage, which the shader reads and
     *        writes as `alpha`. `background` sets it, because a pixel it paints counts as
     *        covered
     * @return false when the shader is not an imager, or when a run failed
     **/
    bool run(FrameBuffer* frame, unsigned int coverage);

 private:
    InstancePtr shader_;
    runtime::Renderer* renderer_;
    runtime::Machine machine_;
};

};  // namespace v3d::render::offline::sl
