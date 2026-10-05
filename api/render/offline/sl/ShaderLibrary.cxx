/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "ShaderLibrary.h"

#include <api/render/offline/SearchPath.h>
#include <api/render/offline/sl/syntax/Shader.h>

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>

#include "Compiler.h"
#include "Emitter.h"
#include "Parser.h"

namespace v3d::render::offline::sl {

namespace {

/*
    The standard shaders, compiled into the library as source strings so that
    Surface "matte" works with no shader files at all.

    They are RI's own, written in this implementation's interpretation of the language: L
    points from the point being shaded toward the light, so a spotlight tests its cone
    against -L, the direction the light travels.

    Each of the three directional lights calls transmission() for how much of its light
    arrives, which is how a shadow is cast. A renderer with no ray tracer returns full
    transmission, so its lights cast no shadows.

    shinymetal is RI's with trace() where RI reads an environment map. glass is not one of
    RI's, since RI defines no refracting shader. glass is opaque because it shows what is
    behind it by tracing a refracted ray rather than by letting a ray through. It flips its
    normal and its ratio of indices when the ray is leaving it.
*/
const char* const STANDARD = R"(
surface constant() {
    Oi = Os;
    Ci = Os * Cs;
}

surface matte(float Ka = 1; float Kd = 1) {
    normal Nf = faceforward(normalize(N), I);
    Oi = Os;
    Ci = Os * Cs * (Ka * ambient() + Kd * diffuse(Nf));
}

surface metal(float Ka = 1; float Ks = 1; float roughness = 0.1) {
    normal Nf = faceforward(normalize(N), I);
    vector V = -normalize(I);
    Oi = Os;
    Ci = Os * Cs * (Ka * ambient() + Ks * specular(Nf, V, roughness));
}

surface plastic(float Ka = 1; float Kd = 0.5; float Ks = 0.5; float roughness = 0.1;
        color specularcolor = 1) {
    normal Nf = faceforward(normalize(N), I);
    vector V = -normalize(I);
    Oi = Os;
    Ci = Os * (Cs * (Ka * ambient() + Kd * diffuse(Nf)) +
        specularcolor * Ks * specular(Nf, V, roughness));
}

surface paintedplastic(float Ka = 1; float Kd = 0.5; float Ks = 0.5; float roughness = 0.1;
        color specularcolor = 1; string texturename = "") {
    normal Nf = faceforward(normalize(N), I);
    vector V = -normalize(I);
    Oi = Os;
    Ci = Cs;
    if (texturename != "") {
        Ci *= color texture(texturename);
    }
    Ci = Os * (Ci * (Ka * ambient() + Kd * diffuse(Nf)) +
        specularcolor * Ks * specular(Nf, V, roughness));
}

surface shinymetal(float Ka = 1; float Ks = 1; float Kr = 1; float roughness = 0.1) {
    normal Nf = faceforward(normalize(N), I);
    vector V = -normalize(I);
    Oi = Os;
    Ci = Os * Cs * (Ka * ambient() + Ks * specular(Nf, V, roughness) +
        Kr * trace(P, reflect(I, Nf)));
}

surface glass(float Ka = 0; float Ks = 0.5; float Kr = 1; float Kt = 1; float roughness = 0.05;
        float eta = 1.5) {
    normal Nn = normalize(N);
    vector In = normalize(I);
    normal Nf = Nn;
    float ratio = 1 / eta;
    if (In . Nn > 0) {
        Nf = -Nn;
        ratio = eta;
    }
    float kr = 0;
    float kt = 0;
    vector R = 0;
    vector T = 0;
    fresnel(In, Nf, ratio, kr, kt, R, T);
    Oi = 1;
    Ci = Ka * Cs * ambient() + Ks * specular(Nf, -In, roughness) +
        Kr * kr * trace(P, R) + Kt * kt * Cs * trace(P, T);
}

light ambientlight(float intensity = 1; color lightcolor = 1) {
    Cl = intensity * lightcolor;
}

light distantlight(float intensity = 1; color lightcolor = 1;
        point from = point "shader" (0, 0, 0); point to = point "shader" (0, 0, 1)) {
    solar(to - from, 0) {
        /*
            A light at infinity has no position for a shadow ray to end at, so the ray runs
            a long way back along L, which points at the light. The right length depends on
            the scene's size, and this is a constant: a scene larger than it is shadowed
            wrongly. The tracer does not accept a ray with no end.
        */
        Cl = intensity * lightcolor * transmission(Ps, Ps + L * 100000);
    }
}

light pointlight(float intensity = 1; color lightcolor = 1;
        point from = point "shader" (0, 0, 0)) {
    illuminate(from) {
        // L points at the light, so Ps + L is where it is: the shadow ray ends there
        // rather than going past it, and a surface behind the light does not block it
        Cl = intensity * lightcolor * transmission(Ps, Ps + L) / (L . L);
    }
}

light spotlight(float intensity = 1; color lightcolor = 1;
        point from = point "shader" (0, 0, 0); point to = point "shader" (0, 0, 1);
        float coneangle = 0.5235988; float conedeltaangle = 0.0698132;
        float beamdistribution = 2) {
    vector A = normalize(to - from);
    illuminate(from, A, coneangle) {
        float cosangle = ((-L) . A) / length(L);
        float atten = pow(cosangle, beamdistribution) / (L . L);
        atten *= smoothstep(cos(coneangle), cos(coneangle - conedeltaangle), cosangle);
        Cl = atten * intensity * lightcolor * transmission(Ps, Ps + L);
    }
}

imager background(color background = 0) {
    Ci = Ci + (1 - alpha) * background;
    alpha = 1;
}
)";

/**
 * The shader of that name out of a source, or null when it holds none.
 *
 * A file found on the search path is allowed to name its shader something other than the
 * file: RI identifies a shader by the name in its source, and a file holding exactly one
 * is unambiguous whatever it is called.
 **/
syntax::ShaderPtr find(const std::vector<syntax::ShaderPtr> & shaders, const std::string & name) {
    for (const syntax::ShaderPtr & shader : shaders) {
        if (shader->name == name) {
            return shader;
        }
    }
    return shaders.size() == 1 ? shaders[0] : syntax::ShaderPtr();
}

};  // namespace

