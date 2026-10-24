/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Renderer.h"

#include "RenderContext.h"

#include <algorithm>
#include <iostream>
#include <string>

#include <boost/make_shared.hpp>

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

    contexts_.push_back(boost::make_shared<RenderContext>(name));
}

void Renderer::destroyActiveRenderContext(void) {
    if (!contexts_.empty()) {
        contexts_.pop_back();
    }
}

bool Renderer::activate(const RenderContext * context) {
    const auto found = std::find_if(contexts_.begin(), contexts_.end(),
        [context](const boost::shared_ptr<RenderContext> & held) { return held.get() == context; });
    if (found == contexts_.end()) {
        return false;
    }
    std::rotate(found, found + 1, contexts_.end());
    return true;
}

RenderContext & Renderer::activeRenderContext(void) {
    if (contexts_.empty()) {
        createRenderContext("");
    }
    return *contexts_.back();
}

};  // namespace v3d::moya
