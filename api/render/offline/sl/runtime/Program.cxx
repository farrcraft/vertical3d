/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Program.h"

#include <string>

namespace v3d::render::offline::sl::runtime {

int Program::symbol(const std::string & wanted) const {
    for (std::size_t i = 0; i < symbols && i < registers.size(); i++) {
        if (registers[i].name == wanted) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

};  // namespace v3d::render::offline::sl::runtime
