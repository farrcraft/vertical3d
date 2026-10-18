/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <vector>

#include "Declaration.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace v3d::render::offline::rib {

/**
 * The token-value pairs of one request, typed by the declarations that were in force.
 *
 * Parameters are typed once, in the reader, so no handler works out again how many floats
 * "Cs" is. A name that is not here reads as empty or as the caller's fallback, and nothing
 * throws. A renderer must accept a request carrying a parameter it does not support.
 **/
class ParameterList final {
 public:
    void add(const std::string & name, const Declaration & declaration,
        const std::vector<float> & values, const std::vector<std::string> & strings);

    bool has(const std::string & name) const;
    std::size_t size() const;

    /**
     * Every parameter the request carried, in name order. A consumer that binds parameters
     * onto its own variables iterates these, rather than looking up names fixed in advance.
     **/
    std::vector<std::string> names() const;

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
    /**
     * The first value, or an empty string when the parameter is absent or empty.
     **/
    std::string string(const std::string & name) const;

    /**
     * Sixteen floats as a matrix, or the identity when there are fewer than sixteen. RIB
     * writes a matrix row by row, for row vectors. glm stores a matrix column by column, for
     * column vectors. Reading the floats in order therefore converts between the two
     * conventions, and a transpose here would undo it.
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
