/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Renderer.h"

#include "RenderContext.h"

#include <iostream>
#include <string>

namespace v3d::moya {


Renderer::Renderer() {
}

Renderer::~Renderer() {
}

void Renderer::createRenderContext(const std::string & name) {
    // printed each time the stack goes from empty to one context, so a process that begins
    // more than one context in turn prints it more than once
    if (contexts_.empty()) {
        std::cout << "The RenderMan (R) Interface Procedures and Protocol are:" << "\n" <<
                     "Copyright 1988, 1989, Pixar" << "\n" <<
                     "All Rights Reserved" << "\n";
    }

    const RenderContext context(name);
    contexts_.push_back(context);
}

void Renderer::destroyActiveRenderContext(void) {
    contexts_.pop_back();
}

RenderContext & Renderer::activeRenderContext(void) {
    if (contexts_.empty()) {
        createRenderContext("");
    }
    return contexts_[contexts_.size() - 1];
}

};  // namespace v3d::moya
