/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Sound.h"

namespace v3d::event::kind {

Sound::Sound(const std::string_view& clip) : clip_(clip) {
}

std::string_view Sound::clip() const noexcept {
    return clip_;
}
};  // namespace v3d::event::kind
