/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Loaders.h"

#include <api/asset/media/loader/Gltf.h>
#include <api/asset/media/loader/Image.h>
#include <api/asset/media/loader/TextureFont.h>

#include <boost/make_shared.hpp>

namespace v3d::asset::media {

void registerLoaders(Manager& manager, const boost::shared_ptr<v3d::log::Logger>& logger) {
    manager.registerLoader(boost::make_shared<loader::Image>(Type::ImagePng, logger), {".png"});
    manager.registerLoader(boost::make_shared<loader::Image>(Type::ImageJpeg, logger), {".jpg", ".jpeg"});
    manager.registerLoader(boost::make_shared<loader::Image>(Type::ImageTga, logger), {".tga"});
    manager.registerLoader(boost::make_shared<loader::Image>(Type::ImageBmp, logger), {".bmp"});
    manager.registerLoader(boost::make_shared<loader::Gltf>(logger), {".gltf", ".glb"});
    // a typeface is loaded by type and never by extension: the loader needs a size to load
    // at, which a file name does not carry
    manager.registerLoader(boost::make_shared<loader::TextureFont>(logger), {});
}

};  // namespace v3d::asset::media
