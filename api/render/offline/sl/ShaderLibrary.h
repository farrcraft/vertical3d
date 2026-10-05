/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>
#include <api/render/offline/rib/Parameters.h>
#include <api/render/offline/sl/Instance.h>

#include <map>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::render::offline::sl {

/**
 * Resolves the shader a scene names, as in `Surface "plastic"`. A name is compiled on first
 * use and kept.
 *
 * **The standard shaders are source strings compiled into the library**, so `Surface "matte"`
 * works with no shader files at all. A renderer's test suite is then hermetic, and a first
 * render needs nothing installed. `Option "searchpath" "shader"` adds directories for
 * everything else. A `.sl` file found there takes precedence over a built-in of the same
 * name, so a scene can replace one.
 *
 * **A shader that will not compile is reported and substituted rather than fatal.** RI
 * requires a renderer to carry on. The report names the shader and the position and is
 * logged as an error, because a scene whose shader failed and a scene that named no shader
 * must not look the same from outside.
 **/
class ShaderLibrary final {
 public:
    explicit ShaderLibrary(const boost::shared_ptr<v3d::log::Logger> & logger);

    /**
     * Where a `.sl` file that is not built in is looked for. RI writes this as
     * `Option "searchpath" "shader" ["./shaders:&"]`, colon separated, where `&` is
     * whatever the path was before.
     **/
    void searchpath(const std::string & path);

    /**
     * An instance of the named shader with the scene's parameters bound onto it, or the
     * substitute when the name will not compile.
     *
     * @param wanted what a shader of this type is expected to be; a name that turns out to
     *        be a shader of another type is reported and substituted too
     **/
    InstancePtr instance(const std::string & name, ShaderType wanted,
        const rib::ParameterList & parameters);

    /**
     * What a surface with no shader is drawn with, and what a shader that failed is
     * replaced by. RI leaves the choice to the renderer and forbids only "null".
     **/
    InstancePtr fallback(ShaderType wanted);

 private:
    /**
     * The program for a name, compiling it the first time it is requested. Null when it
     * will not compile, which is cached too - a scene naming a broken shader on a thousand
     * primitives is one report and one attempt.
     **/
    ProgramPtr program(const std::string & name);
    ProgramPtr compile(const std::string & name, const std::string & source,
        const std::string & where);
    /** The source of a `.sl` file on the search path, or empty when there is none. **/
    std::string file(const std::string & name, std::string* where) const;

    boost::shared_ptr<v3d::log::Logger> logger_;
    std::vector<std::string> directories_;
    std::map<std::string, ProgramPtr> programs_;
};

};  // namespace v3d::render::offline::sl
