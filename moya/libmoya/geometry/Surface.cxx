/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/geometry/Surface.h>

#include <moya/libmoya/geometry/internal/Patches.h>
#include <moya/libmoya/geometry/internal/Scalar.h>
#include <moya/libmoya/geometry/Tessellation.h>
#include <moya/libmoya/geometry/Topology.h>
#include <moya/libmoya/scene/Declaration.h>

#include <opensubdiv/far/patchMap.h>
#include <opensubdiv/far/patchTable.h>
#include <opensubdiv/far/patchTableFactory.h>
#include <opensubdiv/far/primvarRefiner.h>
#include <opensubdiv/far/ptexIndices.h>
#include <opensubdiv/far/topologyDescriptor.h>
#include <opensubdiv/far/topologyRefiner.h>
#include <opensubdiv/far/topologyRefinerFactory.h>
#include <opensubdiv/sdc/options.h>
#include <opensubdiv/sdc/types.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

#include <glm/vec2.hpp>

namespace v3d::moya::geometry {

namespace {

namespace far = OpenSubdiv::Far;
namespace sdc = OpenSubdiv::Sdc;

/** How many levels adaptive refinement isolates creases and extraordinary vertices to. **/
constexpr unsigned int kIsolation = 6;

/** The most points one of OpenSubdiv's patches weighs: a Gregory basis patch's. **/
constexpr std::size_t kMostPatchPoints = 20;

/** Throws std::invalid_argument when a vertex is outside the points. **/
void checkVertex(std::int32_t vertex, std::uint32_t points) {
    if (vertex < 0 || static_cast<std::uint32_t>(vertex) >= points) {
        throw std::invalid_argument("vertex " + std::to_string(vertex) + " lies outside its " + std::to_string(points) + " points");
    }
}

/** Throws std::invalid_argument for a topology OpenSubdiv cannot refine, as Surface() lists. **/
void check(const Topology & topology) {
    std::size_t total = 0;
    for (const std::int32_t count : topology.vertices) {
        if (count < 3) {
            throw std::invalid_argument("a face of " + std::to_string(count) + " vertices is not a surface");
        }
        if (topology.scheme == Scheme::Loop && count != 3) {
            throw std::invalid_argument("a Loop surface's faces are triangles, and one has " + std::to_string(count) + " vertices");
        }
        total += static_cast<std::size_t>(count);
    }
    if (total != topology.indices.size()) {
        throw std::invalid_argument("its counts of vertices do not add up to its indices");
    }
    std::size_t creased = 0;
    for (const std::int32_t length : topology.creaseLengths) {
        if (length < 2) {
            throw std::invalid_argument("a crease of " + std::to_string(length) + " vertices has no edge");
        }
        creased += static_cast<std::size_t>(length);
    }
    if (creased != topology.creases.size() || topology.creaseSharpness.size() != topology.creaseLengths.size()) {
        throw std::invalid_argument("its crease lengths and sharpnesses do not match its creases");
    }
    if (topology.corners.size() != topology.cornerSharpness.size()) {
        throw std::invalid_argument("its corners and their sharpnesses do not match");
    }
    for (const std::vector<std::int32_t> * vertices : { &topology.indices, &topology.creases, &topology.corners }) {
        for (const std::int32_t vertex : *vertices) {
            checkVertex(vertex, topology.points);
        }
    }
    for (const std::int32_t hole : topology.holes) {
        if (hole < 0 || static_cast<std::size_t>(hole) >= topology.vertices.size()) {
            throw std::invalid_argument("hole " + std::to_string(hole) + " names no face");
        }
    }
}

sdc::SchemeType schemeOf(Scheme scheme) {
    switch (scheme) {
        case Scheme::Loop:
            return sdc::SCHEME_LOOP;
        case Scheme::Bilinear:
            return sdc::SCHEME_BILINEAR;
        case Scheme::CatmullClark:
            break;
    }
    return sdc::SCHEME_CATMARK;
}

sdc::Options::VtxBoundaryInterpolation boundaryOf(Boundary boundary) {
    switch (boundary) {
        case Boundary::Edges:
            return sdc::Options::VTX_BOUNDARY_EDGE_ONLY;
        case Boundary::EdgesAndCorners:
            return sdc::Options::VTX_BOUNDARY_EDGE_AND_CORNER;
        case Boundary::None:
            break;
    }
    return sdc::Options::VTX_BOUNDARY_NONE;
}

/** The options of a table of patches, with face-varying patches when a channel is given. **/
far::PatchTableFactory::Options patchOptions(bool faceVarying) {
    far::PatchTableFactory::Options out(kIsolation);
    out.SetEndCapType(far::PatchTableFactory::Options::ENDCAP_GREGORY_BASIS);
    out.useInfSharpPatch = true;
    out.generateVaryingTables = true;
    out.generateFVarTables = faceVarying;
    out.generateFVarLegacyLinearPatches = false;
    return out;
}

/**
 * A topology refined adaptively, and the table of patches over it. A face-varying channel of a
 * count of values is given by an index for each of the topology's indices, or by none.
 **/
boost::shared_ptr<const internal::Patches> patchesOf(const Topology & topology, const std::vector<std::int32_t> & faceVarying, std::size_t values) {
    // a crease of n vertices is n - 1 edges, each of its sharpness
    std::vector<std::int32_t> pairs;
    std::vector<float> weights;
    std::size_t at = 0;
    for (std::size_t crease = 0; crease < topology.creaseLengths.size(); ++crease) {
        const auto length = static_cast<std::size_t>(topology.creaseLengths[crease]);
        for (std::size_t edge = 0; edge + 1 < length; ++edge) {
            pairs.push_back(topology.creases[at + edge]);
            pairs.push_back(topology.creases[at + edge + 1]);
            weights.push_back(topology.creaseSharpness[crease]);
        }
        at += length;
    }
    far::TopologyDescriptor descriptor;
    descriptor.numVertices = static_cast<int>(topology.points);
    descriptor.numFaces = static_cast<int>(topology.vertices.size());
    descriptor.numVertsPerFace = topology.vertices.data();
    descriptor.vertIndicesPerFace = topology.indices.data();
    descriptor.numCreases = static_cast<int>(weights.size());
    descriptor.creaseVertexIndexPairs = pairs.data();
    descriptor.creaseWeights = weights.data();
    descriptor.numCorners = static_cast<int>(topology.corners.size());
    descriptor.cornerVertexIndices = topology.corners.data();
    descriptor.cornerWeights = topology.cornerSharpness.data();
    descriptor.numHoles = static_cast<int>(topology.holes.size());
    descriptor.holeIndices = topology.holes.data();
    far::TopologyDescriptor::FVarChannel channel;
    if (!faceVarying.empty()) {
        channel.numValues = static_cast<int>(values);
        channel.valueIndices = faceVarying.data();
        descriptor.numFVarChannels = 1;
        descriptor.fvarChannels = &channel;
    }
    sdc::Options options;
    options.SetVtxBoundaryInterpolation(boundaryOf(topology.boundary));
    options.SetFVarLinearInterpolation(sdc::Options::FVAR_LINEAR_CORNERS_ONLY);
    using Factory = far::TopologyRefinerFactory<far::TopologyDescriptor>;
    far::TopologyRefiner * refined = Factory::Create(descriptor, Factory::Options(schemeOf(topology.scheme), options));
    if (refined == nullptr) {
        throw std::invalid_argument("OpenSubdiv refuses its topology");
    }
    const boost::shared_ptr<far::TopologyRefiner> refiner(refined);
    const far::PatchTableFactory::Options patch = patchOptions(!faceVarying.empty());
    refiner->RefineAdaptive(patch.GetRefineAdaptiveOptions());
    const boost::shared_ptr<const far::PatchTable> table(far::PatchTableFactory::Create(*refiner, patch));
    return boost::make_shared<const internal::Patches>(refiner, table);
}

/**
 * One float of each control value, refined through every level of the refiner and on to the
 * table's local points, which the patches weigh.
 **/
std::vector<internal::Scalar> refined(const internal::Patches & patches, const std::vector<float> & values, std::size_t width, std::size_t component,
    scene::Interpolation interpolation) {
    const far::TopologyRefiner & refiner = patches.refiner();
    const far::PatchTable & table = patches.table();
    const bool faceVarying = interpolation == scene::Interpolation::FaceVarying;
    const bool varying = interpolation == scene::Interpolation::Varying;
    const auto onLevel = [&](int level) {
        return faceVarying ? refiner.GetLevel(level).GetNumFVarValues(0) : refiner.GetLevel(level).GetNumVertices();
    };
    const int total = faceVarying ? refiner.GetNumFVarValuesTotal(0) : refiner.GetNumVerticesTotal();
    int local = table.GetNumLocalPoints();
    if (faceVarying) {
        local = table.GetNumLocalPointsFaceVarying(0);
    } else if (varying) {
        local = table.GetNumLocalPointsVarying();
    }
    std::vector<internal::Scalar> out(static_cast<std::size_t>(total + local));
    for (std::size_t value = 0; value < static_cast<std::size_t>(onLevel(0)); ++value) {
        out[value].value = values[(value * width) + component];
    }
    const far::PrimvarRefiner primvars(refiner);
    internal::Scalar * source = out.data();
    for (int level = 1; level <= refiner.GetMaxLevel(); ++level) {
        internal::Scalar * destination = source + onLevel(level - 1);
        if (faceVarying) {
            primvars.InterpolateFaceVarying(level, source, destination, 0);
        } else if (varying) {
            primvars.InterpolateVarying(level, source, destination);
        } else {
            primvars.Interpolate(level, source, destination);
        }
        source = destination;
    }
    internal::Scalar * localPoints = out.data() + total;
    if (faceVarying) {
        table.ComputeLocalPointValuesFaceVarying(out.data(), localPoints, 0);
    } else if (varying) {
        table.ComputeLocalPointValuesVarying(out.data(), localPoints);
    } else {
        table.ComputeLocalPointValues(out.data(), localPoints);
    }
    return out;
}

/**
 * A primvar of vertex, varying or face-varying interpolation at each place of a tessellation, each
 * float refined and then weighed by the patch the place lies on.
 **/
std::vector<float> evaluated(const internal::Patches & patches, const std::vector<float> & values, std::size_t width,
    scene::Interpolation interpolation, const Tessellation & tessellation) {
    const far::PatchTable & table = patches.table();
    std::vector<float> out(tessellation.patches.size() * width, 0.0F);
    for (std::size_t component = 0; component < width; ++component) {
        const std::vector<internal::Scalar> points = refined(patches, values, width, component, interpolation);
        for (std::size_t place = 0; place < tessellation.patches.size(); ++place) {
            const glm::vec2 & at = tessellation.positions[place];
            const far::PatchTable::PatchHandle * handle = patches.map().FindPatch(static_cast<int>(tessellation.patches[place]), at.x, at.y);
            if (handle == nullptr) {
                continue;
            }
            float weights[kMostPatchPoints] {};
            far::ConstIndexArray indices;
            if (interpolation == scene::Interpolation::FaceVarying) {
                table.EvaluateBasisFaceVarying(*handle, at.x, at.y, weights);
                indices = table.GetPatchFVarValues(*handle);
            } else if (interpolation == scene::Interpolation::Varying) {
                table.EvaluateBasisVarying(*handle, at.x, at.y, weights);
                indices = table.GetPatchVaryingVertices(*handle);
            } else {
                table.EvaluateBasis(*handle, at.x, at.y, weights);
                indices = table.GetPatchVertices(*handle);
            }
            float sum = 0.0F;
            for (int point = 0; point < indices.size(); ++point) {
                sum += weights[point] * points[static_cast<std::size_t>(indices[point])].value;
            }
            out[(place * width) + component] = sum;
        }
    }
    return out;
}

/**
 * The value indices of a face-varying primvar given at each corner, and the values they index.
 * Corners of one vertex that give equal values share one.
 **/
std::vector<std::int32_t> welded(const Topology & topology, const std::vector<float> & values, std::size_t width, std::vector<float> & unique) {
    std::vector<std::int32_t> out(topology.indices.size());
    // the value indices already given at each vertex
    std::vector<std::vector<std::int32_t>> atVertex(topology.points);
    for (std::size_t corner = 0; corner < topology.indices.size(); ++corner) {
        const auto first = values.begin() + static_cast<std::ptrdiff_t>(corner * width);
        std::vector<std::int32_t> & given = atVertex[static_cast<std::size_t>(topology.indices[corner])];
        const auto same = std::ranges::find_if(given, [&](std::int32_t index) {
            return std::equal(first, first + static_cast<std::ptrdiff_t>(width), unique.begin() + (static_cast<std::ptrdiff_t>(index) * static_cast<std::ptrdiff_t>(width)));
        });
        if (same != given.end()) {
            out[corner] = *same;
            continue;
        }
        out[corner] = static_cast<std::int32_t>(unique.size() / width);
        given.push_back(out[corner]);
        unique.insert(unique.end(), first, first + static_cast<std::ptrdiff_t>(width));
    }
    return out;
}

/** Appends the places and triangles of a four-sided patch face, each side cut a rate of times. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the patch face, its face, then the rate
void cutQuad(std::uint32_t patch, std::uint32_t face, std::uint32_t rate, Tessellation & out) {
    const auto first = static_cast<std::uint32_t>(out.patches.size());
    const float step = 1.0F / static_cast<float>(rate);
    for (std::uint32_t row = 0; row <= rate; ++row) {
        for (std::uint32_t column = 0; column <= rate; ++column) {
            out.patches.push_back(patch);
            out.positions.emplace_back(static_cast<float>(column) * step, static_cast<float>(row) * step);
            out.placeFaces.push_back(face);
        }
    }
    const auto place = [&](std::uint32_t column, std::uint32_t row) {
        return first + (row * (rate + 1)) + column;
    };
    for (std::uint32_t row = 0; row < rate; ++row) {
        for (std::uint32_t column = 0; column < rate; ++column) {
            out.corners.insert(out.corners.end(), { place(column, row), place(column + 1, row), place(column + 1, row + 1) });
            out.corners.insert(out.corners.end(), { place(column, row), place(column + 1, row + 1), place(column, row + 1) });
            out.faces.insert(out.faces.end(), 2, face);
        }
    }
}

/** Appends the places and triangles of a triangular patch face, each side cut a rate of times. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the patch face, its face, then the rate
void cutTriangle(std::uint32_t patch, std::uint32_t face, std::uint32_t rate, Tessellation & out) {
    const auto first = static_cast<std::uint32_t>(out.patches.size());
    const float step = 1.0F / static_cast<float>(rate);
    std::vector<std::uint32_t> rows;
    for (std::uint32_t row = 0; row <= rate; ++row) {
        rows.push_back(static_cast<std::uint32_t>(out.patches.size()) - first);
        for (std::uint32_t column = 0; column + row <= rate; ++column) {
            out.patches.push_back(patch);
            out.positions.emplace_back(static_cast<float>(column) * step, static_cast<float>(row) * step);
            out.placeFaces.push_back(face);
        }
    }
    const auto place = [&](std::uint32_t column, std::uint32_t row) {
        return first + rows[row] + column;
    };
    for (std::uint32_t row = 0; row < rate; ++row) {
        for (std::uint32_t column = 0; column + row < rate; ++column) {
            out.corners.insert(out.corners.end(), { place(column, row), place(column + 1, row), place(column, row + 1) });
            out.faces.push_back(face);
            if (column + row + 1 < rate) {
                out.corners.insert(out.corners.end(), { place(column + 1, row), place(column + 1, row + 1), place(column, row + 1) });
                out.faces.push_back(face);
            }
        }
    }
}

};  // namespace

Surface::Surface(const Topology & topology) : topology_(topology) {
    check(topology_);
    // a surface of no faces has no patches, and OpenSubdiv is not asked to refine it
    if (!topology_.vertices.empty()) {
        patches_ = patchesOf(topology_, {}, 0);
    }
}

Tessellation Surface::cut(std::uint32_t rate) const {
    Tessellation out;
    if (!patches_) {
        return out;
    }
    const far::PtexIndices ptex(patches_->refiner());
    const bool triangular = topology_.scheme == Scheme::Loop;
    for (std::size_t face = 0; face < topology_.vertices.size(); ++face) {
        const int first = ptex.GetFaceId(static_cast<int>(face));
        const std::int32_t count = triangular || topology_.vertices[face] == 4 ? 1 : topology_.vertices[face];
        for (std::int32_t corner = 0; corner < count; ++corner) {
            const auto patch = static_cast<std::uint32_t>(first + corner);
            // a hole, or a face a boundary that is not interpolated leaves open, has no patch
            const double middle = triangular ? 1.0 / 3.0 : 0.5;
            if (patches_->map().FindPatch(static_cast<int>(patch), middle, middle) == nullptr) {
                continue;
            }
            if (triangular) {
                cutTriangle(patch, static_cast<std::uint32_t>(face), std::max(rate, 1U), out);
            } else {
                cutQuad(patch, static_cast<std::uint32_t>(face), std::max(rate, 1U), out);
            }
        }
    }
    return out;
}

std::vector<float> Surface::at(const std::vector<float> & values, std::size_t width, scene::Interpolation interpolation,
    const Tessellation & tessellation) const {
    std::size_t count = topology_.points;
    if (interpolation == scene::Interpolation::Constant) {
        count = 1;
    } else if (interpolation == scene::Interpolation::Uniform) {
        count = topology_.vertices.size();
    } else if (interpolation == scene::Interpolation::FaceVarying) {
        count = topology_.indices.size();
    }
    if (width == 0 || values.size() != count * width) {
        throw std::invalid_argument("a primvar of " + std::to_string(values.size()) + " floats is not " + std::to_string(count) + " values");
    }
    std::vector<float> out;
    if (!patches_) {
        return out;
    }
    switch (interpolation) {
        case scene::Interpolation::Constant:
            for (std::size_t place = 0; place < tessellation.patches.size(); ++place) {
                out.insert(out.end(), values.begin(), values.begin() + static_cast<std::ptrdiff_t>(width));
            }
            return out;
        case scene::Interpolation::Uniform:
            for (const std::uint32_t face : tessellation.placeFaces) {
                const auto first = values.begin() + static_cast<std::ptrdiff_t>(face * width);
                out.insert(out.end(), first, first + static_cast<std::ptrdiff_t>(width));
            }
            return out;
        case scene::Interpolation::FaceVarying: {
            std::vector<float> unique;
            const std::vector<std::int32_t> indices = welded(topology_, values, width, unique);
            const boost::shared_ptr<const internal::Patches> patches = patchesOf(topology_, indices, unique.size() / width);
            return evaluated(*patches, unique, width, interpolation, tessellation);
        }
        case scene::Interpolation::Varying:
        case scene::Interpolation::Vertex:
            break;
    }
    return evaluated(*patches_, values, width, interpolation, tessellation);
}

};  // namespace v3d::moya::geometry
