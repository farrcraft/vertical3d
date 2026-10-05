/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Imager.h"

#include <api/render/offline/sl/Globals.h>

#include <glm/vec3.hpp>

namespace v3d::render::offline::sl {

Imager::Imager(const InstancePtr & shader, runtime::Renderer* renderer) :
    shader_(shader),
    renderer_(renderer) {
}

bool Imager::run(FrameBuffer* frame, unsigned int coverage) {
    if (!shader_ || frame == nullptr || shader_->type() != ShaderType::IMAGER) {
        return false;
    }
    const unsigned int width = frame->width();
    const unsigned int height = frame->height();
    if (width == 0 || height == 0 || frame->planes() <= coverage) {
        return false;
    }

    const runtime::Program & program = shader_->program();
    machine_.renderer(renderer_);
    // a row of pixels is the batch, sized once for the whole pass
    machine_.prepare(program, width);

    const Globals globals(program);
    for (unsigned int row = 0; row < height; row++) {
        if (!shader_->write(&machine_)) {
            return false;
        }
        for (unsigned int column = 0; column < width; column++) {
            // the pixel's centre in raster space
            globals.pixel(&machine_, column, glm::vec3(frame->value(0, column, row), frame->value(1, column, row),
                frame->value(2, column, row)), frame->value(coverage, column, row),
                glm::vec3(static_cast<float>(column) + 0.5f, static_cast<float>(row) + 0.5f, 0.0f));
        }
        if (!machine_.run()) {
            return false;
        }
        for (unsigned int column = 0; column < width; column++) {
            const glm::vec3 was(frame->value(0, column, row), frame->value(1, column, row), frame->value(2, column, row));
            const glm::vec3 shaded = globals.colour(machine_, column, was);
            frame->value(0, column, row, shaded.r);
            frame->value(1, column, row, shaded.g);
            frame->value(2, column, row, shaded.b);
            frame->value(coverage, column, row, globals.alpha(machine_, column, frame->value(coverage, column, row)));
        }
    }
    return true;
}

};  // namespace v3d::render::offline::sl
