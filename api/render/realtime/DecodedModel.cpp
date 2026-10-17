/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "DecodedModel.h"

#include <api/asset/Manager.h>
#include <api/asset/Type.h>
#include <api/asset/media/kind/Image.h>
#include <api/asset/media/kind/Model.h>
#include <api/image/Image.h>

#include <cstddef>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/filesystem/path.hpp>

namespace v3d::render::realtime {

/**
 **/
DecodedModel decodeModel(asset::Manager& assets, const std::string& path, const AlbedoUploaded& uploaded) {
    const boost::shared_ptr<asset::media::kind::Model> loaded =
        assets.load<asset::media::kind::Model>(path, asset::Type::ModelGltf);
    if (!loaded || !loaded->model()) {
        throw std::runtime_error("Unable to load a model from " + path);
    }

    DecodedModel decoded;
    decoded.path = path;
    decoded.model = loaded->model();

    // an image packed into the file is the file's own, and one it names is resolved beside it
    const std::vector<type::Model::Material>& materials = decoded.model->materials();
    decoded.albedos.resize(materials.size());
    for (std::size_t index = 0; index < materials.size(); index++) {
        const std::string& named = materials[index].baseColourTexture;
        if (loaded->baseColourImage(index)) {
            decoded.albedos[index] = DecodedModel::Albedo{ path + "#albedo" + std::to_string(index), loaded->baseColourImage(index) };
        } else if (!named.empty()) {
            decoded.albedos[index].key = (boost::filesystem::path(path).parent_path() / named).generic_string();
        }
    }
    decodeAlbedos(assets, &decoded.albedos, uploaded);
    return decoded;
}

/**
 **/
void decodeAlbedos(asset::Manager& assets, std::vector<DecodedModel::Albedo>* albedos, const AlbedoUploaded& uploaded) {
    std::map<std::string, boost::shared_ptr<image::Image>> decoded;
    for (DecodedModel::Albedo& albedo : *albedos) {
        if (albedo.key.empty() || albedo.pixels || (uploaded && uploaded(albedo.key))) {
            continue;
        }
        const std::map<std::string, boost::shared_ptr<image::Image>>::const_iterator found = decoded.find(albedo.key);
        if (found != decoded.end()) {
            albedo.pixels = found->second;
            continue;
        }
        const boost::shared_ptr<asset::media::kind::Image> asset = assets.load<asset::media::kind::Image>(albedo.key);
        if (asset) {
            albedo.pixels = asset->image();
        }
        decoded.emplace(albedo.key, albedo.pixels);
    }
}

};  // namespace v3d::render::realtime
