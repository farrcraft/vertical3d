/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Loaders.h"

#include <api/asset/Manager.h>
#include <api/asset/media/loader/Gltf.h>
#include <api/asset/media/loader/Image.h>
#include <api/log/Logger.h>

#include <boost/make_shared.hpp>

namespace v3d::asset::media {

void registerLoaders(Manager& manager, const boost::shared_ptr<v3d::log::Logger>& logger) {
    manager.registerLoader(boost::make_shared<loader::Image>(logger), {".png", ".jpg", ".jpeg", ".tga", ".bmp"});
    manager.registerLoader(boost::make_shared<loader::Gltf>(logger), {".gltf", ".glb"});
}

};  // namespace v3d::asset::media
