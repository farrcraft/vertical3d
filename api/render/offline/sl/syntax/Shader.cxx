/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Shader.h"

#include <api/render/offline/sl/Types.h>

namespace v3d::render::offline::sl::syntax {

bool Shader::supported() const {
    // displacement reaches back into dicing and is a geometry change wearing a shading
    // change's clothes; a volume shader has no place to run until there is a volume
    return type == ShaderType::SURFACE || type == ShaderType::LIGHT || type == ShaderType::IMAGER;
}

};  // namespace v3d::render::offline::sl::syntax
