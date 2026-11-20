/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <array>

namespace v3d::moya::material::internal {

/**
 * A surface shader's definition, the definition of its translation onto OpenPBR Surface, and the
 * inputs that pass to OpenPBR's as they are.
 **/
struct Translation {
    /** An input of the shader, and the input of OpenPBR's it passes to. **/
    struct Passed {
        const char * from { nullptr };
        const char * to { nullptr };
    };

    const char * definition { nullptr };
    const char * translator { nullptr };
    std::array<Passed, 4> passed {};
};

};  // namespace v3d::moya::material::internal
