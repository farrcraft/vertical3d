/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/model/Model.h>

#include <functional>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::asset {
class Manager;
};  // namespace v3d::asset

namespace v3d::image {
class Image;
};  // namespace v3d::image

namespace v3d::render::realtime {

/**
 * A model and the pixels of the albedo each of its materials uses, read and decoded and not yet
 * uploaded. MeshRegistry::add uploads one.
 *
 * Building one reads files and makes no device call, so it may be built on a thread other than
 * the one that uploads it.
 **/
struct DecodedModel final {
    /**
     * One material's albedo.
     **/
    struct Albedo final {
        std::string key;  /**< what the albedo is shared by, or empty for a material with none **/
        boost::shared_ptr<image::Image> pixels;  /**< null when it was not found, or was skipped as uploaded **/
    };

    std::string path;  /**< what the model is registered under **/
    boost::shared_ptr<const model::Model> model;
    std::vector<Albedo> albedos;  /**< one per material, by index **/
};

/**
 * Whether an albedo key is already on the device, so its pixels need not be decoded.
 *
 * It is called on the thread that decodes. One that reads a MeshRegistry is safe only on the
 * thread that owns the registry.
 **/
using AlbedoUploaded = std::function<bool(const std::string& key)>;

/**
 * Read a glTF model and decode the albedo each material uses.
 *
 * An image the file embeds is keyed by the path, "#albedo" and the material's index. An image
 * the file names is keyed by its path beside the model, relative to the manager's root, and is
 * decoded through the manager. One that cannot be found has a key and no pixels.
 *
 * @param uploaded the keys whose pixels are not needed; none are skipped when it is empty
 * @throw std::runtime_error if the file is not a model
 **/
DecodedModel decodeModel(asset::Manager& assets, const std::string& path, const AlbedoUploaded& uploaded = {});

/**
 * Decode every albedo that has a key and no pixels, through the manager, unless it is uploaded.
 * Two albedos with one key share one decoded image. One that cannot be found keeps no pixels.
 **/
void decodeAlbedos(asset::Manager& assets, std::vector<DecodedModel::Albedo>* albedos, const AlbedoUploaded& uploaded = {});

};  // namespace v3d::render::realtime
