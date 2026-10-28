/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Renderer.h"

#include <api/render/offline/Texture.h>

#include "Value.h"

#include <string>
#include <vector>

namespace v3d::render::offline::sl::runtime {

bool Renderer::projected(const std::string & name) const {
    return name == "screen" || name == "raster" || name == "NDC";
}

Renderer::Shape Renderer::shape() const {
    return {};
}

/*
    The defaults for a renderer that cannot do these things. With each default a scene still
    renders, though wrongly, and the machine reports what it could not do. A picture missing
    a shadow is easier to diagnose than no picture. The parameter names are in comments
    because none of them is used.
*/

unsigned int Renderer::lights() {
    return 0;
}

bool Renderer::light(unsigned int /* index */, const Value & /* surface */, const Cone & /* gathering */,
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

bool Renderer::message(const std::string & /* shader */, const std::string & /* name */, Value* /* value */) {
    return false;
}

};  // namespace v3d::render::offline::sl::runtime
