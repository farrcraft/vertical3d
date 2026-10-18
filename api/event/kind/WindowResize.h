/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <glm/vec2.hpp>

namespace v3d::event::kind {

/**
 **/
class WindowResize final {
 public:
    /**
     **/
    explicit WindowResize(const glm::ivec2& size) noexcept;
    int width() const noexcept;
    int height() const noexcept;

 private:
     int width_;
     int height_;
};
};  // namespace v3d::event::kind
