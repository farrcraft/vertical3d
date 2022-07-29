/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>
#include <string_view>

namespace v3d::event {

    /**
     **/
    class Sound final {
     public:
        /**
         **/
        Sound(const std::string_view &clip) noexcept;
        std::string_view clip() const noexcept;

     private:
        std::string clip_;
    };
};  // namespace v3d::event
