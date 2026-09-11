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
 * This is the third consumer of the machine and the one that proves the batch model was
 * not built for grids alone: **a batch here is a row of pixels**, which is neither a
 * micropolygon grid nor a ray hit and needs no special case to be either.
 *
 * It is shared because both renderers do the same thing with it - moya after the last
 * bucket and talyn after the last ray - and because a framebuffer is already the one
 * structure they have in common.
 *
 * An imager is how a scene says what a pixel nothing was drawn into is worth, which is
 * what phase 2's talyn reference worked around with a backdrop polygon.
 **/
class Imager final {
 public:
    Imager(const InstancePtr & shader, runtime::Renderer* renderer);

    /**
     * Run it over every pixel.
     *
     * The first three planes are the colour, which the shader reads as `Ci` and writes
     * back. `Oi` is the coverage replicated, because neither renderer keeps an opacity
     * of its own: a framebuffer here holds what was drawn and how much of the pixel it
     * covered, and the two are the same number when a sample either lands or does not.
     *
     * @param coverage which plane holds per-pixel coverage, which the shader reads and
     *        writes as `alpha`. `background` sets it, because a pixel it has painted is
     *        no longer one that nothing was drawn into
     * @return false when the shader is not an imager, or when a run failed
     **/
    bool run(FrameBuffer* frame, unsigned int coverage);

 private:
    InstancePtr shader_;
    runtime::Renderer* renderer_;
    runtime::Machine machine_;
};

};  // namespace v3d::render::offline::sl
