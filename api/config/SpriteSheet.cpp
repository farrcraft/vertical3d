/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "SpriteSheet.h"

#include <map>
#include <string>
#include <vector>

namespace v3d::config {

/**
 **/
SpriteSheet::SpriteSheet() :
width_(0),
height_(0) {
}

/**
 **/
SpriteSheet::SpriteSheet(const std::string& name, const std::string& image, int width, int height) :
name_(name),
image_(image),
width_(width),
height_(height) {
}

/**
 **/
const std::string& SpriteSheet::name() const noexcept {
    return name_;
}

/**
 **/
const std::string& SpriteSheet::image() const noexcept {
    return image_;
}

/**
 **/
int SpriteSheet::width() const noexcept {
    return width_;
}

/**
 **/
int SpriteSheet::height() const noexcept {
    return height_;
}

/**
 **/
bool SpriteSheet::has(const std::string& sprite) const {
    return regions_.find(sprite) != regions_.end();
}

/**
 **/
SpriteRegion SpriteSheet::get(const std::string& sprite) const {
    const std::map<std::string, SpriteRegion>::const_iterator found = regions_.find(sprite);
    return found == regions_.end() ? SpriteRegion() : found->second;
}

/**
 **/
const std::vector<std::string>& SpriteSheet::sprites() const noexcept {
    return sprites_;
}

/**
 **/
bool SpriteSheet::uv(const std::string& sprite, glm::vec2* uv0, glm::vec2* uv1) const {
    if (uv0 == nullptr || uv1 == nullptr || width_ <= 0 || height_ <= 0) {
        return false;
    }
    const std::map<std::string, SpriteRegion>::const_iterator found = regions_.find(sprite);
    if (found == regions_.end()) {
        return false;
    }

    const float across = static_cast<float>(width_);
    const float down = static_cast<float>(height_);
    *uv0 = glm::vec2(static_cast<float>(found->second.x) / across,
        static_cast<float>(found->second.y) / down);
    *uv1 = glm::vec2(static_cast<float>(found->second.x + found->second.width) / across,
        static_cast<float>(found->second.y + found->second.height) / down);
    return true;
}

/**
 **/
bool SpriteSheet::place(const std::string& sprite, const SpriteRegion& region) {
    // a region running off the sheet would give a uv outside 0..1, which samples whatever the
    // wrap mode decides rather than reporting anything
    if (sprite.empty() || region.width <= 0 || region.height <= 0 ||
        region.x < 0 || region.y < 0 ||
        region.x + region.width > width_ || region.y + region.height > height_) {
        return false;
    }
    if (regions_.emplace(sprite, region).second) {
        sprites_.push_back(sprite);
    }
    return true;
}

};  // namespace v3d::config
