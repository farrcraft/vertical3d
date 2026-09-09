/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/asset/Loader.h>
#include <api/image/Image.h>

#include <string>

struct cgltf_image;

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
 * Only the base colour of the metallic-roughness model is read. A texture the file names
 * arrives as that name, on v3d::type::Model::Material; one the file carries - a .glb's own
 * buffer, or a data uri - has no name to hand over and arrives decoded, on
 * v3d::asset::Model::baseColourImage().
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

 private:
    /**
     * Decode an image the file carried rather than named.
     *
     * @param model the asset being loaded, for the line a failure is reported on
     * @return the pixels, or null where the format is one glTF does not allow embedded or
     *         the bytes do not decode - either of which is a texture the model loses and
     *         not a model that fails to load
     **/
    boost::shared_ptr<v3d::image::Image> decodeEmbedded(const cgltf_image& image, std::string_view model);
};
};  // namespace v3d::asset::loader
