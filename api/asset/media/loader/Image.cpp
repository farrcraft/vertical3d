/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
**/

#include "Image.h"

#include <api/asset/Asset.h>
#include <api/asset/File.h>
#include <api/asset/Loader.h>
#include <api/asset/media/kind/Image.h>
#include <api/image/Factory.h>
#include <api/image/Image.h>
#include <api/log/Logger.h>

#include <optional>
#include <string>
#include <string_view>

#include <boost/make_shared.hpp>

namespace v3d::asset::media::loader {

/**
 **/
Image::Image(const boost::shared_ptr<v3d::log::Logger>& logger) : Loader(typeid(kind::Image), logger) {
}

/**
 **/
boost::shared_ptr<Asset> Image::load(std::string_view name) {
    logger()->get()->info("Looking for image asset at: {}", name);
    const std::optional<std::string> encoded = readFile(name);
    if (!encoded) {
        logger()->get()->error("Could not read image asset: {}", name);
        return boost::shared_ptr<Asset>();
    }
    // the format is the one the extension names, the same as for a file read directly
    v3d::image::Factory factory(logger());
    const boost::shared_ptr<v3d::image::Image> image = factory.read(
        reinterpret_cast<const unsigned char*>(encoded->data()), encoded->size(), v3d::image::Factory::kind(name));
    // an asset holding no image looks like a loaded one until a consumer dereferences it,
    // so a failed read returns no asset at all
    if (!image) {
        logger()->get()->error("Could not decode image asset: {}", name);
        return boost::shared_ptr<Asset>();
    }
    return boost::make_shared<kind::Image>(std::string(name), image);
}
};  // namespace v3d::asset::media::loader
