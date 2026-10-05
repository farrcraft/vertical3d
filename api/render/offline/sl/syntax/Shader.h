/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/sl/Types.h>

#include <string>
#include <vector>

#include "Block.h"
#include "Function.h"
#include "Parameter.h"

#include <boost/shared_ptr.hpp>

namespace v3d::render::offline::sl::syntax {

class Shader final {
 public:
    /**
     * Whether a shader of this type runs. A displacement or a volume shader parses and is
     * reported by name, so a scene naming one is reported as unsupported rather than
     * malformed.
     **/
    bool supported() const;

    ShaderType type = ShaderType::SURFACE;
    std::string name;
    std::vector<Parameter> parameters;
    std::vector<Function> functions;
    BlockPtr body;
    unsigned int line = 0;
    unsigned int column = 0;
};

typedef boost::shared_ptr<Shader> ShaderPtr;

};  // namespace v3d::render::offline::sl::syntax
