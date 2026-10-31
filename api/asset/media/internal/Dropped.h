/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

namespace v3d::asset::media::internal {

/**
 * What the traversal found that a model cannot hold, for the loader to report.
 **/
struct Dropped final {
    bool skins{ false };       /**< a mesh bound to a skin other than the one kept **/
    bool influences{ false };  /**< a second set of joints and weights **/
    bool unweighted{ false };  /**< a skinned primitive with no joints, or none that weigh anything **/
    bool channels{ false };    /**< a channel animating a node that is not one of the skeleton's joints **/
    bool lines{ false };       /**< a primitive of points or lines, which a model of triangles cannot hold **/
    bool joints{ false };      /**< a weight given to a joint the skin does not have **/
};

};  // namespace v3d::asset::media::internal
