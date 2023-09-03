/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "../../../api/render/realtime/2D/Engine2D.h"
#include "../../api/render/realtime/Frame.h"

namespace odyssey::render {
    /**
     **/
    class Renderable {
    public:
        /**
         **/
        Renderable(boost::shared_ptr<v3d::render::realtime::Engine2D> renderer);

        /**
         **/
        virtual void draw(boost::shared_ptr<v3d::render::realtime::Frame> frame) = 0;

    protected:
        boost::shared_ptr<v3d::render::realtime::Engine2D> renderer_;
    };

};  // namespace odyssey::render
