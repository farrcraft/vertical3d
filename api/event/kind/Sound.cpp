/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Sound.h"

#include <optional>
#include <string_view>

#include <glm/vec3.hpp>

namespace v3d::event::kind {

Sound::Sound(const std::string_view& clip, float pan) : clip_(clip), pan_(pan) {
}

Sound::Sound(const std::string_view& clip, const glm::vec3& position) : clip_(clip), pan_(0.0F), position_(position) {
}

std::string_view Sound::clip() const noexcept {
    return clip_;
}

float Sound::pan() const noexcept {
    return pan_;
}

std::optional<glm::vec3> Sound::position() const noexcept {
    return position_;
}
};  // namespace v3d::event::kind
