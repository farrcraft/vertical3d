/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Loaders.h"

#include <api/asset/media/loader/Gltf.h>
#include <api/asset/media/loader/Image.h>

#include <boost/make_shared.hpp>

namespace v3d::asset::media {

void registerLoaders(Manager& manager, const boost::shared_ptr<v3d::log::Logger>& logger) {
    manager.registerLoader(boost::make_shared<loader::Image>(Type::ImagePng, logger), {".png"});
    manager.registerLoader(boost::make_shared<loader::Image>(Type::ImageJpeg, logger), {".jpg", ".jpeg"});
    manager.registerLoader(boost::make_shared<loader::Image>(Type::ImageTga, logger), {".tga"});
    manager.registerLoader(boost::make_shared<loader::Image>(Type::ImageBmp, logger), {".bmp"});
    manager.registerLoader(boost::make_shared<loader::Gltf>(logger), {".gltf", ".glb"});
}

};  // namespace v3d::asset::media
