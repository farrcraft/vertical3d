/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Declaration.h"

#include "Words.h"

#include <cstdlib>
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
            return 1;
    }
    return 1;
}

unsigned int Declaration::elements(unsigned int vertices) const {
    switch (storage_) {
        case Storage::VARYING:
        case Storage::VERTEX:
            return vertices;
        case Storage::CONSTANT:
        case Storage::UNIFORM:
            return 1;
    }
    return 1;
}

};  // namespace v3d::render::offline::rib
