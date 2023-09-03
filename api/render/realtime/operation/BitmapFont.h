/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "../Operation.h"
#include "../../../font/BitmapTextBuffer.h"
#include "../../../gl/VertexBuffer.h"
#include "../../../gl/Program.h"
#include "../../../gl/GLTexture.h"
#include "../../../log/Logger.h"

namespace v3d::render::realtime::operation {
    /**
     * Draw a GL font at position
     **/
    class BitmapFont : public Operation {
     public:
        /**
         *
         **/
        BitmapFont(boost::shared_ptr<v3d::font::BitmapTextBuffer> buffer, boost::shared_ptr<v3d::gl::Program> program, const boost::shared_ptr<v3d::log::Logger>& logger);

        boost::shared_ptr<v3d::font::BitmapTextBuffer> buffer();

        void upload();
        void clear();

        /**
         *
         **/
        bool run(boost::shared_ptr<Context> context);

     private:
        boost::shared_ptr<v3d::font::BitmapTextBuffer> buffer_;
        boost::shared_ptr<v3d::gl::Program> program_;
        v3d::gl::VertexBuffer vertexBuffer_;
        v3d::gl::GLTexture texture_;
    };
};  // namespace v3d::render::realtime::operation
