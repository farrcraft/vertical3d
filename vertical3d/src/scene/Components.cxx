/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Components.h"

#include <api/brep/BRep.h>
#include <api/brep/HalfEdge.h>
#include <api/brep/Index.h>

#include <algorithm>
#include <cstddef>
#include <set>
#include <vector>

namespace v3d::editor {

/**
 **/
bool Components::selected(unsigned int item, Component kind, v3d::brep::Index index) const {
    const auto found = parts_.find(item);
    return found != parts_.end() && found->second[static_cast<std::size_t>(kind)].contains(index);
}

/**
 **/
void Components::select(unsigned int item, Component kind, v3d::brep::Index index, bool selected) {
    if (selected) {
        parts_[item][static_cast<std::size_t>(kind)].insert(index);
        return;
    }
    const auto found = parts_.find(item);
    if (found == parts_.end()) {
        return;
    }
    found->second[static_cast<std::size_t>(kind)].erase(index);
    if (!any(item)) {
        parts_.erase(found);
    }
}

/**
 **/
std::vector<v3d::brep::Index> Components::of(unsigned int item, Component kind) const {
    const auto found = parts_.find(item);
    if (found == parts_.end()) {
        return {};
    }
    const std::set<v3d::brep::Index>& parts = found->second[static_cast<std::size_t>(kind)];
    return std::vector<v3d::brep::Index>(parts.begin(), parts.end());
}

/**
 **/
bool Components::edge(unsigned int item, const v3d::brep::BRep& mesh, v3d::brep::Index half) const {
    if (selected(item, Component::Edge, half)) {
        return true;
    }
    const v3d::brep::HalfEdge* edge = mesh.edge(half);
    return edge != nullptr && edge->pair() != v3d::brep::INVALID_ID && selected(item, Component::Edge, edge->pair());
}

/**
 **/
bool Components::any(unsigned int item) const {
    const auto found = parts_.find(item);
    if (found == parts_.end()) {
        return false;
    }
    return std::ranges::any_of(found->second, [](const std::set<v3d::brep::Index>& parts) { return !parts.empty(); });
}

/**
 **/
void Components::clear(unsigned int item) noexcept {
    parts_.erase(item);
}

/**
 **/
void Components::clear() noexcept {
    parts_.clear();
}

};  // namespace v3d::editor
