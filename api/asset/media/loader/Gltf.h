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
 * Reads glTF 2.0, in either the .gltf or the .glb packaging, into a v3d::model::Model.
 *
 * The loader traverses the scene's node hierarchy from its roots, not the file's mesh list,
 * so that node transforms apply. Every mesh a node names is merged into one vertex array and
 * one index run, placed by the node's world matrix. Its normals are turned by that matrix's
 * inverse transpose. A mesh that two nodes name is merged twice. The scene is the one the file
 * names as its default, or its first, or every root node when it has no scene.
 *
 * Primitives that share a material form one part of the model, in the order the traversal
 * reaches them. The parts are in the order their materials were first reached. Each
 * primitive's indices are rebased onto the merged array. A primitive with no indices of its
 * own gets a sequential run, so every part of the merged model is indexed. A primitive with no
 * material is drawn with a default one.
 *
 * Positions are required. Normals and texture coordinates are read where a primitive has
 * them, and left at zero where it does not.
 *
 * The first skin that a traversed mesh is bound to becomes the model's skeleton. Its joints
 * are reordered so that a parent precedes its children, and every vertex gets an influence. A
 * skinned mesh is placed by its joints, not by its node. Any other mesh follows one joint
 * rigidly: the nearest joint above it, or the first root when it is under none. It stands
 * where its node puts it while that joint is at rest. Four influences are read for each
 * vertex, and a second set of joints and weights is dropped.
 *
 * Every animation in the file becomes a clip, keeping the channels that animate the
 * skeleton's joints. A file with no skeleton has no clips.
 *
 * Only the base colour of the metallic-roughness model is read. A texture in a separate file
 * arrives as its uri, on v3d::model::Model::Material. A texture stored inside the file, in a
 * .glb's own buffer or as a data uri, has no name. It arrives decoded, on
 * v3d::asset::media::kind::Model::baseColourImage().
 **/
class Gltf final : public Loader {
 public:
    /**
     **/
    Gltf(const boost::shared_ptr<v3d::log::Logger>& logger);

    /**
     * @param name the path to a .gltf or .glb file
     * @return the model, or null when the file cannot be read, parsed or validated, or
     *         holds no geometry
     **/
    boost::shared_ptr<Asset> load(std::string_view name) override;

 private:
    /**
     * Decode an image stored inside the file.
     *
     * @param model the asset being loaded, for the line a failure is reported on
     * @return the pixels, or null where the format is one glTF does not allow embedded or
     *         the bytes do not decode. Either way the model loses that texture and still
     *         loads.
     **/
    boost::shared_ptr<v3d::image::Image> decodeEmbedded(const cgltf_image& image, std::string_view model);
};
};  // namespace v3d::asset::media::loader
