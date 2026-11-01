/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

namespace v3d::moya::raytrace {

/** Which pass of which row is being cast, and the first column of the row's pixels. **/
class Pass final {
 public:
    unsigned int row = 0;
    unsigned int pass = 0;
    unsigned int left = 0;
};

};  // namespace v3d::moya::raytrace
