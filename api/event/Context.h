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

        std::string_view name() const;

     private:
        std::string name_;
    };

};  // namespace v3d::event
