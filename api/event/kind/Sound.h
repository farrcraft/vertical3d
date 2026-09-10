/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>
#include <string_view>

namespace v3d::event::kind {

/**
 **/
class Sound final {
 public:
    /**
     **/
    // not noexcept: clip_ is a std::string built from the view, which allocates
    Sound(const std::string_view &clip);
    std::string_view clip() const noexcept;

 private:
    std::string clip_;
};
};  // namespace v3d::event::kind
