/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/geometry/Tessellation.h>
#include <moya/libmoya/geometry/Topology.h>

#include <array>
#include <cstdint>
#include <map>
#include <vector>

#include <glm/vec2.hpp>

namespace v3d::moya::geometry::internal {

class Patches;

/**
 * Cuts a surface's patch faces into triangles, behind geometry::Surface::cut(). Each edge is cut
 * into its own number of pieces, and both patch faces beside it cut it alike. An edge is an edge
 * of the control mesh, or an edge between two patch faces of one face.
 *
 * Each edge is measured once, along the first face that reaches it. A control edge beside a face
 * that is cut into a patch face at each corner is cut into an even number of pieces, so that its
 * middle is a place.
 **/
class Cutter final {
 public:
    Cutter(const Topology & topology, const Patches & patches);

    /** Places along every edge that is cut, a run of them for each edge in turn. **/
    const Tessellation & along() const {
        return along_;
    }

    /** Where each edge's run of places begins in along(), and where the last run ends. **/
    const std::vector<std::uint32_t> & runs() const {
        return runs_;
    }

    /**
     * The surface cut, each edge into the pieces given for it in the order of the runs. A patch
     * face whose sides are all one piece is cut corner to corner. Any other has a grid inside it
     * as fine as its finest sides, and each side is joined to the grid's nearest row.
     **/
    Tessellation cut(const std::vector<std::uint32_t> & pieces) const;

 private:
    /** Where a place lies on the control mesh: what it lies on, two indices, and a piece. **/
    typedef std::array<std::uint32_t, 4> Key;

    /** The places already cut at each key, and the tessellation they are cut into. **/
    struct Cutting {
        std::map<Key, std::uint32_t> placed;
        Tessellation out;
    };

    /** Adds a run of places for each edge of a face that no earlier face measured. **/
    void measure(std::uint32_t face, std::vector<bool> & measured);

    /** Whether a face is cut into a patch face at each of its corners. **/
    bool split(std::uint32_t face) const;

    /** Appends a run of places along a side of a patch face, from one position to another. **/
    void sample(std::uint32_t patch, const glm::vec2 & from, const glm::vec2 & to);

    /** Ends a run of places for an edge. **/
    void endRun(std::uint32_t edge);

    /** The keys of the places along a control edge from one of its vertices to the other. **/
    std::vector<Key> edgeKeys(const std::vector<std::uint32_t> & rates, std::int32_t edge, std::int32_t from) const;

    /** The keys of the places along an edge inside a face, from a control edge's middle in. **/
    std::vector<Key> innerKeys(const std::vector<std::uint32_t> & rates, std::uint32_t face, std::uint32_t corner) const;

    /** Appends a place, which takes the point of the first place cut at its key. **/
    static std::uint32_t place(Cutting & cutting, std::uint32_t patch, std::uint32_t face, const glm::vec2 & position, const Key * key);

    /** The places along each side of a patch face, each corner shared by two sides. **/
    static std::vector<std::vector<std::uint32_t>> rim(Cutting & cutting, std::uint32_t patch, std::uint32_t face, const std::vector<glm::vec2> & corners,
        const std::vector<std::vector<Key>> & sides);

    /** Appends the triangles between a side's places and the row of a grid nearest it. **/
    static void stitch(const std::vector<std::uint32_t> & side, const std::vector<std::uint32_t> & row, std::uint32_t face, Tessellation & out);

    static void cutQuad(Cutting & cutting, std::uint32_t patch, std::uint32_t face, const std::vector<std::vector<Key>> & sides);
    static void cutTriangle(Cutting & cutting, std::uint32_t patch, std::uint32_t face, const std::vector<std::vector<Key>> & sides);

    const Topology & topology_;
    const Patches & patches_;

    /** Where each face's vertices begin among the indices. **/
    std::vector<std::uint32_t> offsets_;

    /** The first patch face of each face. **/
    std::vector<std::uint32_t> firstPatch_;

    /** Whether each patch face has a patch: a hole's, or an open boundary's, have none. **/
    std::vector<bool> patched_;

    /** Whether each control edge lies beside a face that is split. **/
    std::vector<bool> halved_;

    /**
     * The edge of each run: a control edge, or past the control edges by a face's offset and
     * corner, an edge inside a face.
     **/
    std::vector<std::uint32_t> edges_;

    Tessellation along_;
    std::vector<std::uint32_t> runs_;
};

};  // namespace v3d::moya::geometry::internal
