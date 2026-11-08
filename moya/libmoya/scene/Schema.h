/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/scene/Value.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace v3d::moya::scene {

/**
 * How a primvar's elements spread over a geometry. RI and USD name the same five classes. A
 * constant primvar has one element for the whole geometry, and a uniform one has one per face.
 * A varying one has one per vertex, interpolated linearly, and a vertex one has one per vertex,
 * interpolated by the surface's basis. A face-varying one has one per corner of each face.
 **/
enum class Interpolation : std::uint8_t {
    Constant,
    Uniform,
    Varying,
    Vertex,
    FaceVarying
};

/** The name a description and an error message give an interpolation class. **/
const char * name(Interpolation interpolation);

/**
 * One parameter of a kind: its name, its type, how many elements it holds, whether it holds
 * samples over the shutter, and the value it has until a client sets it.
 **/
struct Declaration {
    std::string name;
    Type type { Type::Float };

    /**
     * How many elements its value holds: one for a parameter that is not an array, a count for an
     * array of that many, or kAny for an array of any length.
     **/
    std::uint32_t count { 1 };

    /** Whether it may hold several samples, each at a time of its own. **/
    bool motion { false };

    /**
     * Its value until a client sets one. It has the declared type, and holds as many elements as
     * the count says. An array of any length may have an empty fallback.
     **/
    Value fallback;

    /** The strings a string parameter may hold. Empty for any string. **/
    std::vector<std::string> choices;

    /** The kind of object a handle parameter names. Empty for any other type. **/
    std::string target;

    /** A primvar's interpolation class. An ordinary parameter is constant. **/
    Interpolation interpolation { Interpolation::Constant };

    /** The count of an array of any length. **/
    static constexpr std::uint32_t kAny = 0;

    /**
     * Check a value's size and, for a string, its choices.
     *
     * @throws std::invalid_argument when the value breaks them
     **/
    void admit(const Value & value) const;
};

/**
 * A kind of scene object, such as a camera or a mesh: its name and its declared parameters. A
 * kind that holds primvars takes a declaration of each one from its client, per object.
 **/
struct Kind {
    std::string name;
    std::vector<Declaration> parameters;
    bool primvars { false };
};

/**
 * The kinds a scene's objects can be. A kind is reached by its index, which a handle holds.
 **/
class Schema final {
 public:
    /**
     * Add a kind, after checking each declaration.
     *
     * @return its index
     * @throws std::invalid_argument when the kind or a parameter has no name, or two share a
     *         name. Also when a fallback's type, count or string breaks its declaration, a
     *         parameter that is not a string has choices, or a handle names no kind added before
     **/
    std::uint32_t add(const Kind & kind);

    /**
     * @throws std::out_of_range when there is no such kind
     **/
    const Kind & kind(std::uint32_t index) const;

    /** The index of a kind by its name. **/
    std::optional<std::uint32_t> find(const std::string & name) const;

    std::uint32_t size() const noexcept;

    /**
     * Check a declaration as add() checks each of a kind's, and a primvar's declaration as a
     * client gives it.
     *
     * @throws std::invalid_argument as add() does
     **/
    void check(const Declaration & declaration) const;

 private:
    std::vector<Kind> kinds_;
};

};  // namespace v3d::moya::scene
