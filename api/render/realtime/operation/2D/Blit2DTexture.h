/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "../../2D/Operation2D.h"
#include "../../2D/Texture2D.h"

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime::operation {
    /**
     * Copy a source texture to a destination texture
     **/
    class Blit2DTexture : public Operation2D {
     public:
        /**
         **/
        Blit2DTexture(boost::shared_ptr<Texture2D> source, boost::shared_ptr<Texture2D> destination);

     protected:
        /**
         **/
        bool run2D(boost::shared_ptr<Context2D> context) override;

     private:
        boost::shared_ptr<Texture2D> source_;
        boost::shared_ptr<Texture2D> destination_;
    };
};  // namespace v3d::render::realtime::operation
