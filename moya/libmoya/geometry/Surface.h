/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/geometry/Tessellation.h>
#include <moya/libmoya/geometry/Topology.h>
#include <moya/libmoya/scene/Declaration.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::moya::geometry {

namespace internal {
class Patches;
};  // namespace internal

/**
 * The limit surface of a subdivision mesh, through OpenSubdiv. Its patches are built once from
 * the topology, and every sample of its points and every primvar is evaluated over them.
 *
 * Creases, corners and holes follow RI's tags, and a sharpness of 10 or more is infinite. Where the
 * boundary is not interpolated, its faces are left open. A face-varying value is interpolated
 * linearly at the corners of its seams, as RI's facevaryinginterpolateboundary does by default.
 **/
class Surface final {
 public:
    /**
     * The surface of a topology, which may have no faces. Throws std::invalid_argument for what
     * OpenSubdiv cannot refine: a face of fewer than three vertices, or a Loop face of more. It
     * throws for counts that do not add up to the indices, and an index outside the points. It
     * throws for tags whose counts do not match, and a hole that names no face.
     **/
    explicit Surface(const Topology & topology);

    /**
     * The surface cut into triangles, each side of each patch face into a rate of pieces. A
     * hole's faces, and the faces a boundary that is not interpolated leaves open, are not cut.
     **/
    Tessellation cut(std::uint32_t rate) const;

    /**
     * A primvar at each place of a tessellation, width floats per value. A vertex value is
     * evaluated on the limit surface, as the points are, and a varying one bilinearly across each
     * patch face. A face-varying value is given at each of the indices, and corners of one vertex
     * that give it equal values share it. A uniform value is its face's, and a constant one is the
     * same everywhere. Throws std::invalid_argument for values of another count than their
     * interpolation gives.
     **/
    std::vector<float> at(const std::vector<float> & values, std::size_t width, scene::Interpolation interpolation,
        const Tessellation & tessellation) const;

 private:
    Topology topology_;
    boost::shared_ptr<const internal::Patches> patches_;
};

};  // namespace v3d::moya::geometry
