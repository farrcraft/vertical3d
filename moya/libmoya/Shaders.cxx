/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Shaders.h"

#include <api/log/Logger.h>
#include <api/render/offline/Textures.h>
#include <api/render/offline/rib/Parameters.h>
#include <api/render/offline/sl/Instance.h>
#include <api/render/offline/sl/ShaderLibrary.h>
#include <api/render/offline/sl/Types.h>

#include <string>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::moya {

Shaders::Shaders(const boost::shared_ptr<v3d::log::Logger> & logger) :
    library_(boost::make_shared<v3d::render::offline::sl::ShaderLibrary>(logger)),
    textures_(boost::make_shared<v3d::render::offline::Textures>(logger)) {
}

v3d::render::offline::sl::InstancePtr Shaders::instance(const std::string & name,
    v3d::render::offline::sl::ShaderType type, const v3d::render::offline::rib::ParameterList & parameters) {
    v3d::render::offline::sl::InstancePtr shader = library_->instance(name, type, parameters);
    if (shader && shader->program().traces) {
        traces_ = true;
    }
    return shader;
}

v3d::render::offline::sl::InstancePtr Shaders::constant() {
    return library_->instance("constant", v3d::render::offline::sl::ShaderType::SURFACE,
        v3d::render::offline::rib::ParameterList());
}

bool Shaders::traces() const {
    return traces_;
}

void Shaders::imager(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters) {
    imager_ = instance(name, v3d::render::offline::sl::ShaderType::IMAGER, parameters);
}

const v3d::render::offline::sl::InstancePtr & Shaders::imager() const {
    return imager_;
}

void Shaders::searchpath(const std::string & path) {
    library_->searchpath(path);
}

v3d::render::offline::Textures & Shaders::textures() {
    return *textures_;
}

};  // namespace v3d::moya
