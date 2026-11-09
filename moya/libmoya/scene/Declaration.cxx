/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/scene/Declaration.h>

#include <moya/libmoya/scene/Value.h>

#include <algorithm>
#include <stdexcept>
#include <string>

namespace v3d::moya::scene {

const char * name(Interpolation interpolation) {
    switch (interpolation) {
        case Interpolation::Constant: return "constant";
        case Interpolation::Uniform: return "uniform";
        case Interpolation::Varying: return "varying";
        case Interpolation::Vertex: return "vertex";
        case Interpolation::FaceVarying: return "facevarying";
    }
    return "unknown";
}

void Declaration::admit(const Value & value) const {
    if (count != kAny && value.size() != count) {
        throw std::invalid_argument("parameter " + name + " holds " + std::to_string(count) + " elements, not " + std::to_string(value.size()));
    }
    if (choices.empty()) {
        return;
    }
    for (const std::string & chosen : value.stringArray()) {
        if (std::find(choices.begin(), choices.end(), chosen) == choices.end()) {
            throw std::invalid_argument("parameter " + name + " cannot be " + chosen);
        }
    }
}

};  // namespace v3d::moya::scene
