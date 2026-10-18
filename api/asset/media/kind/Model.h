/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/asset/Asset.h>
#include <api/image/Image.h>
#include <api/type/Model.h>

#include <cstddef>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::asset::media::kind {
/**
 * Geometry loaded from a model file, as the asset manager hands it out.
 **/
class Model : public Asset {
 public:
    /**
     **/
    Model(const std::string& name, Type t, const boost::shared_ptr<v3d::type::Model>& model,
        const std::vector<boost::shared_ptr<v3d::image::Image>>& baseColours = {});

    /**
     **/
    boost::shared_ptr<v3d::type::Model> model();

    /**
     * A material's base colour texture, when the file stored its pixels instead of a uri.
     *
     * A .glb packs its images into its own buffer, and a .gltf may inline one as a data uri.
     * Neither has a path for the material to name, so type::Model::Material's
     * baseColourTexture is empty. The decoded pixels arrive here instead, for the app to
     * upload the way it uploads any other image.
     *
     * The image is held on the asset, not on the material, because it describes how the file
     * was packaged, not the surface. This also keeps api/type free of api/image, so a model's
     * consumers, the offline renderer included, depend on glm alone.
     *
     * Null for a material whose texture is a separate file, and for one with no texture.
     *
     * @param material an index into the model's materials()
     **/
    boost::shared_ptr<v3d::image::Image> baseColourImage(std::size_t material) const;

 private:
    boost::shared_ptr<v3d::type::Model> model_;
    std::vector<boost::shared_ptr<v3d::image::Image>> baseColours_;  /**< by material, and short when the last have none **/
};
};  // namespace v3d::asset::media::kind
