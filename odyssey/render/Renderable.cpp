/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2021 Joshua Farr (josh@farrcraft.com)
  **/

#include "Renderable.h"

namespace odyssey::render {

    /**
     **/
    Renderable::Renderable(boost::shared_ptr<v3d::render::realtime::Engine2D> renderer) :
        renderer_(renderer) {
    }

};  // namespace odyssey::render