/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

namespace v3d::event {

    /**
     **/
    class Key {
     public:
        /**
         **/
        Key(const std::string_view &name, bool pressed) noexcept;

        /**
         **/
        bool pressed() const noexcept;

        /**
         **/
        std::string_view name() const noexcept;
     private:
        std::string name_;
        bool pressed_;
    };
};  // namespace v3d::event
