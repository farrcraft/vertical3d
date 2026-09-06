/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "../Loader.h"

namespace v3d::asset::loader {
/**
 * Reads glTF 2.0, in either the .gltf or the .glb packaging, into a v3d::type::Model.
 *
 * Every mesh and every primitive in the file is merged into one vertex array and one index
 * run, with each primitive's indices rebased onto the merged array. A primitive that
 * carries no indices of its own gets a sequential run, so the merged model is uniformly
 * indexed rather than silently losing the geometry. The first material encountered is the
 * model's - a merge collapses the file into one draw, so there is nowhere for a second one
 * to apply.
 *
 * Positions are required; normals and texture coordinates are taken where a primitive has
 * them and left at zero where it does not.
 *
 * Only the base colour of the metallic-roughness model is read, and its texture arrives as
 * the name the file gave it rather than as pixels - see v3d::type::Model::Material. An
 * image embedded in a .glb has no name to hand over and is reported rather than dropped
 * quietly: decoding one needs an image reader that can be pointed at a buffer, which
 * api/image does not have.
 **/
class Gltf final : public Loader {
 public:
    /**
     **/
    Gltf(Manager* manager, const boost::shared_ptr<v3d::log::Logger>& logger);

    /**
     * @param name the path to a .gltf or .glb file
     * @return the model, or no asset at all where the file could not be read or parsed -
     *         the same way every other loader in the tree reports a failure
     **/
    boost::shared_ptr<Asset> load(std::string_view name) override;
};
};  // namespace v3d::asset::loader
