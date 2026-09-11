/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Renderer.h"

#include <vector>

namespace v3d::render::offline::sl::runtime {

/*
    What a renderer that cannot do the thing answers. Each is the reading under which a
    scene renders wrong rather than not at all: the machine says out loud what it could not
    do, and a picture missing a shadow is easier to read than no picture. The parameter
    names are in comments because none of them is looked at.
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

};  // namespace v3d::render::offline::sl::runtime
