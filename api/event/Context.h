/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

namespace v3d::event {
    /**
     **/
    class Context {
     public:
        explicit Context(const std::string& name);

        void active(bool state);

        std::string_view name() const;
        bool active() const;

     private:
        std::string name_;
        bool active_;
    };

};  // namespace v3d::event
