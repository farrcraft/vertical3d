/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "ShaderProgram.h"

#include <string>
#include <vector>

#include "../Manager.h"
#include "../ShaderProgram.h"
#include "../Shader.h"
#include "../Type.h"

#include "../../gl/Shader.h"

#include <boost/make_shared.hpp>

namespace v3d::asset::loader {

    /**
     **/
    ShaderProgram::ShaderProgram(Manager* manager, const boost::shared_ptr<v3d::log::Logger>& logger) : Loader(manager, Type::SHADER_PROGRAM, logger) {
    }

    /**
     **/
    boost::shared_ptr<Asset> ShaderProgram::load(std::string_view name) {
        boost::optional<ParameterValue> param = parameter("shaderTypes");
        if (!param) {
            return nullptr;
        }
        unsigned int shaderTypes = std::get<unsigned int>(param.get());

        std::vector<boost::shared_ptr<v3d::gl::Shader>> theShaders;
        boost::shared_ptr<v3d::gl::Program> program;

        if (shaderTypes & v3d::gl::Shader::SHADER_TYPE_VERTEX) {
            boost::shared_ptr<v3d::asset::Shader> shader = boost::dynamic_pointer_cast<v3d::asset::Shader>(manager_->load(name, Type::SHADER_VERTEX));
            theShaders.push_back(shader->shader());
        }

        if (shaderTypes & v3d::gl::Shader::SHADER_TYPE_FRAGMENT) {
            boost::shared_ptr<v3d::asset::Shader> shader = boost::dynamic_pointer_cast<v3d::asset::Shader>(manager_->load(name, Type::SHADER_FRAGMENT));
            theShaders.push_back(shader->shader());
        }

        program = boost::make_shared<v3d::gl::Program>(theShaders);

        boost::shared_ptr<v3d::asset::ShaderProgram> asset = boost::make_shared<v3d::asset::ShaderProgram>(std::string(name), Type::SHADER_PROGRAM, program);
        return asset;
    }
};  // namespace v3d::asset::loader
