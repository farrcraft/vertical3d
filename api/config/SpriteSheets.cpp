/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "SpriteSheets.h"

#include <map>
#include <string>
#include <vector>

namespace v3d::config {

namespace {

/**
 * @return the value read, or the fallback if the entry is missing or not a whole number
 **/
int whole(const boost::json::object& entry, const char* key, int fallback) {
    if (!entry.contains(key) || !entry.at(key).is_number()) {
        return fallback;
    }
    boost::system::error_code error;
    const int value = entry.at(key).to_number<int>(error);
    return error ? fallback : value;
}

/**
 * @return the value read, or an empty string if the entry is missing or not a string
 **/
std::string text(const boost::json::object& entry, const char* key) {
    if (!entry.contains(key) || !entry.at(key).is_string()) {
        return std::string();
    }
    return boost::json::value_to<std::string>(entry.at(key));
}

};  // namespace

/**
 **/
SpriteRegion::SpriteRegion() noexcept :
x(0),
y(0),
width(0),
height(0) {
}

/**
 **/
SpriteSheet::SpriteSheet() noexcept :
width_(0),
height_(0) {
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
SpriteSheets::SpriteSheets(const boost::shared_ptr<v3d::log::Logger>& logger) :
    logger_(logger) {
}

/**
 **/
bool SpriteSheets::load(const boost::shared_ptr<v3d::asset::Json>& config) {
    if (!config) {
        return false;
    }
    const boost::json::object doc = config->document();
    if (!doc.contains("sheets") || !doc.at("sheets").is_array()) {
        logger_->get()->error("Missing sheets in the sprite config");
        return false;
    }

    bool understood = true;
    for (const boost::json::value& value : doc.at("sheets").as_array()) {
        if (!value.is_object()) {
            logger_->get()->error("Unrecognized sprite sheet");
            understood = false;
            continue;
        }
        const boost::json::object entry = value.as_object();

        SpriteSheet sheet;
        sheet.name_ = text(entry, "name");
        sheet.image_ = text(entry, "image");
        sheet.width_ = whole(entry, "width", 0);
        sheet.height_ = whole(entry, "height", 0);
        if (sheet.name_.empty() || sheet.image_.empty() || sheet.width_ <= 0 || sheet.height_ <= 0) {
            logger_->get()->error("A sprite sheet needs a name, an image and a size");
            understood = false;
            continue;
        }

        if (entry.contains("sprites") && entry.at("sprites").is_array()) {
            for (const boost::json::value& item : entry.at("sprites").as_array()) {
                if (!item.is_object()) {
                    logger_->get()->error("Unrecognized sprite in sheet {}", sheet.name_);
                    understood = false;
                    continue;
                }
                const boost::json::object record = item.as_object();
                const std::string named = text(record, "name");
                SpriteRegion region;
                region.x = whole(record, "x", 0);
                region.y = whole(record, "y", 0);
                region.width = whole(record, "width", 0);
                region.height = whole(record, "height", 0);

                // a region running off the sheet would give a uv outside 0..1, which samples
                // whatever the wrap mode decides rather than reporting anything
                if (named.empty() || region.width <= 0 || region.height <= 0 ||
                    region.x < 0 || region.y < 0 ||
                    region.x + region.width > sheet.width_ ||
                    region.y + region.height > sheet.height_) {
                    logger_->get()->error("Sprite {} does not fit sheet {}", named, sheet.name_);
                    understood = false;
                    continue;
                }
                if (sheet.regions_.emplace(named, region).second) {
                    sheet.sprites_.push_back(named);
                }
            }
        }

        if (sheets_.emplace(sheet.name_, sheet).second) {
            names_.push_back(sheet.name_);
        }
    }
    return understood;
}

/**
 **/
SpriteSheet SpriteSheets::get(const std::string& name) const {
    const std::map<std::string, SpriteSheet>::const_iterator found = sheets_.find(name);
    return found == sheets_.end() ? SpriteSheet() : found->second;
}

/**
 **/
bool SpriteSheets::has(const std::string& name) const {
    return sheets_.find(name) != sheets_.end();
}

/**
 **/
const std::vector<std::string>& SpriteSheets::names() const noexcept {
    return names_;
}

};  // namespace v3d::config
