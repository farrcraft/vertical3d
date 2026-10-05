/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/asset/Loader.h>
#include <api/image/Image.h>

#include <string>

struct cgltf_image;

namespace v3d::asset::media::loader {
/**
 * Reads glTF 2.0, in either the .gltf or the .glb packaging, into a v3d::type::Model.
 *
 * The file's scene is walked from its roots, and every mesh a node names is merged into one
 * vertex array and one index run where the node's world matrix places it - ADR-0069. A mesh
 * two nodes name is merged twice. The scene is the one the file names as its default, or its
 * first, or every root node when it has none.
 *
 * Primitives sharing a material are one part of the model, in the order the walk reaches
 * them, and the parts are in the order their materials were first reached. Each primitive's
 * indices are rebased onto the merged array. One that carries no indices of its own gets a
 * sequential run, so the merged model is uniformly indexed rather than silently losing the
 * geometry. A primitive with no material is drawn with a default one.
 *
 * Positions are required; normals and texture coordinates are taken where a primitive has
 * them and left at zero where it does not.
 *
 * The first skin a mesh in the walk is bound to is the model's skeleton, with its joints put
 * in an order where a parent precedes its children, and every vertex has an influence. A
 * skinned mesh is placed by its joints and not by its node. Any other mesh follows the nearest
 * joint above it rigidly, or the first root under none, and stands where its node puts it
 * while that joint is at rest. Four influences a vertex are read, and a second set is dropped.
 *
 * Every animation in the file is a clip, keeping the channels that animate the skeleton's
 * joints. A file with no skeleton has no clips.
 *
 * Only the base colour of the metallic-roughness model is read. A texture the file names
 * arrives as that name, on v3d::type::Model::Material; one the file carries - a .glb's own
 * buffer, or a data uri - has no name to hand over and arrives decoded, on
 * v3d::asset::media::kind::Model::baseColourImage().
 **/
class Gltf final : public Loader {
 public:
    /**
     **/
    Gltf(const boost::shared_ptr<v3d::log::Logger>& logger);

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
};  // namespace v3d::asset::media::loader
