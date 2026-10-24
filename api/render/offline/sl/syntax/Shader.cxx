/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Shader.h"

#include <api/render/offline/sl/Types.h>

namespace v3d::render::offline::sl::syntax {

bool Shader::supported() const {
    // a volume shader needs a volume to run in, and the renderer has none
    return type != ShaderType::VOLUME;
}

};  // namespace v3d::render::offline::sl::syntax
