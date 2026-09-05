/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "RIBDeclarations.h"

#include <cstdlib>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace v3d::render::offline {

    namespace {

        bool storageWord(const std::string & word, RIBDeclaration::Storage * storage) {
            if (word == "constant") {
                *storage = RIBDeclaration::Storage::CONSTANT;
            } else if (word == "uniform") {
                *storage = RIBDeclaration::Storage::UNIFORM;
            } else if (word == "varying") {
                *storage = RIBDeclaration::Storage::VARYING;
            } else if (word == "vertex") {
                *storage = RIBDeclaration::Storage::VERTEX;
            } else {
                return false;
            }
            return true;
        }

        bool typeWord(const std::string & word, RIBDeclaration::Type * type) {
            if (word == "float") {
                *type = RIBDeclaration::Type::FLOAT;
            } else if (word == "integer" || word == "int") {
                *type = RIBDeclaration::Type::INTEGER;
            } else if (word == "string") {
                *type = RIBDeclaration::Type::STRING;
            } else if (word == "color" || word == "colour") {
                *type = RIBDeclaration::Type::COLOR;
            } else if (word == "point") {
                *type = RIBDeclaration::Type::POINT;
            } else if (word == "vector") {
                *type = RIBDeclaration::Type::VECTOR;
            } else if (word == "normal") {
                *type = RIBDeclaration::Type::NORMAL;
            } else if (word == "matrix") {
                *type = RIBDeclaration::Type::MATRIX;
            } else if (word == "hpoint") {
                *type = RIBDeclaration::Type::HPOINT;
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

    RIBDeclaration::RIBDeclaration() {
    }

    RIBDeclaration::RIBDeclaration(Storage storage, Type type, unsigned int count) :
        storage_(storage),
        type_(type),
        count_(count) {
    }

    bool RIBDeclaration::parse(const std::string & text, RIBDeclaration * declaration) {
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

        *declaration = RIBDeclaration(storage, type, count);
        return true;
    }

    RIBDeclaration::Storage RIBDeclaration::storage() const {
        return storage_;
    }

    RIBDeclaration::Type RIBDeclaration::type() const {
        return type_;
    }

    unsigned int RIBDeclaration::count() const {
        return count_;
    }

    unsigned int RIBDeclaration::floats() const {
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

    unsigned int RIBDeclaration::elements(unsigned int vertices) const {
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

    RIBDeclarations::RIBDeclarations() {
        typedef RIBDeclaration::Storage Storage;
        typedef RIBDeclaration::Type Type;

        // the standard geometric primitive variables, RI table 5.1
        declarations_["P"] = RIBDeclaration(Storage::VERTEX, Type::POINT, 1);
        declarations_["Pz"] = RIBDeclaration(Storage::VERTEX, Type::FLOAT, 1);
        declarations_["Pw"] = RIBDeclaration(Storage::VERTEX, Type::HPOINT, 1);
        declarations_["N"] = RIBDeclaration(Storage::VARYING, Type::NORMAL, 1);
        declarations_["Np"] = RIBDeclaration(Storage::UNIFORM, Type::NORMAL, 1);
        declarations_["Cs"] = RIBDeclaration(Storage::VARYING, Type::COLOR, 1);
        declarations_["Os"] = RIBDeclaration(Storage::VARYING, Type::COLOR, 1);
        declarations_["s"] = RIBDeclaration(Storage::VARYING, Type::FLOAT, 1);
        declarations_["t"] = RIBDeclaration(Storage::VARYING, Type::FLOAT, 1);
        declarations_["st"] = RIBDeclaration(Storage::VARYING, Type::FLOAT, 2);
        declarations_["width"] = RIBDeclaration(Storage::VARYING, Type::FLOAT, 1);
        declarations_["constantwidth"] = RIBDeclaration(Storage::CONSTANT, Type::FLOAT, 1);

        // the parameters of the standard shaders
        declarations_["Ka"] = RIBDeclaration(Storage::UNIFORM, Type::FLOAT, 1);
        declarations_["Kd"] = RIBDeclaration(Storage::UNIFORM, Type::FLOAT, 1);
        declarations_["Ks"] = RIBDeclaration(Storage::UNIFORM, Type::FLOAT, 1);
        declarations_["Kr"] = RIBDeclaration(Storage::UNIFORM, Type::FLOAT, 1);
        declarations_["roughness"] = RIBDeclaration(Storage::UNIFORM, Type::FLOAT, 1);
        declarations_["specularcolor"] = RIBDeclaration(Storage::UNIFORM, Type::COLOR, 1);
        declarations_["texturename"] = RIBDeclaration(Storage::UNIFORM, Type::STRING, 1);
        declarations_["intensity"] = RIBDeclaration(Storage::UNIFORM, Type::FLOAT, 1);
        declarations_["lightcolor"] = RIBDeclaration(Storage::UNIFORM, Type::COLOR, 1);
        declarations_["from"] = RIBDeclaration(Storage::UNIFORM, Type::POINT, 1);
        declarations_["to"] = RIBDeclaration(Storage::UNIFORM, Type::POINT, 1);
        declarations_["coneangle"] = RIBDeclaration(Storage::UNIFORM, Type::FLOAT, 1);
        declarations_["conedeltaangle"] = RIBDeclaration(Storage::UNIFORM, Type::FLOAT, 1);
        declarations_["beamdistribution"] = RIBDeclaration(Storage::UNIFORM, Type::FLOAT, 1);
        declarations_["mindistance"] = RIBDeclaration(Storage::UNIFORM, Type::FLOAT, 1);
        declarations_["maxdistance"] = RIBDeclaration(Storage::UNIFORM, Type::FLOAT, 1);
        declarations_["distance"] = RIBDeclaration(Storage::UNIFORM, Type::FLOAT, 1);
        declarations_["amplitude"] = RIBDeclaration(Storage::UNIFORM, Type::FLOAT, 1);
        declarations_["background"] = RIBDeclaration(Storage::UNIFORM, Type::COLOR, 1);
        declarations_["fov"] = RIBDeclaration(Storage::UNIFORM, Type::FLOAT, 1);

        // the identifier attributes, which is how a scene names an object
        declarations_["name"] = RIBDeclaration(Storage::UNIFORM, Type::STRING, 1);
        declarations_["shadinggroup"] = RIBDeclaration(Storage::UNIFORM, Type::STRING, 1);

        // the renderer options this tree's Option "limits" carries. The standard leaves
        // these implementation specific, and an undeclared one is a warning per read.
        declarations_["bucketsize"] = RIBDeclaration(Storage::UNIFORM, Type::INTEGER, 2);
        declarations_["gridsize"] = RIBDeclaration(Storage::UNIFORM, Type::INTEGER, 1);
    }

    bool RIBDeclarations::declare(const std::string & name, const std::string & text) {
        RIBDeclaration declaration;
        if (!RIBDeclaration::parse(text, &declaration)) {
            return false;
        }
        declarations_[name] = declaration;
        return true;
    }

    bool RIBDeclarations::resolve(const std::string & token, std::string * name, RIBDeclaration * declaration) const {
        const std::vector<std::string> parts = words(token);
        if (parts.size() > 1) {
            // the inline form: the last word is the name and the rest types it
            *name = parts.back();
            return RIBDeclaration::parse(token.substr(0, token.rfind(parts.back())), declaration);
        }

        *name = token;
        const std::map<std::string, RIBDeclaration>::const_iterator found = declarations_.find(token);
        if (found == declarations_.end()) {
            return false;
        }
        *declaration = found->second;
        return true;
    }

};  // namespace v3d::render::offline
