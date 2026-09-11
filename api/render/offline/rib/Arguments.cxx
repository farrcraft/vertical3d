/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Arguments.h"

#include <string>
#include <vector>

namespace v3d::render::offline::rib {

void arguments(const Declarations & declarations, int count,
    const char* const* tokens, const void* const* values, unsigned int vertices,
    ParameterList* list, std::vector<std::string>* unresolved) {
    if (tokens == nullptr || values == nullptr || list == nullptr) {
        return;
    }
    for (int i = 0; i < count; i++) {
        if (tokens[i] == nullptr) {
            continue;
        }
        std::string name;
        Declaration declaration;
        if (!declarations.resolve(tokens[i], &name, &declaration)) {
            // without a declaration there is no length, so this parameter cannot be read
            // and neither can anything be said about what the caller meant by it
            if (unresolved != nullptr) {
                unresolved->push_back(name);
            }
            continue;
        }
        if (values[i] == nullptr) {
            continue;
        }

        const unsigned int elements = declaration.elements(vertices) * declaration.count();
        std::vector<float> floats;
        std::vector<std::string> strings;
        if (declaration.type() == Declaration::Type::STRING) {
            // a string parameter's array is an array of pointers to characters, which is
            // what RtString is
            const char* const* text = static_cast<const char* const*>(values[i]);
            for (unsigned int element = 0; element < elements; element++) {
                strings.push_back(text[element] == nullptr ? std::string() : text[element]);
            }
        } else {
            const float* numbers = static_cast<const float*>(values[i]);
            const unsigned int wanted = elements * declaration.floats();
            for (unsigned int value = 0; value < wanted; value++) {
                floats.push_back(numbers[value]);
            }
        }
        list->add(name, declaration, floats, strings);
    }
}

};  // namespace v3d::render::offline::rib
