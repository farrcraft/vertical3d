/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/brep/BRep.h>
#include <api/brep/Index.h>

#include <array>
#include <cstdint>
#include <map>
#include <set>
#include <vector>

namespace v3d::editor {

/**
 * The kinds of part of an item that can be selected.
 *
 * A mesh item has vertices, edges and faces. A curve item has control points, which are its
 * vertices, and the handles of its Bezier points.
 **/
enum class Component : std::uint8_t { Vertex, Edge, Face, Handle };

/**
 * Which parts of which mesh and curve items are selected.
 *
 * A part is named by its item's id() and its index in the item's geometry. A part is a vertex,
 * a half edge or a face of a mesh, or a control point or a handle of a curve as CurvePoint
 * numbers them. The selection belongs to the item, not to the geometry, so two items that share one
 * mesh select its parts apart. An edge is selected when either of its two halves is.
 *
 * An index names whatever part holds it now. An operation that renumbers an item's geometry
 * replaces the item's selection, or a part it named could be another part afterwards.
 **/
class Components final {
 public:
    /**
     * @return whether the part is selected
     **/
    bool selected(unsigned int item, Component kind, v3d::brep::Index index) const;

    /**
     * Select a part, or deselect it.
     **/
    void select(unsigned int item, Component kind, v3d::brep::Index index, bool selected);

    /**
     * @return the selected parts of one kind of an item, rising
     **/
    std::vector<v3d::brep::Index> of(unsigned int item, Component kind) const;

    /**
     * @return whether the edge a half edge belongs to is selected, which either of its halves
     *         being selected is enough for
     **/
    bool edge(unsigned int item, const v3d::brep::BRep& mesh, v3d::brep::Index half) const;

    /**
     * @return whether any part of an item is selected
     **/
    bool any(unsigned int item) const;

    /**
     * Deselect every part of one item.
     **/
    void clear(unsigned int item) noexcept;

    /**
     * Deselect every part of every item.
     **/
    void clear() noexcept;

 private:
    // each item's selected vertices, half edges, faces and handles, in the order of Component
    std::map<unsigned int, std::array<std::set<v3d::brep::Index>, 4>> parts_;
};

};  // namespace v3d::editor
