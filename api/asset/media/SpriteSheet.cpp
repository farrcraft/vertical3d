/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "SpriteSheet.h"

#include <api/image/Pack.h>

#include "SpriteRegion.h"

#include <map>
#include <string>
#include <vector>

#include <glm/vec4.hpp>

namespace v3d::asset::media {

/**
 **/
SpriteSheet::SpriteSheet() :
width_(0),
height_(0) {
}

/**
 **/
SpriteSheet::SpriteSheet(const Spec& spec) :
name_(spec.name),
image_(spec.image),
width_(spec.width),
height_(spec.height) {
}

/**
 **/
SpriteSheet SpriteSheet::packed(const std::string& name, const std::string& image, const v3d::image::Packing& packing) {
    if (!packing.sheet) {
        return SpriteSheet();
    }
    SpriteSheet sheet({.name = name, .image = image,
        .width = static_cast<int>(packing.sheet->width()), .height = static_cast<int>(packing.sheet->height())});
    for (const v3d::image::Placement& placement : packing.placements) {
        SpriteRegion region;
        region.x = static_cast<int>(placement.area.x);
        region.y = static_cast<int>(placement.area.y);
        region.width = static_cast<int>(placement.area.z);
        region.height = static_cast<int>(placement.area.w);
        // pack() places every sprite inside the sheet it returns, so place() takes each one
        sheet.place(placement.name, region);
    }
    return sheet;
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
    // a region running off the sheet would give a uv outside 0..1. The far edge is measured
    // as the room left after the corner, because x + width can overflow an int.
    if (sprite.empty() || region.width <= 0 || region.height <= 0 ||
        region.x < 0 || region.y < 0 || region.x > width_ || region.y > height_ ||
        region.width > width_ - region.x || region.height > height_ - region.y) {
        return false;
    }
    if (regions_.emplace(sprite, region).second) {
        sprites_.push_back(sprite);
    }
    return true;
}

/**
 **/
std::vector<v3d::image::Placement> SpriteSheet::placements() const {
    std::vector<v3d::image::Placement> placements;
    placements.reserve(sprites_.size());
    for (const std::string& sprite : sprites_) {
        // place() admits no negative corner or size, so every value converts unchanged
        const SpriteRegion& region = regions_.at(sprite);
        placements.push_back({.name = sprite, .area = glm::uvec4(static_cast<unsigned int>(region.x),
            static_cast<unsigned int>(region.y), static_cast<unsigned int>(region.width), static_cast<unsigned int>(region.height))});
    }
    return placements;
}

};  // namespace v3d::asset::media
