/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

namespace v3d::event {

    /**
     **/
    class WindowResize final {
     public:
        /**
         **/
        WindowResize(int width, int height) noexcept;
        int width() const noexcept;
        int height() const noexcept;

     private:
         int width_;
         int height_;
    };
};  // namespace v3d::event
