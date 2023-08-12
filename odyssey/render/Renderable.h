/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2021 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "../../api/render/realtime/Frame.h"

namespace odyssey::render {
    /**
     * Forward declaration 
     **/
    class Engine;

    /**
     **/
    class Renderable {
    public:
        /**
         **/
        Renderable(boost::shared_ptr<Engine> renderer);

        /**
         **/
        virtual void draw(boost::shared_ptr<v3d::render::realtime::Frame> frame) = 0;

    protected:
        boost::shared_ptr<odyssey::render::Engine> renderer_;
    };

};  // namespace odyssey::render
