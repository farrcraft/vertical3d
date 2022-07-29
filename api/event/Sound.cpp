/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Sound.h"

namespace v3d::event {

    Sound::Sound(const std::string_view& clip) noexcept : clip_(clip) {

    }

    std::string_view Sound::clip() const noexcept {
        return clip_;
    }
};  // namespace v3d::event
