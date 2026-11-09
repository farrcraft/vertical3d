/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/scene/Handle.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

/**
 * The new core's retained scene: objects of declared kinds, their parameters, and the versions
 * that record each change. RIB, the editor and later a Hydra delegate are its clients.
 **/
namespace v3d::moya::scene {

/** The type of a parameter's elements. **/
enum class Type : std::uint8_t {
    Integer,
    Float,
    String,
    Colour,  /**< three floats in a colour space the value names **/
    Point,
    Vector,
    Normal,
    Matrix,  /**< sixteen floats, column by column, as glm holds them **/
    Handle   /**< another object of the scene **/
};

/** The name a description and an error message give a type. **/
const char * name(Type type);

/**
 * How many floats one element of a type holds: 1 for a float, 3 for a colour, a point, a vector
 * or a normal, and 16 for a matrix. Zero for a type whose elements are not floats.
 **/
std::size_t floatsOf(Type type);

/**
 * One value of a parameter: an array of elements of one type. A parameter declared to hold one
 * element holds an array of one.
 **/
class Value final {
 public:
    /** An empty array of integers. **/
    Value();

    static Value integers(const std::vector<std::int32_t> & values);
    static Value floats(const std::vector<float> & values);
    static Value strings(const std::vector<std::string> & values);

    /**
     * @param space the colour space the colours are in, as OpenColorIO names it. An empty name is
     *        the working space
     **/
    static Value colours(const std::vector<glm::vec3> & values, const std::string & space = "");
    static Value points(const std::vector<glm::vec3> & values);
    static Value vectors(const std::vector<glm::vec3> & values);
    static Value normals(const std::vector<glm::vec3> & values);
    static Value matrices(const std::vector<glm::mat4> & values);
    static Value handles(const std::vector<Handle> & values);

    /** One element of each type. **/
    static Value integer(std::int32_t value);
    static Value number(float value);
    static Value string(const std::string & value);
    static Value colour(const glm::vec3 & value, const std::string & space = "");
    static Value point(const glm::vec3 & value);
    static Value vector(const glm::vec3 & value);
    static Value normal(const glm::vec3 & value);
    static Value matrix(const glm::mat4 & value);
    static Value handle(const Handle & value);

    Type type() const noexcept;

    /** The number of elements. **/
    std::size_t size() const noexcept;

    /** The colour space of a colour value, empty for the working space or any other type. **/
    const std::string & space() const noexcept;

    /**
     * The elements of an integer, string or handle value, or the floats of any type that
     * floatsOf() counts, element after element.
     *
     * @throws std::logic_error when the value holds another type
     **/
    const std::vector<std::int32_t> & integerArray() const;
    const std::vector<float> & floatArray() const;
    const std::vector<std::string> & stringArray() const;
    const std::vector<Handle> & handleArray() const;

    /**
     * One element of a colour, point, vector or normal value, and of a matrix value.
     *
     * @throws std::logic_error when the value holds another type
     * @throws std::out_of_range when the value has no such element
     **/
    glm::vec3 triple(std::size_t element) const;
    glm::mat4 matrixAt(std::size_t element) const;

    bool operator==(const Value & other) const = default;

 private:
    Value(Type type, std::variant<std::vector<std::int32_t>, std::vector<float>, std::vector<std::string>, std::vector<Handle>> elements,
        std::string space = "");

    static Value triples(Type type, const std::vector<glm::vec3> & values, const std::string & space);

    Type type_ { Type::Integer };
    std::variant<std::vector<std::int32_t>, std::vector<float>, std::vector<std::string>, std::vector<Handle>> elements_;
    std::string space_;
};

};  // namespace v3d::moya::scene
