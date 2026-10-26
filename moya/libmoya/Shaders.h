/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>
#include <api/render/offline/Textures.h>
#include <api/render/offline/rib/Parameters.h>
#include <api/render/offline/sl/Instance.h>
#include <api/render/offline/sl/ShaderLibrary.h>
#include <api/render/offline/sl/Types.h>

#include <string>

#include <boost/shared_ptr.hpp>

namespace v3d::moya {

/**
 * The shaders a scene binds, the textures they read, and the imager.
 **/
class Shaders final {
 public:
    explicit Shaders(const boost::shared_ptr<v3d::log::Logger> & logger);

    /**
     * A shader by name, with the scene's parameters. Null when the library cannot load it, and
     * the library has logged why.
     **/
    v3d::render::offline::sl::InstancePtr instance(const std::string & name, v3d::render::offline::sl::ShaderType type,
        const v3d::render::offline::rib::ParameterList & parameters);
    /** "constant", the surface a primitive is shaded by when the scene names none. **/
    v3d::render::offline::sl::InstancePtr constant();
    /** Whether any shader the scene bound calls trace or transmission. **/
    bool traces() const;

    /** RiImager. Null when the scene named none. **/
    void imager(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters);
    const v3d::render::offline::sl::InstancePtr & imager() const;

    /** Where a .sl file that is not built in is looked for, from Option "searchpath" "shader". **/
    void searchpath(const std::string & path);
    /**
     * The images the shaders read, each loaded once, and where a relative name is looked for,
     * from Option "searchpath" "texture".
     **/
    v3d::render::offline::Textures & textures();

 private:
    boost::shared_ptr<v3d::render::offline::sl::ShaderLibrary> library_;
    boost::shared_ptr<v3d::render::offline::Textures> textures_;
    v3d::render::offline::sl::InstancePtr imager_;
    bool traces_ = false;
};

};  // namespace v3d::moya
