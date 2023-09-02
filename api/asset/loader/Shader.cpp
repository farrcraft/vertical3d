/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Shader.h"

#include <string>

#include "../Manager.h"
#include "../Shader.h"
#include "../Type.h"
#include "../Text.h"
#include "../../gl/Shader.h"

#include <boost/make_shared.hpp>

namespace v3d::asset::loader {

/**
 **/
Shader::Shader(Manager* manager, Type t, const boost::shared_ptr<v3d::log::Logger>& logger) : Loader(manager, t, logger) {
}

/**
 **/
boost::shared_ptr<Asset> Shader::load(std::string_view name) {
    boost::shared_ptr<v3d::gl::Shader> shader;
    if (type() == Type::SHADER_FRAGMENT) {
        std::string filename = std::string(name) + std::string(".frag");
        boost::shared_ptr<v3d::asset::Text> script = boost::dynamic_pointer_cast<v3d::asset::Text>(manager_->load(filename, Type::TEXT));
        shader.reset(new v3d::gl::Shader(v3d::gl::Shader::SHADER_TYPE_FRAGMENT, script->content()));
    } else if (type() == Type::SHADER_VERTEX) {
        std::string filename = std::string(name) + std::string(".vert");
        boost::shared_ptr<v3d::asset::Text> script = boost::dynamic_pointer_cast<v3d::asset::Text>(manager_->load(filename, Type::TEXT));
        shader.reset(new v3d::gl::Shader(v3d::gl::Shader::SHADER_TYPE_VERTEX, script->content()));
    }
    boost::shared_ptr<Asset> asset = boost::make_shared<v3d::asset::Shader>(std::string(name), type(), shader);
    return asset;
}

};  // namespace v3d::asset::loader
