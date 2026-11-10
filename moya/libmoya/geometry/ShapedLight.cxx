/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/geometry/ShapedLight.h>

#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Schema.h>
#include <moya/libmoya/scene/Value.h>

namespace v3d::moya::geometry {

bool shaped(const scene::Scene & scene, const scene::Handle & handle) {
    const auto & kind = scene.schema().kind(handle.kind).name;
    if (kind == "sphereLight") {
        return scene.value(handle, "radius").floatArray().front() > 0.0F && scene.value(handle, "treatAsPoint").integerArray().front() == 0;
    }
    return kind == "rectLight" || kind == "diskLight" || kind == "cylinderLight";
}

};  // namespace v3d::moya::geometry
