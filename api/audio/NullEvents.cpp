/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "NullEvents.h"

#include <string>
#include <string_view>

namespace v3d::audio {

/**
 **/
bool NullEvents::bank(const std::string& /* path */) {
    return false;
}

/**
 **/
bool NullEvents::play(std::string_view /* event */) {
    return false;
}

/**
 **/
bool NullEvents::parameter(std::string_view /* name */, float /* value */) {
    return false;
}

/**
 **/
void NullEvents::update() {
}

/**
 **/
std::string_view NullEvents::name() const noexcept {
    return "null";
}

};  // namespace v3d::audio
