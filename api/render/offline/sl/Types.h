/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace v3d::render::offline::sl {

/**
 * A value's type. The three point-like ones are all three floats and differ only in how a
 * transform treats them: a point translates, a vector does not, and a normal goes by the
 * inverse transpose. The compiler handles the difference, not the parser.
 *
 * SL has no integer type: a count, an index and a colour component are all floats.
 **/
enum class Type : std::uint8_t {
    VOID,
    FLOAT,
    POINT,
    VECTOR,
    NORMAL,
    COLOR,
    MATRIX,
    STRING
};

/**
 * Whether a value is stored once or once per shading point. A declaration that names
 * neither carries UNSPECIFIED, and the compiler's varying inference turns it into one of
 * the other two.
 **/
enum class Storage : std::uint8_t {
    UNSPECIFIED,
    UNIFORM,
    VARYING
};

/**
 * The shader types. Every type parses, but only surface, light and imager shaders run.
 **/
enum class ShaderType : std::uint8_t {
    SURFACE,
    LIGHT,
    DISPLACEMENT,
    VOLUME,
    IMAGER
};

/**
 * The name a type is written with, for a diagnostic.
 **/
const char* name(Type type);
const char* name(ShaderType type);

/**
 * What a type is made of, in floats. A string has none: it names a coordinate space, a
 * texture or a message, and there is no arithmetic to do on it.
 **/
unsigned int components(Type type);

/** How far apart the diagonal components of a matrix are, among its sixteen. **/
constexpr unsigned int MATRIX_DIAGONAL_STRIDE = 5;

/**
 * Whether the type is a point, vector or normal. All three are three floats and convert to
 * each other freely in arithmetic. Only a transform treats them differently.
 **/
bool pointlike(Type type);

/**
 * Whether a value of one type may be used where the other is wanted.
 *
 * A float promotes to any of the three-float types by replication, and to a matrix as that
 * multiple of the identity. The three point-like types convert to each other. A colour does
 * **not** convert to or from them, because a colour used as a position is almost always a
 * mistake. A shader that means it builds one from the other's components.
 **/
bool coercible(Type from, Type to);

/**
 * The type of an arithmetic expression over the two, or VOID when the two types have no
 * arithmetic between them.
 **/
Type arithmetic(Type left, Type right);

/**
 * A point through a matrix: rotated, scaled **and translated**, because a point has a
 * position.
 **/
glm::vec3 ptransform(const glm::mat4x4 & matrix, const glm::vec3 & point);

/**
 * A vector through a matrix: rotated and scaled, and **not** translated, because a
 * direction has no position.
 **/
glm::vec3 vtransform(const glm::mat4x4 & matrix, const glm::vec3 & vector);

/**
 * A normal through a matrix, by the inverse transpose.
 *
 * moya's dicing and its ray-traced geometry use the same rule. Under a rotation or a
 * uniform scale the result matches vtransform. Under a non-uniform scale it does not,
 * because the matrix that moves the points would tilt a normal off its surface.
 **/
glm::vec3 ntransform(const glm::mat4x4 & matrix, const glm::vec3 & normal);

};  // namespace v3d::render::offline::sl
