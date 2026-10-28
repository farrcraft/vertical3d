/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "Curve.h"

#include <glm/vec3.hpp>

namespace v3d::curve {

/**
 * Any number of curves, each of its own type, as one piece of geometry.
 *
 * Every curve held is one that problem() finds nothing wrong with, and a Bezier curve's
 * Automatic and Vector handles are settled as it is stored. Each curve's evaluated points are
 * worked out when first asked for, and kept until that curve or the resolution changes. Asking
 * for them changes what is kept, so two threads must not read one Curves at once.
 *
 * It counts its changes. Adding, replacing or removing a curve, and setting the resolution, each
 * add one to changes(), which only goes up.
 **/
class Curves final {
 public:
    /**
     * Enough pieces per segment that a quarter circle drawn from them looks round.
     **/
    static constexpr unsigned int DEFAULT_RESOLUTION = 12;

    std::size_t size(void) const noexcept;
    bool empty(void) const noexcept;

    /**
     * @param index below size()
     **/
    const Curve& curve(std::size_t index) const;

    /**
     * Add a curve after the others.
     *
     * @return why the curve was refused, or empty when it was added
     **/
    std::string add(Curve curve);

    /**
     * Replace the curve at an index.
     *
     * @param index below size()
     * @return why the curve was refused, or empty when it replaced the one there
     **/
    std::string set(std::size_t index, Curve curve);

    /**
     * Remove the curve at an index. The curves after it move down one.
     *
     * @param index below size()
     **/
    void remove(std::size_t index);

    /**
     * The points the curve at an index passes through, as evaluate() gives them at this
     * geometry's resolution.
     *
     * @param index below size()
     **/
    const std::vector<glm::vec3>& evaluated(std::size_t index) const;

    unsigned int resolution(void) const noexcept;

    /**
     * @param pieces how many pieces each segment is cut into, at least 1
     **/
    void resolution(unsigned int pieces);

    std::uint64_t changes(void) const noexcept;

 private:
    std::vector<Curve> curves_;
    mutable std::vector<std::vector<glm::vec3>> evaluated_;
    unsigned int resolution_ = DEFAULT_RESOLUTION;
    std::uint64_t changes_ = 0;
};

};  // namespace v3d::curve
