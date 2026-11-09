/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/geometry/subdivision/internal/Cutter.h>

#include <moya/libmoya/geometry/subdivision/internal/Patches.h>
#include <moya/libmoya/geometry/subdivision/Tessellation.h>
#include <moya/libmoya/geometry/subdivision/Topology.h>

#include <opensubdiv/far/ptexIndices.h>
#include <opensubdiv/far/topologyLevel.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include <glm/vec2.hpp>

namespace v3d::moya::geometry::subdivision::internal {

namespace {

namespace far = OpenSubdiv::Far;

/** How many pieces each edge is sampled in to be measured. **/
constexpr std::uint32_t kSamples = 4;

/** What a place lies on: a control vertex or edge, a face's centre, or an edge inside a face. **/
constexpr std::uint32_t kVertex = 0;
constexpr std::uint32_t kEdge = 1;
constexpr std::uint32_t kCentre = 2;
constexpr std::uint32_t kInner = 3;

/** The corners of a four-sided patch face, counterclockwise. **/
const std::vector<glm::vec2> & quad() {
    static const std::vector<glm::vec2> corners { { 0.0F, 0.0F }, { 1.0F, 0.0F }, { 1.0F, 1.0F }, { 0.0F, 1.0F } };
    return corners;
}

/** The corners of a triangular patch face, counterclockwise. **/
const std::vector<glm::vec2> & triangular() {
    static const std::vector<glm::vec2> corners { { 0.0F, 0.0F }, { 1.0F, 0.0F }, { 0.0F, 1.0F } };
    return corners;
}

/** Whether every side of a patch face is one piece: its two corners and nothing between. **/
bool whole(const std::vector<std::vector<std::uint32_t>> & sides) {
    return std::ranges::all_of(sides, [](const std::vector<std::uint32_t> & side) {
        return side.size() == 2;
    });
}

void triangle(Tessellation & out, std::uint32_t face, std::uint32_t first, std::uint32_t second, std::uint32_t third) {
    out.corners.insert(out.corners.end(), { first, second, third });
    out.faces.push_back(face);
}

};  // namespace

Cutter::Cutter(const Topology & topology, const Patches & patches) : topology_(topology), patches_(patches) {
    const far::TopologyLevel & level = patches_.refiner().GetLevel(0);
    const far::PtexIndices ptex(patches_.refiner());
    patched_.assign(static_cast<std::size_t>(ptex.GetNumFaces()), false);
    std::uint32_t offset = 0;
    for (std::uint32_t face = 0; face < topology_.vertices.size(); ++face) {
        offsets_.push_back(offset);
        offset += static_cast<std::uint32_t>(topology_.vertices[face]);
        firstPatch_.push_back(static_cast<std::uint32_t>(ptex.GetFaceId(static_cast<int>(face))));
        const std::uint32_t count = split(face) ? static_cast<std::uint32_t>(topology_.vertices[face]) : 1U;
        const double middle = topology_.scheme == Scheme::Loop ? 1.0 / 3.0 : 0.5;
        for (std::uint32_t corner = 0; corner < count; ++corner) {
            const std::uint32_t patch = firstPatch_[face] + corner;
            patched_[patch] = patches_.map().FindPatch(static_cast<int>(patch), middle, middle) != nullptr;
        }
    }
    halved_.assign(static_cast<std::size_t>(level.GetNumEdges()), false);
    for (int edge = 0; edge < level.GetNumEdges(); ++edge) {
        for (const int face : level.GetEdgeFaces(edge)) {
            halved_[static_cast<std::size_t>(edge)] = halved_[static_cast<std::size_t>(edge)] || split(static_cast<std::uint32_t>(face));
        }
    }

    // each edge is measured along the first face whose patch faces reach it
    std::vector<bool> measured(halved_.size(), false);
    for (std::uint32_t face = 0; face < topology_.vertices.size(); ++face) {
        measure(face, measured);
    }
    runs_.push_back(static_cast<std::uint32_t>(along_.patches.size()));
}

void Cutter::measure(std::uint32_t face, std::vector<bool> & measured) {
    const far::ConstIndexArray edges = patches_.refiner().GetLevel(0).GetFaceEdges(static_cast<int>(face));
    const auto count = static_cast<std::uint32_t>(edges.size());
    const std::uint32_t first = firstPatch_[face];
    if (!split(face)) {
        if (!patched_[first]) {
            return;
        }
        const std::vector<glm::vec2> & corners = topology_.scheme == Scheme::Loop ? triangular() : quad();
        for (std::uint32_t side = 0; side < count; ++side) {
            const auto edge = static_cast<std::size_t>(edges[static_cast<int>(side)]);
            if (!measured[edge]) {
                measured[edge] = true;
                sample(first, corners[side], corners[(side + 1) % count]);
                endRun(static_cast<std::uint32_t>(edge));
            }
        }
        return;
    }
    // patch face c runs from vertex c to the middle of edge c, the centre, and the middle of
    // edge c - 1. Edge c is the first side of patch face c and the last of patch face c + 1
    const std::vector<glm::vec2> & corners = quad();
    for (std::uint32_t corner = 0; corner < count; ++corner) {
        const std::uint32_t here = first + corner;
        const std::uint32_t next = first + ((corner + 1) % count);
        const auto edge = static_cast<std::size_t>(edges[static_cast<int>(corner)]);
        if (!measured[edge] && patched_[here] && patched_[next]) {
            measured[edge] = true;
            sample(here, corners[0], corners[1]);
            sample(next, corners[3], corners[0]);
            endRun(static_cast<std::uint32_t>(edge));
        }
        if (patched_[here]) {
            sample(here, corners[1], corners[2]);
            endRun(static_cast<std::uint32_t>(halved_.size()) + offsets_[face] + corner);
        }
    }
}

bool Cutter::split(std::uint32_t face) const {
    return topology_.scheme != Scheme::Loop && topology_.vertices[face] != 4;
}

void Cutter::sample(std::uint32_t patch, const glm::vec2 & from, const glm::vec2 & to) {
    if (runs_.size() == edges_.size()) {
        runs_.push_back(static_cast<std::uint32_t>(along_.patches.size()));
    }
    for (std::uint32_t step = 0; step <= kSamples; ++step) {
        const float at = static_cast<float>(step) / static_cast<float>(kSamples);
        along_.patches.push_back(patch);
        along_.positions.push_back(from + ((to - from) * at));
        along_.placeFaces.push_back(0);
    }
}

void Cutter::endRun(std::uint32_t edge) {
    edges_.push_back(edge);
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the edge, then the vertex it starts at
std::vector<Cutter::Key> Cutter::edgeKeys(const std::vector<std::uint32_t> & rates, std::int32_t edge, std::int32_t from) const {
    const far::ConstIndexArray ends = patches_.refiner().GetLevel(0).GetEdgeVertices(edge);
    const std::uint32_t pieces = rates[static_cast<std::size_t>(edge)];
    std::vector<Key> out;
    out.reserve(pieces + 1);
    for (std::uint32_t piece = 0; piece <= pieces; ++piece) {
        // pieces are counted from the edge's first vertex, whichever side cuts it
        const std::uint32_t at = ends[0] == from ? piece : pieces - piece;
        if (at == 0) {
            out.push_back(Key { kVertex, static_cast<std::uint32_t>(ends[0]), 0, 0 });
        } else if (at == pieces) {
            out.push_back(Key { kVertex, static_cast<std::uint32_t>(ends[1]), 0, 0 });
        } else {
            out.push_back(Key { kEdge, static_cast<std::uint32_t>(edge), at, 0 });
        }
    }
    return out;
}

std::vector<Cutter::Key> Cutter::innerKeys(const std::vector<std::uint32_t> & rates, std::uint32_t face, std::uint32_t corner) const {
    const far::TopologyLevel & level = patches_.refiner().GetLevel(0);
    const std::int32_t edge = level.GetFaceEdges(static_cast<int>(face))[static_cast<int>(corner)];
    const std::uint32_t pieces = rates[halved_.size() + offsets_[face] + corner];
    std::vector<Key> out;
    out.reserve(pieces + 1);
    out.push_back(Key { kEdge, static_cast<std::uint32_t>(edge), rates[static_cast<std::size_t>(edge)] / 2, 0 });
    for (std::uint32_t piece = 1; piece < pieces; ++piece) {
        out.push_back(Key { kInner, face, corner, piece });
    }
    out.push_back(Key { kCentre, face, 0, 0 });
    return out;
}

Tessellation Cutter::cut(const std::vector<std::uint32_t> & pieces) const {
    std::vector<std::uint32_t> rates(halved_.size() + topology_.indices.size(), 1);
    for (std::size_t run = 0; run < edges_.size(); ++run) {
        rates[edges_[run]] = std::max(pieces[run], 1U);
    }
    for (std::size_t edge = 0; edge < halved_.size(); ++edge) {
        if (halved_[edge]) {
            rates[edge] += rates[edge] % 2;
        }
    }
    const far::TopologyLevel & level = patches_.refiner().GetLevel(0);
    Cutting cutting;
    for (std::uint32_t face = 0; face < topology_.vertices.size(); ++face) {
        const far::ConstIndexArray edges = level.GetFaceEdges(static_cast<int>(face));
        const far::ConstIndexArray vertices = level.GetFaceVertices(static_cast<int>(face));
        const auto count = static_cast<std::uint32_t>(edges.size());
        if (!split(face)) {
            if (!patched_[firstPatch_[face]]) {
                continue;
            }
            std::vector<std::vector<Key>> sides;
            sides.reserve(count);
            for (std::uint32_t side = 0; side < count; ++side) {
                sides.push_back(edgeKeys(rates, edges[static_cast<int>(side)], vertices[static_cast<int>(side)]));
            }
            if (topology_.scheme == Scheme::Loop) {
                cutTriangle(cutting, firstPatch_[face], face, sides);
            } else {
                cutQuad(cutting, firstPatch_[face], face, sides);
            }
            continue;
        }
        for (std::uint32_t corner = 0; corner < count; ++corner) {
            const std::uint32_t patch = firstPatch_[face] + corner;
            if (!patched_[patch]) {
                continue;
            }
            const std::uint32_t previous = (corner + count - 1) % count;
            // the sides run along the first half of edge c from vertex c, and in to the centre.
            // They run out to the middle of edge c - 1, and along its second half to vertex c
            std::vector<Key> first = edgeKeys(rates, edges[static_cast<int>(corner)], vertices[static_cast<int>(corner)]);
            first.resize((first.size() / 2) + 1);
            std::vector<Key> out = innerKeys(rates, face, previous);
            std::ranges::reverse(out);
            std::vector<Key> last = edgeKeys(rates, edges[static_cast<int>(previous)], vertices[static_cast<int>(previous)]);
            last.erase(last.begin(), last.begin() + static_cast<std::ptrdiff_t>(last.size() / 2));
            cutQuad(cutting, patch, face, { first, innerKeys(rates, face, corner), out, last });
        }
    }
    return std::move(cutting.out);
}

std::uint32_t Cutter::place(Cutting & cutting, std::uint32_t patch, std::uint32_t face, const glm::vec2 & position, const Key * key) {
    Tessellation & out = cutting.out;
    const auto index = static_cast<std::uint32_t>(out.patches.size());
    out.patches.push_back(patch);
    out.positions.push_back(position);
    out.placeFaces.push_back(face);
    std::uint32_t takes = index;
    if (key != nullptr) {
        takes = cutting.placed.try_emplace(*key, index).first->second;
    }
    out.shared.push_back(takes);
    return index;
}

std::vector<std::vector<std::uint32_t>> Cutter::rim(Cutting & cutting, std::uint32_t patch, std::uint32_t face, const std::vector<glm::vec2> & corners,
    const std::vector<std::vector<Key>> & sides) {
    const std::size_t count = corners.size();
    std::vector<std::uint32_t> at;
    at.reserve(count);
    for (std::size_t corner = 0; corner < count; ++corner) {
        at.push_back(place(cutting, patch, face, corners[corner], &sides[corner].front()));
    }
    std::vector<std::vector<std::uint32_t>> out(count);
    for (std::size_t side = 0; side < count; ++side) {
        const std::size_t pieces = sides[side].size() - 1;
        const glm::vec2 & from = corners[side];
        const glm::vec2 & to = corners[(side + 1) % count];
        out[side].push_back(at[side]);
        for (std::size_t piece = 1; piece < pieces; ++piece) {
            const float along = static_cast<float>(piece) / static_cast<float>(pieces);
            out[side].push_back(place(cutting, patch, face, from + ((to - from) * along), &sides[side][piece]));
        }
        out[side].push_back(at[(side + 1) % count]);
    }
    return out;
}

void Cutter::stitch(const std::vector<std::uint32_t> & side, const std::vector<std::uint32_t> & row, std::uint32_t face, Tessellation & out) {
    // the side's places lie at steps of 1 / pieces along it, and the row's between its ends at
    // steps of 1 / (count + 1), and the nearer next place is joined first
    const auto pieces = static_cast<std::uint64_t>(side.size() - 1);
    const auto steps = static_cast<std::uint64_t>(row.size() + 1);
    std::size_t outer = 0;
    std::size_t inner = 0;
    while (outer + 1 < side.size() || inner + 1 < row.size()) {
        const bool alongSide = inner + 1 == row.size() ||
            (outer + 1 < side.size() && (static_cast<std::uint64_t>(outer + 1) * steps) <= (static_cast<std::uint64_t>(inner + 2) * pieces));
        if (alongSide) {
            triangle(out, face, side[outer], side[outer + 1], row[inner]);
            ++outer;
        } else {
            triangle(out, face, side[outer], row[inner + 1], row[inner]);
            ++inner;
        }
    }
}

void Cutter::cutQuad(Cutting & cutting, std::uint32_t patch, std::uint32_t face, const std::vector<std::vector<Key>> & sides) {
    const std::vector<std::vector<std::uint32_t>> edges = rim(cutting, patch, face, quad(), sides);
    Tessellation & out = cutting.out;
    if (whole(edges)) {
        triangle(out, face, edges[0][0], edges[1][0], edges[2][0]);
        triangle(out, face, edges[0][0], edges[2][0], edges[3][0]);
        return;
    }
    const auto across = static_cast<std::uint32_t>(std::max({ edges[0].size(), edges[2].size(), std::size_t { 3 } }) - 1);
    const auto up = static_cast<std::uint32_t>(std::max({ edges[1].size(), edges[3].size(), std::size_t { 3 } }) - 1);
    // the grid's places at i / across and j / up, for i and j inside the patch face
    std::vector<std::uint32_t> grid;
    for (std::uint32_t row = 1; row < up; ++row) {
        for (std::uint32_t column = 1; column < across; ++column) {
            const glm::vec2 position { static_cast<float>(column) / static_cast<float>(across), static_cast<float>(row) / static_cast<float>(up) };
            grid.push_back(place(cutting, patch, face, position, nullptr));
        }
    }
    const auto at = [&](std::uint32_t column, std::uint32_t row) {
        return grid[((row - 1) * (across - 1)) + (column - 1)];
    };
    for (std::uint32_t row = 1; row + 1 < up; ++row) {
        for (std::uint32_t column = 1; column + 1 < across; ++column) {
            triangle(out, face, at(column, row), at(column + 1, row), at(column + 1, row + 1));
            triangle(out, face, at(column, row), at(column + 1, row + 1), at(column, row + 1));
        }
    }
    std::vector<std::vector<std::uint32_t>> rows(4);
    for (std::uint32_t column = 1; column < across; ++column) {
        rows[0].push_back(at(column, 1));
        rows[2].push_back(at(across - column, up - 1));
    }
    for (std::uint32_t row = 1; row < up; ++row) {
        rows[1].push_back(at(across - 1, row));
        rows[3].push_back(at(1, up - row));
    }
    for (std::size_t side = 0; side < 4; ++side) {
        stitch(edges[side], rows[side], face, out);
    }
}

void Cutter::cutTriangle(Cutting & cutting, std::uint32_t patch, std::uint32_t face, const std::vector<std::vector<Key>> & sides) {
    const std::vector<std::vector<std::uint32_t>> edges = rim(cutting, patch, face, triangular(), sides);
    Tessellation & out = cutting.out;
    if (whole(edges)) {
        triangle(out, face, edges[0][0], edges[1][0], edges[2][0]);
        return;
    }
    const auto pieces = static_cast<std::uint32_t>(std::max({ edges[0].size(), edges[1].size(), edges[2].size(), std::size_t { 4 } }) - 1);
    // the grid's places at i / pieces and j / pieces, for i and j of at least 1 that add up to
    // less than pieces
    std::vector<std::uint32_t> rowStart;
    std::vector<std::uint32_t> grid;
    for (std::uint32_t row = 1; row + 1 < pieces; ++row) {
        rowStart.push_back(static_cast<std::uint32_t>(grid.size()));
        for (std::uint32_t column = 1; column + row < pieces; ++column) {
            const glm::vec2 position { static_cast<float>(column) / static_cast<float>(pieces), static_cast<float>(row) / static_cast<float>(pieces) };
            grid.push_back(place(cutting, patch, face, position, nullptr));
        }
    }
    const auto at = [&](std::uint32_t column, std::uint32_t row) {
        return grid[rowStart[row - 1] + (column - 1)];
    };
    for (std::uint32_t row = 1; row + 2 < pieces; ++row) {
        for (std::uint32_t column = 1; column + row + 2 <= pieces; ++column) {
            triangle(out, face, at(column, row), at(column + 1, row), at(column, row + 1));
            if (column + row + 3 <= pieces) {
                triangle(out, face, at(column + 1, row), at(column + 1, row + 1), at(column, row + 1));
            }
        }
    }
    std::vector<std::vector<std::uint32_t>> rows(3);
    for (std::uint32_t step = 1; step + 1 < pieces; ++step) {
        rows[0].push_back(at(step, 1));
        rows[1].push_back(at(pieces - 1 - step, step));
        rows[2].push_back(at(1, pieces - 1 - step));
    }
    for (std::size_t side = 0; side < 3; ++side) {
        stitch(edges[side], rows[side], face, out);
    }
}

};  // namespace v3d::moya::geometry::subdivision::internal
