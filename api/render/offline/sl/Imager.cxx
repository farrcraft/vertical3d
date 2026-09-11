/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Imager.h"

#include <glm/vec3.hpp>

namespace v3d::render::offline::sl {

namespace {

/**
 * Which register each of an imager's globals is, looked up once rather than per row.
 **/
class Globals final {
 public:
    explicit Globals(const runtime::Program & program) :
        colour(program.symbol("Ci")),
        opacity(program.symbol("Oi")),
        alpha(program.symbol("alpha")),
        position(program.symbol("P")) {
    }

    int colour;
    int opacity;
    int alpha;
    int position;
};

/** One pixel's worth of the frame, into the register file. **/
void read(const FrameBuffer & frame, unsigned int coverage, const Globals & globals,
    runtime::Machine* machine, unsigned int column, unsigned int row) {
    const float covered = frame.value(coverage, column, row);
    if (globals.colour >= 0) {
        machine->value(globals.colour).triple(column, glm::vec3(
            frame.value(0, column, row), frame.value(1, column, row), frame.value(2, column, row)));
    }
    if (globals.opacity >= 0) {
        machine->value(globals.opacity).triple(column, glm::vec3(covered));
    }
    if (globals.alpha >= 0) {
        machine->value(globals.alpha).number(column, covered);
    }
    if (globals.position >= 0) {
        // the pixel's centre in raster space, which is where an imager that varies across
        // the frame reads what it varies by
        machine->value(globals.position).triple(column, glm::vec3(
            static_cast<float>(column) + 0.5f, static_cast<float>(row) + 0.5f, 0.0f));
    }
}

/** And back out of it. **/
void write(FrameBuffer* frame, unsigned int coverage, const Globals & globals,
    const runtime::Machine & machine, unsigned int column, unsigned int row) {
    if (globals.colour >= 0) {
        const glm::vec3 shaded = machine.value(globals.colour).triple(column);
        frame->value(0, column, row, shaded.r);
        frame->value(1, column, row, shaded.g);
        frame->value(2, column, row, shaded.b);
    }
    if (globals.alpha >= 0) {
        frame->value(coverage, column, row, machine.value(globals.alpha).number(column));
    }
}

};  // namespace

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
        shader_->write(&machine_);
        for (unsigned int column = 0; column < width; column++) {
            read(*frame, coverage, globals, &machine_, column, row);
        }
        if (!machine_.run(program)) {
            return false;
        }
        for (unsigned int column = 0; column < width; column++) {
            write(frame, coverage, globals, machine_, column, row);
        }
    }
    return true;
}

};  // namespace v3d::render::offline::sl
