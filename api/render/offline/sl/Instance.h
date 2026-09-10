/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>
#include <api/render/offline/rib/Parameters.h>
#include <api/render/offline/sl/Types.h>
#include <api/render/offline/sl/runtime/Machine.h>
#include <api/render/offline/sl/runtime/Program.h>

#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::render::offline::sl {

typedef boost::shared_ptr<runtime::Program> ProgramPtr;

/**
 * One compiled program with the values a scene bound onto it: what `Surface "plastic" "Ks"
 * [0.8]` makes.
 *
 * A shader **instance** rather than a shader, and named for it, because `sl::Shader` is
 * already the syntax node a file parses to. The distinction is the one the step turns on:
 * a program is compiled once per name and instanced once per request.
 *
 * The program is shared and the bindings are not. Two primitives shading with `plastic` at
 * different roughnesses are two of these over one program, which is what makes compiling
 * per name the right shape.
 *
 * An instance holds no register file. `write()` puts the bound values into a machine a
 * renderer has prepared, because how many machines there are and how long they live is the
 * renderer's question - moya reuses one across a thousand grids and talyn shades one hit.
 **/
class Instance final {
 public:
    Instance(const ProgramPtr & program, const boost::shared_ptr<v3d::log::Logger> & logger);

    const runtime::Program & program() const;
    const std::string & name() const;
    ShaderType type() const;

    /**
     * Whether a light shader lights every point without a direction - one using neither
     * `illuminate` nor `solar`.
     *
     * That is what keeps it out of an illuminance loop and inside `ambient()`, and the
     * program says it so that a renderer does not read the source again to find out.
     **/
    bool ambient() const;

    /**
     * Take the values a scene named, over the declared defaults.
     *
     * A name the shader does not declare is reported and dropped, because a renderer must
     * accept a request carrying a parameter it does not support. A value that will not
     * coerce is reported rather than reinterpreted: a colour bound onto a float is a scene
     * saying something the shader has no reading for, and guessing one is how a picture
     * comes out wrong quietly.
     **/
    void bind(const rib::ParameterList & parameters);

    /**
     * Write the bound values into a machine already prepared for this program. Called
     * after every `prepare`, since sizing the register file empties it.
     **/
    void write(runtime::Machine* machine) const;

 private:
    /**
     * One parameter's value as the machine wants it: the register it goes in, and the
     * floats or the string it holds.
     **/
    class Binding final {
     public:
        std::string name;
        int reg = -1;
        Type type = Type::FLOAT;
        std::vector<float> values;
        std::string text;
    };

    /**
     * Run the program's prologue and read each parameter out of it, which is how a default
     * written as an expression - `point "shader" (0, 0, 1)` - becomes a value.
     **/
    void defaults();
    Binding* binding(const std::string & wanted);

    ProgramPtr program_;
    boost::shared_ptr<v3d::log::Logger> logger_;
    std::vector<Binding> bindings_;
    bool ambient_ = false;
};

typedef boost::shared_ptr<Instance> InstancePtr;

};  // namespace v3d::render::offline::sl
