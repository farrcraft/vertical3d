/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include "Asset.h"
#include "../image/Image.h"
#include "../type/Model.h"

#include <boost/shared_ptr.hpp>

namespace v3d::asset {
/**
 * Geometry loaded from a model file, as the asset manager hands it out.
 **/
class Model : public Asset {
 public:
    /**
     **/
    Model(const std::string& name, Type t, const boost::shared_ptr<v3d::type::Model>& model,
        const boost::shared_ptr<v3d::image::Image>& baseColour = boost::shared_ptr<v3d::image::Image>());

    /**
     **/
    boost::shared_ptr<v3d::type::Model> model();

    /**
     * The base colour texture, where the file carried its pixels rather than a name.
     *
     * A .glb packs its images into its own buffer and a .gltf may inline one as a data
     * uri, so there is no path for the material to name and type::Model::Material's
     * baseColourTexture is empty. Decoded pixels arrive here instead, for the app to
     * upload the way it uploads any other image.
     *
     * It is on the asset rather than on the material because it is a fact about how the
     * file was packaged rather than about the surface, and because api/type is built
     * against glm alone - a material holding an image would take api/image into every
     * consumer of a mesh, offline renderers included.
     *
     * Null for a model whose texture was named, which is every .gltf pointing at a file
     * beside it, and for one with no texture at all.
     **/
    boost::shared_ptr<v3d::image::Image> baseColourImage() const;

 private:
    boost::shared_ptr<v3d::type::Model> model_;
    boost::shared_ptr<v3d::image::Image> baseColour_;
};
};  // namespace v3d::asset
