/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/scene/Description.h>

#include <moya/libmoya/scene/Declaration.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Kind.h>
#include <moya/libmoya/scene/Samples.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Schema.h>
#include <moya/libmoya/scene/Value.h>

#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <string>
#include <system_error>
#include <vector>

#include <glm/vec3.hpp>

namespace v3d::moya::scene {

namespace {

/** The floats in a matrix. **/
constexpr std::size_t kMatrixFloats = 16;

/**
 * The significant digits a float is written with. Two compilers' mathematics round a sine or an
 * inverse differently in the last bits a float holds, and six digits leave those bits out.
 **/
constexpr int kDigits = 6;

/** A float to kDigits significant digits, with a negative zero written as zero. **/
std::string number(float value) {
    std::array<char, 32> digits {};
    const float shown = value == 0.0F ? 0.0F : value;
    const std::to_chars_result written =
        std::to_chars(digits.data(), digits.data() + digits.size(), shown, std::chars_format::general, kDigits);
    if (written.ec != std::errc()) {
        return "nan";
    }
    return std::string(digits.data(), written.ptr);
}

std::string quoted(const std::string & text) {
    std::string result = "\"";
    for (const char letter : text) {
        if (letter == '"' || letter == '\\') {
            result += '\\';
        }
        result += letter;
    }
    return result + "\"";
}

/** A handle as its object's kind and name. **/
std::string handle(const Scene & scene, const Handle & named) {
    if (!scene.contains(named)) {
        return "none";
    }
    return scene.schema().kind(named.kind).name + " " + quoted(scene.name(named));
}

/** Each element of a value, after a space. **/
std::string elements(const Scene & scene, const Value & value) {
    std::string result;
    switch (value.type()) {
        case Type::Integer:
            for (const std::int32_t element : value.integerArray()) {
                result += " " + std::to_string(element);
            }
            break;
        case Type::Float:
            for (const float element : value.floatArray()) {
                result += " " + number(element);
            }
            break;
        case Type::String:
            for (const std::string & element : value.stringArray()) {
                result += " " + quoted(element);
            }
            break;
        case Type::Colour:
        case Type::Point:
        case Type::Vector:
        case Type::Normal:
            for (std::size_t i = 0; i < value.size(); ++i) {
                const glm::vec3 element = value.triple(i);
                result += " (" + number(element.x) + ", " + number(element.y) + ", " + number(element.z) + ")";
            }
            if (!value.space().empty()) {
                result += " in " + quoted(value.space());
            }
            break;
        case Type::Matrix: {
            const std::vector<float> & floats = value.floatArray();
            for (std::size_t first = 0; first < floats.size(); first += kMatrixFloats) {
                result += " [";
                for (std::size_t i = 0; i < kMatrixFloats; ++i) {
                    result += (i == 0 ? "" : " ") + number(floats[first + i]);
                }
                result += "]";
            }
            break;
        }
        case Type::Handle:
            for (const Handle & element : value.handleArray()) {
                result += " " + handle(scene, element);
            }
            break;
    }
    return result;
}

/** The declaration of a parameter, in its kind or among the object's primvars. **/
const Declaration * declarationOf(const Kind & kind, const std::vector<Declaration> & primvars, const std::string & parameter) {
    for (const Declaration & declared : kind.parameters) {
        if (declared.name == parameter) {
            return &declared;
        }
    }
    for (const Declaration & declared : primvars) {
        if (declared.name == parameter) {
            return &declared;
        }
    }
    return nullptr;
}

/** A parameter's line. **/
std::string parameterLine(const Scene & scene, const Declaration & declared, bool primvar, const Samples & samples) {
    std::string line = "    " + declared.name + " " + name(declared.type);
    if (declared.width != 1) {
        line += std::to_string(declared.width);
    }
    if (declared.count != 1) {
        line += "[" + std::to_string(samples.values.front().size() / declared.width) + "]";
    }
    if (primvar && declared.interpolation != Interpolation::Constant) {
        line += std::string(" ") + name(declared.interpolation);
    }
    if (samples.times.empty()) {
        line += " =" + elements(scene, samples.values.front());
    } else {
        for (std::size_t i = 0; i < samples.times.size(); ++i) {
            line += " @ " + number(samples.times[i]) + " =" + elements(scene, samples.values[i]);
        }
    }
    return line + "\n";
}

};  // namespace

std::string describe(const Scene & scene) {
    const Schema & schema = scene.schema();
    std::vector<std::uint32_t> kinds;
    kinds.reserve(schema.size());
    for (std::uint32_t i = 0; i < schema.size(); ++i) {
        kinds.push_back(i);
    }
    std::sort(kinds.begin(), kinds.end(), [&schema](std::uint32_t left, std::uint32_t right) {
        return schema.kind(left).name < schema.kind(right).name;
    });

    std::string text;
    for (const std::uint32_t index : kinds) {
        const Kind & kind = schema.kind(index);
        std::vector<Handle> objects = scene.objects(kind.name);
        std::sort(objects.begin(), objects.end(), [&scene](const Handle & left, const Handle & right) {
            return scene.name(left) < scene.name(right);
        });
        for (const Handle & object : objects) {
            text += kind.name + " " + quoted(scene.name(object)) + "\n";
            const std::vector<Declaration> & primvars = scene.primvars(object);
            std::vector<std::string> names;
            names.reserve(kind.parameters.size() + primvars.size());
            for (const Declaration & declared : kind.parameters) {
                names.push_back(declared.name);
            }
            for (const Declaration & declared : primvars) {
                names.push_back(declared.name);
            }
            std::sort(names.begin(), names.end());
            for (const std::string & parameter : names) {
                const Declaration * declared = declarationOf(kind, primvars, parameter);
                const bool primvar = std::none_of(kind.parameters.begin(), kind.parameters.end(),
                    [&parameter](const Declaration & own) { return own.name == parameter; });
                text += parameterLine(scene, *declared, primvar, scene.samples(object, parameter));
            }
        }
    }
    return text;
}

};  // namespace v3d::moya::scene
