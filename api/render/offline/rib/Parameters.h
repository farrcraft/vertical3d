/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <vector>

#include "Declarations.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace v3d::render::offline::rib {

/**
 * The token-value pairs of one request, typed by the declarations that were in force.
 *
 * Parameters are typed once, in the reader, so neither renderer re-derives how many
 * floats "Cs" is. A name that is not here reads as empty or as the caller's fallback
 * rather than throwing: a renderer must accept a request carrying a parameter it does
 * not support.
 **/
class ParameterList final {
 public:
    void add(const std::string & name, const Declaration & declaration,
        const std::vector<float> & values, const std::vector<std::string> & strings);

    bool has(const std::string & name) const;
    std::size_t size() const;

    /**
     * How the parameter was typed, or null if it is not here.
     **/
    const Declaration * declaration(const std::string & name) const;

    /**
     * The values as they were read, ungrouped.
     **/
    const std::vector<float> & floats(const std::string & name) const;
    const std::vector<std::string> & strings(const std::string & name) const;

    /**
     * The values in threes. A parameter holding a partial trailing triple - which a file
     * whose array does not match its declaration can produce - contributes nothing for it.
     **/
    std::vector<glm::vec3> points(const std::string & name) const;

    /**
     * The first value, or the fallback when the parameter is absent or empty.
     **/
    float number(const std::string & name, float fallback) const;
    std::string string(const std::string & name, const std::string & fallback) const;

    /**
     * Sixteen floats as a matrix, or the identity. RIB writes a matrix in row major
     * order under RI's row vector convention and glm stores column major under a column
     * vector one, so reading the floats in order is the change of convention - a
     * transpose here would undo it.
     **/
    glm::mat4x4 matrix(const std::string & name) const;

 private:
    class Parameter final {
     public:
        Declaration declaration;
        std::vector<float> values;
        std::vector<std::string> strings;
    };

    std::map<std::string, Parameter> parameters_;
};

};  // namespace v3d::render::offline::rib
