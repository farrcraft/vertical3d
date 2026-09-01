/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "Context.h"
#include "Operation.h"
#include "Pass.h"

#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime {
    /**
     * Everything to be drawn for one image, as a list of passes.
     *
     * A frame is built up during a tick and recorded in one step at the end of it. The list
     * holds one pass while nothing needs more, but it is a list from the start because
     * compositing, offscreen targets and an editor's several viewports are all more passes
     * over the same frame rather than a different kind of frame - see ADR-0003.
     **/
    class Frame {
     public:
        /**
         **/
        explicit Frame(boost::shared_ptr<Context> context);

        /**
         * The pass of that name, added to the end of the list if the frame has none.
         * @return the pass, which stays valid until the frame is destroyed
         **/
        boost::shared_ptr<Pass> pass(const std::string& name);

        /**
         * @return the passes, in the order they will be recorded
         **/
        const std::vector<boost::shared_ptr<Pass>>& passes() const noexcept;

        /**
         * @return the context the frame is drawn against
         **/
        boost::shared_ptr<Context> context() const noexcept;

        /**
         * Drop what every pass has collected, keeping the passes themselves.
         **/
        void reset() noexcept;

        /**
         * The pre-vulkan submission path, where an operation drew itself against the
         * context. Superseded by passes and draw items per ADR-0004, and kept only until
         * the apps still calling it are ported.
         **/
        void addOperation(boost::shared_ptr<Operation> operation);

        /**
         * Run the operations added to the frame. See addOperation.
         **/
        void draw();

     private:
        boost::shared_ptr<Context> context_;
        std::vector<boost::shared_ptr<Pass>> passes_;
        std::vector<boost::shared_ptr<Operation>> operations_;
    };
};  // namespace v3d::render::realtime
