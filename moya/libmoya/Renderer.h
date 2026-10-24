/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include "RenderContext.h"

namespace v3d::moya {
/**
 * A stack of render contexts. The last one on the stack is the active context, and every RI
 * request lands in it. A context does not move while it is on the stack, so a pointer to one
 * stays good until it is destroyed.
 *
 * Moya is not a RenderMan compliant renderer. Compliance forbids any way into the renderer
 * besides the RI calls and RIB, and RenderContext is another way in. A program that uses the
 * RI standard without being compliant must display Pixar's copyright notice. The renderer
 * prints it on stdout each time a context is pushed onto an empty stack.
 **/
class Renderer {
 public:
        Renderer();
        ~Renderer();

        /**
         * Push a new render context with default state, and make it the active one. This is
         * RiBegin.
         *
         * @param name stored with the context and not otherwise read. An empty string is RI_NULL
         **/
        void createRenderContext(const std::string & name);
        /**
         * Pop the active context off the stack. This is RiEnd. With the stack empty it does
         * nothing.
         **/
        void destroyActiveRenderContext(void);
        /**
         * Make a context on the stack the active one, by moving it to the top. This is
         * RiContext. The order of the others is kept.
         *
         * @return false, with the stack unchanged, when the context is not on the stack
         **/
        bool activate(const RenderContext * context);
        /**
         * The active context. A request that arrives with the stack empty creates one, so no
         * request needs RiBegin first.
         **/
        RenderContext & activeRenderContext(void);

 private:
        std::vector<boost::shared_ptr<RenderContext>> contexts_;
};
};  // namespace v3d::moya
