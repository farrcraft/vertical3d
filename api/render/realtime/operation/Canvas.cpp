/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#include "Canvas.h"

namespace v3d::render::realtime::operation {

    /**
     **/
    Canvas::Canvas(boost::shared_ptr<v3d::gl::Canvas> canvas, boost::shared_ptr<v3d::gl::Program> program) :
        canvas_(canvas),
        program_(program) {
    }

    /**
     **/
    bool Canvas::run(boost::shared_ptr<Context> context) {
        canvas_->upload();
        program_->enable();
        const v3d::gl::VertexBuffer& buffer = canvas_->buffer();
        buffer.render();
        program_->disable();
        return true;
    }

};  // namespace v3d::render::realtime::operation
