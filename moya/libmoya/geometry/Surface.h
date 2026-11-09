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
#include <functional>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <glm/vec3.hpp>

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

    /** How many pieces an edge is cut into, from points along it on the limit surface. **/
    typedef std::function<std::size_t(const std::vector<glm::vec3> & along)> Measure;

    /**
     * The surface cut into triangles, each edge into the pieces a measure gives it. An edge is an
     * edge of the control mesh, or one between two patch faces of a face, and both patch faces
     * beside it cut it alike, so the surface has no cracks. The measure is given points along the
     * edge on the limit surface of points, three floats to each of the topology's points. A
     * control edge beside a face cut into a patch face at each corner is cut into an even number
     * of pieces. A hole's faces, and the faces a boundary that is not interpolated leaves open,
     * are not cut. Throws std::invalid_argument for points of another count.
     **/
    Tessellation cut(const std::vector<float> & points, const Measure & measure) const;

    /** The surface cut as the measured cut() cuts it, each edge into a rate of pieces. **/
    Tessellation cut(std::uint32_t rate) const;

    /**
     * The unit normal of the limit surface of points at each place of a tessellation, three
     * floats each. It is the cross product of the surface's derivatives along a patch face's two
     * sides, so a face wound counterclockwise as it is seen faces the viewer. Where the
     * derivatives meet in a line, as they may at a corner, the normal is taken a little inside the
     * patch face. Throws std::invalid_argument for points of another count than the topology's.
     **/
    std::vector<float> normals(const std::vector<float> & points, const Tessellation & tessellation) const;

    /**
     * A primvar at each place of a tessellation, width floats per value. A vertex value is
     * evaluated on the limit surface, as the points are, and a varying one bilinearly across each
     * patch face. Each place takes a vertex or varying value from the place it shares a point
     * with. A face-varying value is given at each of the indices, and corners of one vertex that
     * give it equal values share it. A uniform value is its face's, and a constant one is the
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
