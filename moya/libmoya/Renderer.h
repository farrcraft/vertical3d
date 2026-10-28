/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include "RIBHandler.h"

namespace v3d::moya {
/**
 * The stack of handlers the RI C interface drives. RiBegin pushes one and RiEnd pops it. The
 * last one on the stack is the active handler, and every RI call becomes a request to it. A
 * handler does not move while it is on the stack, so a pointer to one stays good until it is
 * popped.
 **/
class Renderer {
 public:
        /**
         * Push a new handler with default state, and make it the active one. This is RiBegin.
         *
         * @param name stored with its context and not otherwise read. An empty string is RI_NULL
         **/
        void begin(const std::string & name);
        /**
         * Pop the active handler off the stack. This is RiEnd. With the stack empty it does
         * nothing.
         **/
        void end();
        /**
         * Make a handler on the stack the active one, by moving it to the top. This is
         * RiContext. The order of the others is kept.
         *
         * @return false, with the stack unchanged, when the handler is not on the stack
         **/
        bool activate(const RIBHandler * handler);
        /**
         * The active handler. A call that arrives with the stack empty pushes one, so no call
         * needs RiBegin first.
         **/
        RIBHandler & active();

 private:
        std::vector<boost::shared_ptr<RIBHandler>> handlers_;
};
};  // namespace v3d::moya
