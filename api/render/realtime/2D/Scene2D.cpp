/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#include "Scene2D.h"

#include <boost/make_shared.hpp>

namespace v3d::render::realtime {

    /**
     **/
    Scene2D::Scene2D(boost::shared_ptr<Context2D> context) :
        context_(context) {

    }

    /**
     **/
    boost::shared_ptr<Frame> Scene2D::collect() {
        boost::shared_ptr<Frame> frame = boost::make_shared<Frame>(context_);

        // do stuff with frame
        // player_->draw(frame);

        return frame;
    }

};  // namespace v3d::render::realtime
