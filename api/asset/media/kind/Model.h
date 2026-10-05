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
     * A material's base colour texture, where the file carried its pixels rather than a name.
     *
     * A .glb packs its images into its own buffer and a .gltf may inline one as a data
     * uri, so there is no path for the material to name and type::Model::Material's
     * baseColourTexture is empty. Decoded pixels arrive here instead, for the app to
     * upload the way it uploads any other image.
     *
     * It is on the asset rather than on the material because it is a fact about how the
     * file was packaged rather than about the surface, and because api/type is built
     * against glm alone - a material holding an image would take api/image into every
     * consumer of a mesh, the offline renderer included.
     *
     * Null for a material whose texture was named, which is every .gltf pointing at a file
     * beside it, and for one with no texture at all.
     *
     * @param material an index into the model's materials()
     **/
    boost::shared_ptr<v3d::image::Image> baseColourImage(std::size_t material) const;

 private:
    boost::shared_ptr<v3d::type::Model> model_;
    std::vector<boost::shared_ptr<v3d::image::Image>> baseColours_;  /**< by material, and short when the last have none **/
};
};  // namespace v3d::asset::media::kind
