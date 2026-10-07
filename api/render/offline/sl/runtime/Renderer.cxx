/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Renderer.h"

#include <string>
#include <vector>

namespace v3d::render::offline::sl::runtime {

/*
    The defaults for a renderer that cannot do these things. Each default lets a scene
    render wrongly rather than not at all: the machine reports what it could not do, and a
    picture missing a shadow is easier to diagnose than no picture. The parameter names are
    in comments because none of them is used.
*/

unsigned int Renderer::lights() {
    return 0;
}

bool Renderer::light(unsigned int /* index */, const Value & /* surface */,
    Value* /* direction */, Value* /* colour */, std::vector<char>* /* reached */,
    bool* /* ambient */) {
    return false;
}

bool Renderer::transmission(const Value & /* from */, const Value & /* to */,
    Value* /* fraction */) {
    return false;
}

bool Renderer::trace(const Value & /* origin */, const Value & /* direction */,
    Value* /* colour */) {
    return false;
}

const Texture* Renderer::texture(const std::string & /* name */) {
    return nullptr;
}

};  // namespace v3d::render::offline::sl::runtime
