/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Texture.h"

#include <api/image/Factory.h>

#include <cmath>
#include <string>
#include <vector>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/make_shared.hpp>

#include "SearchPath.h"

namespace v3d::render::offline {

namespace {

/** A texel index wrapped into [0, size), negative ones included. **/
int wrap(int index, unsigned int size) {
    const int span = static_cast<int>(size);
    const int wrapped = index % span;
    return wrapped < 0 ? wrapped + span : wrapped;
}

};  // namespace

Texture::Texture(const v3d::image::Image & image) :
    width_(image.width()), height_(image.height()) {
    const unsigned int channels = image.bpp() / 8u;
    texels_.reserve(static_cast<std::size_t>(width_) * height_);
    for (unsigned int i = 0; i < width_ * height_; i++) {
        const unsigned int at = i * channels;
        glm::vec3 colour(image[at] / 255.0f);
        if (channels >= 3) {
            colour.g = image[at + 1] / 255.0f;
            colour.b = image[at + 2] / 255.0f;
        }
        texels_.push_back(colour);
    }
}

unsigned int Texture::width() const {
    return width_;
}

unsigned int Texture::height() const {
    return height_;
}

const glm::vec3 & Texture::texel(int column, int row) const {
    return texels_[static_cast<std::size_t>(wrap(row, height_)) * width_ +
        static_cast<std::size_t>(wrap(column, width_))];
}

glm::vec3 Texture::sample(float s, float t) const {
    if (texels_.empty()) {
        return glm::vec3(0.0f);
    }
    // texel centres are at half a texel in, so a position is measured from the first one
    const float x = s * static_cast<float>(width_) - 0.5f;
    const float y = t * static_cast<float>(height_) - 0.5f;
    const float left = std::floor(x);
    const float top = std::floor(y);
    const float across = x - left;
    const float down = y - top;
    const int column = static_cast<int>(left);
    const int row = static_cast<int>(top);
    const glm::vec3 upper = texel(column, row) * (1.0f - across) + texel(column + 1, row) * across;
    const glm::vec3 lower = texel(column, row + 1) * (1.0f - across) + texel(column + 1, row + 1) * across;
    return upper * (1.0f - down) + lower * down;
}

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
