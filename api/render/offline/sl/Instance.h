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

#include <glm/mat4x4.hpp>

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
     * Put the parameters into a machine already prepared for this program: the declared
     * defaults first, then whatever a scene bound over them. Called after every `prepare`,
     * since sizing the register file empties it.
     *
     * The defaults are **run** rather than remembered, because a default may name a
     * coordinate space - `point "shader" (0, 0, 1)` is how three of the four standard
     * lights aim themselves - and what a space comes to is the renderer's answer, which is
     * not known until the machine has one attached.
     *
     * @param placement the shader's own space to the machine's current one, which is the
     *        transform that was in force when the scene instanced this shader. A position
     *        a scene binds is stated in that space, and arrives in this one.
     **/
    void write(runtime::Machine* machine,
        const glm::mat4x4 & placement = glm::mat4x4(1.0f)) const;

 private:
    /**
     * One parameter of the shader, and the value a scene bound onto it if one did.
     *
     * A parameter nothing bound is not written at all: the prologue has already left the
     * register holding what the shader declared.
     **/
    class Binding final {
     public:
        std::string name;
        int reg = -1;
        Type type = Type::FLOAT;
        bool bound = false;
        std::vector<float> values;
        std::string text;
    };

    Binding* binding(const std::string & wanted);

    ProgramPtr program_;
    boost::shared_ptr<v3d::log::Logger> logger_;
    std::vector<Binding> bindings_;
    bool ambient_ = false;
};

typedef boost::shared_ptr<Instance> InstancePtr;

};  // namespace v3d::render::offline::sl
