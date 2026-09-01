/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "../Operation.h"
#include "Context2D.h"

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime {
    /**
     * Base for operations that draw through SDL_Renderer.
     *
     * A Frame carries the engine's Context, so that is what every Operation is handed. The
     * 2D operations need the Context2D behind it, so this narrows it once, here, rather
     * than in each operation - and reports rather than crashes if a 2D operation was added
     * to a frame built on some other context.
     **/
    class Operation2D : public Operation {
     public:
        /**
         * Narrows the context and calls run2D. Not for overriding - implement run2D.
         **/
        bool run(boost::shared_ptr<Context> context) override;

     protected:
        /**
         * Run the operation against the 2D context.
         **/
        virtual bool run2D(boost::shared_ptr<Context2D> context) = 0;
    };
};  // namespace v3d::render::realtime
