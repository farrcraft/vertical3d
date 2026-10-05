/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Textures.h"

#include <api/image/Factory.h>
#include <api/image/Image.h>

#include <string>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

#include "SearchPath.h"

namespace v3d::render::offline {

Textures::Textures(const boost::shared_ptr<v3d::log::Logger> & logger) : logger_(logger) {
}

void Textures::searchpath(const std::string & path) {
    directories_ = offline::searchpath(path, directories_);
}

const Texture* Textures::find(const std::string & name) {
    const auto held = textures_.find(name);
    if (held != textures_.end()) {
        return held->second.get();
    }

    std::string path;
    if (!boost::filesystem::path(name).is_absolute()) {
        for (const std::string & directory : directories_) {
            const boost::filesystem::path candidate = boost::filesystem::path(directory) / name;
            if (boost::filesystem::exists(candidate)) {
                path = candidate.string();
                break;
            }
        }
    }
    if (path.empty() && boost::filesystem::exists(name)) {
        path = name;
    }

    boost::shared_ptr<Texture> texture;
    if (!path.empty()) {
        v3d::image::Factory factory(logger_);
        const boost::shared_ptr<v3d::image::Image> image = factory.read(path);
        if (image && image->width() > 0 && image->height() > 0 && image->bpp() >= 8) {
            texture = boost::make_shared<Texture>(*image);
        }
    }
    if (!texture) {
        logger_->get()->error("the texture \"{}\" cannot be read", name);
    }
    textures_[name] = texture;
    return texture.get();
}

};  // namespace v3d::render::offline
