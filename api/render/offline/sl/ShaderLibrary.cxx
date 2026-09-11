/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "ShaderLibrary.h"

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
    The standard shaders, compiled into the library as source strings per ADR-0026, so that
    Surface "matte" works against no files at all.

    They are RI's own, written in this tree's reading of the language: L points from the
    point being shaded toward the light, so a spotlight tests its cone against -L, which is
    the way the light travels.

    Each of the three directional ones asks transmission() how much of its light arrives,
    which is where a shadow lives. A renderer that cannot answer lets all of it through, so
    this is the whole of the difference between a renderer that casts shadows and one that
    does not - moya draws exactly what it drew before and talyn traces.
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

light ambientlight(float intensity = 1; color lightcolor = 1) {
    Cl = intensity * lightcolor;
}

light distantlight(float intensity = 1; color lightcolor = 1;
        point from = point "shader" (0, 0, 0); point to = point "shader" (0, 0, 1)) {
    solar(to - from, 0) {
        /*
            A light at infinity has no position for a shadow ray to end at, so the ray runs
            a long way back along L, which points at the light. Far enough is a scene sized
            question and this answer is a constant: a scene larger than this shadows itself
            wrongly, and the alternative is a ray with no end, which the tracer has no
            reading for.
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
ShaderPtr find(const std::vector<ShaderPtr> & shaders, const std::string & name) {
    for (const ShaderPtr & shader : shaders) {
        if (shader->name == name) {
            return shader;
        }
    }
    return shaders.size() == 1 ? shaders[0] : ShaderPtr();
}

/**
 * The directories a search path names.
 *
 * RI separates them with a colon, which is also what a Windows drive letter is followed
 * by, so a lone letter before one does not end a directory.
 **/
std::vector<std::string> split(const std::string & path) {
    std::vector<std::string> found;
    std::string current;
    for (std::size_t i = 0; i < path.size(); i++) {
        const bool drive = path[i] == ':' && current.size() == 1 && std::isalpha(current[0]) != 0;
        if (path[i] != ':' || drive) {
            current += path[i];
            continue;
        }
        if (!current.empty()) {
            found.push_back(current);
        }
        current.clear();
    }
    if (!current.empty()) {
        found.push_back(current);
    }
    return found;
}

};  // namespace

ShaderLibrary::ShaderLibrary(const boost::shared_ptr<v3d::log::Logger> & logger) :
    logger_(logger) {
}

void ShaderLibrary::searchpath(const std::string & path) {
    std::vector<std::string> next;
    for (const std::string & directory : split(path)) {
        if (directory != "&") {
            next.push_back(directory);
            continue;
        }
        // '&' is whatever the path was before, which is how a scene appends to it rather
        // than replacing what a driver put there
        for (const std::string & held : directories_) {
            next.push_back(held);
        }
    }
    directories_ = next;
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
    const ShaderPtr shader = find(parser.parse(), name);
    if (!shader) {
        // a source that would not parse and a source that simply holds no shader of that
        // name are different things, and a message that reads as the other one wastes time
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
        // a failure is cached too: a scene naming a broken shader on a thousand primitives
        // is one attempt and one report
        return held->second;
    }
    std::string where;
    std::string source = file(name, &where);
    if (source.empty()) {
        // a file on the search path wins over a built-in of the same name, which is how a
        // scene replaces one
        where = "the standard shaders";
        source = STANDARD;
    }
    ProgramPtr compiled = compile(name, source, where);
    programs_[name] = compiled;
    return compiled;
}

InstancePtr ShaderLibrary::fallback(ShaderType wanted) {
    if (wanted != ShaderType::SURFACE) {
        // RI asks for a default surface and says nothing about a default light: a light
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
        // already reported by name and position, and loud: a scene whose shader failed and
        // a scene that named no shader must not look the same from outside
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
