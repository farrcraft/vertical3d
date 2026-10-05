/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Declarations.h"

#include "Words.h"

#include <map>
#include <string>
#include <vector>

namespace v3d::render::offline::rib {

Declarations::Declarations() {
    typedef Declaration::Storage Storage;
    typedef Declaration::Type Type;

    // the standard geometric primitive variables, RI table 5.1
    declarations_["P"] = Declaration(Storage::VERTEX, Type::POINT, 1);
    declarations_["Pz"] = Declaration(Storage::VERTEX, Type::FLOAT, 1);
    declarations_["Pw"] = Declaration(Storage::VERTEX, Type::HPOINT, 1);
    declarations_["N"] = Declaration(Storage::VARYING, Type::NORMAL, 1);
    declarations_["Np"] = Declaration(Storage::UNIFORM, Type::NORMAL, 1);
    declarations_["Cs"] = Declaration(Storage::VARYING, Type::COLOR, 1);
    declarations_["Os"] = Declaration(Storage::VARYING, Type::COLOR, 1);
    declarations_["s"] = Declaration(Storage::VARYING, Type::FLOAT, 1);
    declarations_["t"] = Declaration(Storage::VARYING, Type::FLOAT, 1);
    declarations_["st"] = Declaration(Storage::VARYING, Type::FLOAT, 2);
    declarations_["width"] = Declaration(Storage::VARYING, Type::FLOAT, 1);
    declarations_["constantwidth"] = Declaration(Storage::CONSTANT, Type::FLOAT, 1);

    // the parameters of the standard shaders
    declarations_["Ka"] = Declaration(Storage::UNIFORM, Type::FLOAT, 1);
    declarations_["Kd"] = Declaration(Storage::UNIFORM, Type::FLOAT, 1);
    declarations_["Ks"] = Declaration(Storage::UNIFORM, Type::FLOAT, 1);
    declarations_["Kr"] = Declaration(Storage::UNIFORM, Type::FLOAT, 1);
    declarations_["roughness"] = Declaration(Storage::UNIFORM, Type::FLOAT, 1);
    declarations_["specularcolor"] = Declaration(Storage::UNIFORM, Type::COLOR, 1);
    declarations_["texturename"] = Declaration(Storage::UNIFORM, Type::STRING, 1);
    declarations_["intensity"] = Declaration(Storage::UNIFORM, Type::FLOAT, 1);
    declarations_["lightcolor"] = Declaration(Storage::UNIFORM, Type::COLOR, 1);
    declarations_["from"] = Declaration(Storage::UNIFORM, Type::POINT, 1);
    declarations_["to"] = Declaration(Storage::UNIFORM, Type::POINT, 1);
    declarations_["coneangle"] = Declaration(Storage::UNIFORM, Type::FLOAT, 1);
    declarations_["conedeltaangle"] = Declaration(Storage::UNIFORM, Type::FLOAT, 1);
    declarations_["beamdistribution"] = Declaration(Storage::UNIFORM, Type::FLOAT, 1);
    declarations_["mindistance"] = Declaration(Storage::UNIFORM, Type::FLOAT, 1);
    declarations_["maxdistance"] = Declaration(Storage::UNIFORM, Type::FLOAT, 1);
    declarations_["distance"] = Declaration(Storage::UNIFORM, Type::FLOAT, 1);
    declarations_["amplitude"] = Declaration(Storage::UNIFORM, Type::FLOAT, 1);
    declarations_["background"] = Declaration(Storage::UNIFORM, Type::COLOR, 1);
    declarations_["fov"] = Declaration(Storage::UNIFORM, Type::FLOAT, 1);

    // the identifier attributes, which a scene uses to name an object
    declarations_["name"] = Declaration(Storage::UNIFORM, Type::STRING, 1);
    declarations_["shadinggroup"] = Declaration(Storage::UNIFORM, Type::STRING, 1);

    // the renderer options this tree's Option "limits" carries. The standard leaves
    // these implementation specific, and an undeclared one is a warning per read.
    declarations_["bucketsize"] = Declaration(Storage::UNIFORM, Type::INTEGER, 2);
    declarations_["gridsize"] = Declaration(Storage::UNIFORM, Type::INTEGER, 1);
    // the parameters of Option "trace" and Option "searchpath", whose names the standard defines
    declarations_["maxdepth"] = Declaration(Storage::UNIFORM, Type::INTEGER, 1);
    declarations_["shader"] = Declaration(Storage::UNIFORM, Type::STRING, 1);
    declarations_["texture"] = Declaration(Storage::UNIFORM, Type::STRING, 1);
}

bool Declarations::declare(const std::string & name, const std::string & text) {
    Declaration declaration;
    if (!Declaration::parse(text, &declaration)) {
        return false;
    }
    declarations_[name] = declaration;
    return true;
}

bool Declarations::resolve(const std::string & token, std::string * name, Declaration * declaration) const {
    const std::vector<std::string> parts = words(token);
    if (parts.size() > 1) {
        // the inline form: the last word is the name and the rest types it
        *name = parts.back();
        return Declaration::parse(token.substr(0, token.rfind(parts.back())), declaration);
    }

    *name = token;
    const std::map<std::string, Declaration>::const_iterator found = declarations_.find(token);
    if (found == declarations_.end()) {
        return false;
    }
    *declaration = found->second;
    return true;
}

};  // namespace v3d::render::offline::rib
