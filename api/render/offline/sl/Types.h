/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace v3d::render::offline::sl {

/**
 * A value's type. The three point-like ones are all three floats and differ only in how a
 * transform treats them - a point translates, a vector does not, and a normal goes by the
 * inverse transpose - which is the compiler's problem rather than the parser's.
 *
 * SL has no integer type: a count, an index and a colour component are all floats.
 **/
enum class Type {
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
 * Whether a value is stored once or once per shading point. UNSPECIFIED is what a
 * declaration that named neither carries; the varying inference in the compiler is what
 * turns it into one of the other two.
 **/
enum class Storage {
    UNSPECIFIED,
    UNIFORM,
    VARYING
};

/**
 * The five shader types. All five parse; surface, light and imager are the three this
 * phase executes.
 **/
enum class ShaderType {
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

/**
 * Whether the type is a position or a direction - point, vector or normal. The three are
 * all three floats and convert to each other freely in arithmetic; what tells them apart is
 * a transform.
 **/
bool pointlike(Type type);

/**
 * Whether a value of one type may be used where the other is wanted.
 *
 * A float promotes to any of the three-float types by replication, and to a matrix as that
 * multiple of the identity. The three point-like types convert to each other. A colour does
 * **not** convert to or from them: a colour that has drifted into a position is a mistake
 * worth catching, and there is `ctransform` for when it is not.
 **/
bool coercible(Type from, Type to);

/**
 * The type of an arithmetic expression over the two, or VOID when there is none - which is
 * what an operator applied to types that have no arithmetic between them answers.
 **/
Type arithmetic(Type left, Type right);

/**
 * A point through a matrix: rotated, scaled **and translated**, because a point has a
 * position.
 **/
glm::vec3 ptransform(const glm::mat4x4 & matrix, const glm::vec3 & point);

/**
 * A vector through a matrix: rotated and scaled, and **not** translated - a direction has
 * no position for a translation to move.
 **/
glm::vec3 vtransform(const glm::mat4x4 & matrix, const glm::vec3 & vector);

/**
 * A normal through a matrix, by the inverse transpose.
 *
 * The same rule as moya's dicing and talyn's fan: under a rotation or a uniform scale this
 * is what vtransform answers, and the moment a scene scales one axis it is not - the matrix
 * that moves the points tilts a normal off its surface.
 **/
glm::vec3 ntransform(const glm::mat4x4 & matrix, const glm::vec3 & normal);

};  // namespace v3d::render::offline::sl
