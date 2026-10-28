/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Renderer.h"

#include "RIBHandler.h"

#include <algorithm>
#include <string>

#include <boost/make_shared.hpp>

namespace v3d::moya {

void Renderer::begin(const std::string & name) {
    handlers_.push_back(boost::make_shared<RIBHandler>(name));
}

void Renderer::end() {
    if (!handlers_.empty()) {
        handlers_.pop_back();
    }
}

bool Renderer::activate(const RIBHandler * handler) {
    const auto found = std::find_if(handlers_.begin(), handlers_.end(),
        [handler](const boost::shared_ptr<RIBHandler> & held) { return held.get() == handler; });
    if (found == handlers_.end()) {
        return false;
    }
    std::rotate(found, found + 1, handlers_.end());
    return true;
}

RIBHandler & Renderer::active() {
    if (handlers_.empty()) {
        begin("");
    }
    return *handlers_.back();
}

};  // namespace v3d::moya
