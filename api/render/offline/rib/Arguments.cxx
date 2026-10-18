/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Arguments.h"

#include "Declaration.h"
#include "Declarations.h"
#include "Parameters.h"

#include <string>
#include <vector>

namespace v3d::render::offline::rib {

namespace {

/**
 * Append a value array of the C type the declaration names to the list's floats.
 **/
template <typename T>
void append(const void* values, unsigned int count, std::vector<float>* floats) {
    const T* numbers = static_cast<const T*>(values);
    for (unsigned int value = 0; value < count; value++) {
        floats->push_back(static_cast<float>(numbers[value]));
    }
}

};  // namespace

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
            // a string parameter's array is an array of pointers to characters, as RtString is
            const char* const* text = static_cast<const char* const*>(values[i]);
            for (unsigned int element = 0; element < elements; element++) {
                strings.push_back(text[element] == nullptr ? std::string() : text[element]);
            }
        } else if (declaration.type() == Declaration::Type::INTEGER) {
            // an integer parameter's array holds ints, as RtInt is, and read as floats it
            // would be their bit patterns
            append<int>(values[i], elements * declaration.floats(), &floats);
        } else {
            append<float>(values[i], elements * declaration.floats(), &floats);
        }
        list->add(name, declaration, floats, strings);
    }
}

};  // namespace v3d::render::offline::rib
