/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/texture/Options.h>

#include <moya/libmoya/scene/Declaration.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>
#include <moya/libmoya/texture/SearchPath.h>

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

namespace v3d::moya::texture {

namespace {

/** A parameter of the scene's options, or its fallback while the scene has none. **/
const scene::Value & option(const scene::Scene & scene, const std::string & name) {
    const std::vector<scene::Handle> options = scene.objects("options");
    if (!options.empty()) {
        return scene.value(options.front(), name);
    }
    for (const scene::Declaration & declaration : scene.schema().kind(*scene.schema().find("options")).parameters) {
        if (declaration.name == name) {
            return declaration.fallback;
        }
    }
    throw std::invalid_argument("the options have no parameter " + name);
}

};  // namespace

Options optionsOf(const scene::Scene & scene) {
    Options out;
    out.searchPath = directoriesOf(option(scene, "textureSearchPath").stringArray().front());
    out.memory = static_cast<std::size_t>(std::max(option(scene, "textureMemory").integerArray().front(), 0)) * kKilobyte;
    out.directory = option(scene, "textureDirectory").stringArray().front();
    return out;
}

};  // namespace v3d::moya::texture
