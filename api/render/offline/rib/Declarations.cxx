/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Declarations.h"

#include <cstdlib>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace v3d::render::offline::rib {

namespace {

bool storageWord(const std::string & word, Declaration::Storage * storage) {
    if (word == "constant") {
        *storage = Declaration::Storage::CONSTANT;
    } else if (word == "uniform") {
        *storage = Declaration::Storage::UNIFORM;
    } else if (word == "varying") {
        *storage = Declaration::Storage::VARYING;
    } else if (word == "vertex") {
        *storage = Declaration::Storage::VERTEX;
    } else {
        return false;
    }
    return true;
}

bool typeWord(const std::string & word, Declaration::Type * type) {
    if (word == "float") {
        *type = Declaration::Type::FLOAT;
    } else if (word == "integer" || word == "int") {
        *type = Declaration::Type::INTEGER;
    } else if (word == "string") {
        *type = Declaration::Type::STRING;
    } else if (word == "color" || word == "colour") {
        *type = Declaration::Type::COLOR;
    } else if (word == "point") {
        *type = Declaration::Type::POINT;
    } else if (word == "vector") {
        *type = Declaration::Type::VECTOR;
    } else if (word == "normal") {
        *type = Declaration::Type::NORMAL;
    } else if (word == "matrix") {
        *type = Declaration::Type::MATRIX;
    } else if (word == "hpoint") {
        *type = Declaration::Type::HPOINT;
    } else {
        return false;
    }
    return true;
}

std::vector<std::string> words(const std::string & text) {
    std::istringstream stream(text);
    std::vector<std::string> result;
    std::string word;
    while (stream >> word) {
        result.push_back(word);
    }
    return result;
}

/**
 * Split a trailing "[n]" off a type word.
 **/
unsigned int arrayCount(std::string * word) {
    const std::string::size_type open = word->find('[');
    if (open == std::string::npos || word->back() != ']') {
        return 1;
    }
    const std::string digits = word->substr(open + 1, word->size() - open - 2);
    word->erase(open);
    const long count = std::strtol(digits.c_str(), nullptr, 10);  // NOLINT(runtime/int)
    return count > 0 ? static_cast<unsigned int>(count) : 1;
}

};  // namespace

Declaration::Declaration() {
}

Declaration::Declaration(Storage storage, Type type, unsigned int count) :
    storage_(storage),
    type_(type),
    count_(count) {
}

bool Declaration::parse(const std::string & text, Declaration * declaration) {
    std::vector<std::string> parts = words(text);
    if (parts.empty()) {
        return false;
    }

    Storage storage = Storage::UNIFORM;
    std::size_t index = 0;
    if (storageWord(parts[0], &storage)) {
        index = 1;
    }
    if (index >= parts.size()) {
        return false;
    }

    std::string word = parts[index];
    const unsigned int count = arrayCount(&word);
    Type type = Type::FLOAT;
    if (!typeWord(word, &type)) {
        return false;
    }

    *declaration = Declaration(storage, type, count);
    return true;
}

Declaration::Storage Declaration::storage() const {
    return storage_;
}

Declaration::Type Declaration::type() const {
    return type_;
}

unsigned int Declaration::count() const {
    return count_;
}

unsigned int Declaration::floats() const {
    switch (type_) {
        case Type::STRING:
            return 0;
        case Type::COLOR:
        case Type::POINT:
        case Type::VECTOR:
        case Type::NORMAL:
            return 3;
        case Type::HPOINT:
            return 4;
        case Type::MATRIX:
            return 16;
        case Type::FLOAT:
        case Type::INTEGER:
        default:
            return 1;
    }
}

unsigned int Declaration::elements(unsigned int vertices) const {
    switch (storage_) {
        case Storage::VARYING:
        case Storage::VERTEX:
            return vertices;
        case Storage::CONSTANT:
        case Storage::UNIFORM:
        default:
            return 1;
    }
}

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

    // the identifier attributes, which is how a scene names an object
    declarations_["name"] = Declaration(Storage::UNIFORM, Type::STRING, 1);
    declarations_["shadinggroup"] = Declaration(Storage::UNIFORM, Type::STRING, 1);

    // the renderer options this tree's Option "limits" carries. The standard leaves
    // these implementation specific, and an undeclared one is a warning per read.
    declarations_["bucketsize"] = Declaration(Storage::UNIFORM, Type::INTEGER, 2);
    declarations_["gridsize"] = Declaration(Storage::UNIFORM, Type::INTEGER, 1);
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