ShaderLibrary::ShaderLibrary(const boost::shared_ptr<v3d::log::Logger> & logger) :
    logger_(logger) {
}

void ShaderLibrary::searchpath(const std::string & path) {
    directories_ = offline::searchpath(path, directories_);
}

std::string ShaderLibrary::file(const std::string & name, std::string* where) const {
    for (const std::string & directory : directories_) {
        std::string path = directory;
        path += "/";
        path += name;
        path += ".sl";
        std::ifstream stream(path.c_str());
        if (!stream.is_open()) {
            continue;
        }
        std::ostringstream source;
        source << stream.rdbuf();
        *where = path;
        return source.str();
    }
    return std::string();
}

ProgramPtr ShaderLibrary::compile(const std::string & name, const std::string & source,
    const std::string & where) {
    std::istringstream stream(source);
    Parser parser(stream);
    const syntax::ShaderPtr shader = find(parser.parse(), name);
    if (!shader) {
        // a source that would not parse and a source that holds no shader of that name are
        // different failures, and the message must say which one happened
        if (parser.error().empty()) {
            logger_->get()->error("there is no shader called '{}' in {}", name, where);
        } else {
            logger_->get()->error("the shader '{}' in {} does not parse - {}",
                name, where, parser.error());
        }
        return ProgramPtr();
    }
    Compiler compiler(shader);
    if (!compiler.compile()) {
        logger_->get()->error("the shader '{}' in {} does not compile - {}",
            name, where, compiler.error());
        return ProgramPtr();
    }
    ProgramPtr program = boost::make_shared<runtime::Program>();
    Emitter emitter(shader, compiler.symbols());
    if (!emitter.emit(program.get())) {
        logger_->get()->error("the shader '{}' in {} does not compile - {}",
            name, where, emitter.error());
        return ProgramPtr();
    }
    return program;
}

ProgramPtr ShaderLibrary::program(const std::string & name) {
    const auto held = programs_.find(name);
    if (held != programs_.end()) {
        // a failure is cached too
        return held->second;
    }
    std::string where;
    std::string source = file(name, &where);
    if (source.empty()) {
        // a file on the search path takes precedence over a built-in of the same name
        where = "the standard shaders";
        source = STANDARD;
    }
    ProgramPtr compiled = compile(name, source, where);
    programs_[name] = compiled;
    return compiled;
}

InstancePtr ShaderLibrary::fallback(ShaderType wanted) {
    if (wanted != ShaderType::SURFACE) {
        // RI requires a default surface and says nothing about a default light: a light
        // that will not compile is one fewer light rather than a light of some other kind
        return InstancePtr();
    }
    const ProgramPtr matte = program("matte");
    if (!matte) {
        return InstancePtr();
    }
    return boost::make_shared<Instance>(matte, logger_);
}

InstancePtr ShaderLibrary::instance(const std::string & name, ShaderType wanted,
    const rib::ParameterList & parameters) {
    const ProgramPtr found = program(name);
    if (!found) {
        // already reported as an error, by name and position
        return fallback(wanted);
    }
    if (found->type != wanted) {
        logger_->get()->error("'{}' is a {} shader, and the scene named it as a {}",
            name, sl::name(found->type), sl::name(wanted));
        return fallback(wanted);
    }
    InstancePtr made = boost::make_shared<Instance>(found, logger_);
    made->bind(parameters);
    return made;
}

};  // namespace v3d::render::offline::sl
