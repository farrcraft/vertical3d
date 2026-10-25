/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <unordered_map>

#include <glm/ext/quaternion_float.hpp>
#include <glm/mat4x4.hpp>

namespace v3d::editor {

/**
 * Where one item is, as an evaluation of its scene found it.
 *
 * The local matrix is measured in the parent's space, and the world matrix is the parent's
 * world matrix times the local one. An item with no parent has the identity as its parent's
 * world matrix.
 **/
struct Placed {
    glm::mat4 parent{ 1.0F };
    glm::mat4 local{ 1.0F };
    glm::mat4 world{ 1.0F };
};

/**
 * Where every item of a scene is, at the moment the scene was evaluated.
 *
 * It is a copy, keyed by each item's dag::Node id. An edit made after the evaluation does not
 * show in it, so a reader that has to see an edit evaluates the scene again after it.
 **/
class Evaluation final {
 public:
    /**
     * @return where the item of that id is, or the origin for an item the evaluation did not
     *         see. It is a copy, so it outlives the evaluation
     **/
    Placed placed(unsigned int id) const;

    /**
     * Record where an item is. The scene's evaluation calls this once for each item.
     **/
    void place(unsigned int id, const Placed& placed);

 private:
    std::unordered_map<unsigned int, Placed> placed_;
};

/**
 * The rotation a matrix turns its axes by, with its scale and any shear taken out.
 *
 * The x axis keeps its direction, the y axis is made square to it, and the z axis is square
 * to both. For a matrix composed of a translation, a rotation and a positive scale, this is
 * the rotation it was composed from.
 **/
glm::quat orientation(const glm::mat4& matrix);

/**
 * A matrix's translation and orientation(), without its scale or shear.
 **/
glm::mat4 withoutScale(const glm::mat4& matrix);

};  // namespace v3d::editor
