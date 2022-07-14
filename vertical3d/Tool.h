/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <string>

namespace v3d {

    class Tool {
     public:
            virtual ~Tool() { }

            virtual void activate(const std::string & name) = 0;
            virtual void deactivate(const std::string & name) = 0;
    };


};  // namespace v3d
