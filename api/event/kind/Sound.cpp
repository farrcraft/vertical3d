/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Sound.h"

#include <string_view>

namespace v3d::event::kind {

Sound::Sound(const std::string_view& clip, float pan) : clip_(clip), pan_(pan) {
}

std::string_view Sound::clip() const noexcept {
    return clip_;
}

float Sound::pan() const noexcept {
    return pan_;
}
};  // namespace v3d::event::kind
