/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "../../api/render/realtime/2D/Scene2D.h"
#include "renderable/Player.h"

#include <boost/shared_ptr.hpp>

namespace odyssey::render {
    /**
     **/
    class Scene final : public v3d::render::realtime::Scene2D {
     public:
        /**
         **/
        boost::shared_ptr<v3d::render::realtime::Frame> collect();

        /**
         **/
        void setPlayer(boost::shared_ptr<renderable::Player> player);

     private:
        boost::shared_ptr<renderable::Player> player_;
    };
};  // namespace odyssey::render
