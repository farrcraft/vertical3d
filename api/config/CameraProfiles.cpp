/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "CameraProfiles.h"

#include <api/asset/Json.h>

#include <optional>
#include <string>
#include <vector>

#include <boost/json.hpp>

namespace v3d::config {

namespace {

/**
 * Read a three element array as a vector.
 * @return the value read, or the fallback if the entry is missing, the wrong length, or holds
 *         something other than numbers
 **/
glm::vec3 vector(const boost::json::object& entry, const char* key, const glm::vec3& fallback) {
    const boost::json::array* values = v3d::asset::readArray(entry, key);
    if (values == nullptr) {
        return fallback;
    }
    const boost::json::array& list = *values;
    if (list.size() != 3 || !list[0].is_number() || !list[1].is_number() || !list[2].is_number()) {
        return fallback;
    }
    return glm::vec3(
        static_cast<float>(list[0].to_number<double>()),
        static_cast<float>(list[1].to_number<double>()),
        static_cast<float>(list[2].to_number<double>()));
}

/**
 * @return the value read, or the fallback if the entry is missing or not a number
 **/
float number(const boost::json::object& entry, const char* key, float fallback) {
    const std::optional<double> value = v3d::asset::readNumber(entry, key);
    return value ? static_cast<float>(*value) : fallback;
}

/**
 * @return the value read, or the fallback if the entry is missing or not a bool
 **/
bool flag(const boost::json::object& entry, const char* key, bool fallback) {
    return v3d::asset::readBool(entry, key).value_or(fallback);
}

};  // namespace

/**
 **/
CameraProfiles::CameraProfiles(const boost::shared_ptr<v3d::log::Logger>& logger) :
    logger_(logger) {
}

/**
 **/
bool CameraProfiles::load(const boost::json::object& doc) {
    if (!doc.contains("cameras") || !doc.at("cameras").is_array()) {
        logger_->get()->error("Missing cameras in the camera config");
        return false;
    }

    for (auto const& value : doc.at("cameras").as_array()) {
        if (!value.is_object()) {
            logger_->get()->error("Unrecognized camera profile");
            return false;
        }
        auto const entry = value.as_object();
        const std::optional<std::string> named = v3d::asset::readString(entry, "name");
        if (!named) {
            logger_->get()->error("A camera profile needs a name");
            return false;
        }
        const std::optional<std::string> adaptiveRead = v3d::asset::readString(entry, "adaptive");
        if (entry.contains("adaptive") && !adaptiveRead) {
            logger_->get()->error("A camera profile's adaptive is none, projection, position or both");
            return false;
        }
        const std::string& name = *named;

        v3d::type::camera::Profile profile(name);
        profile.orthographic(flag(entry, "orthographic", true));
        profile.orthoZoom(number(entry, "zoom", 10.0f));
        profile.fov(number(entry, "fov", 60.0f));
        profile.pixelAspect(number(entry, "aspect", 1.33f));
        profile.clipping(number(entry, "near", 0.1f), number(entry, "far", 100.0f));

        const std::string adaptive = adaptiveRead.value_or("none");
        profile.adaptiveProjection(adaptive == "both" || adaptive == "projection");
        profile.adaptivePosition(adaptive == "both" || adaptive == "position");

        profile.eye(vector(entry, "eye", glm::vec3(0.0f, 0.0f, -10.0f)));
        // up before lookat: lookat starts from the up vector and derives the other two
        // and the rotation from it, so an up that arrives afterwards is overwritten
        profile.up(vector(entry, "up", glm::vec3(0.0f, 1.0f, 0.0f)));
        profile.lookat(vector(entry, "lookat", glm::vec3(0.0f, 0.0f, 0.0f)));

        if (!profiles_.contains(name)) {
            names_.push_back(name);
        }
        profiles_.insert_or_assign(name, profile);
    }

    return true;
}

/**
 **/
v3d::type::camera::Profile CameraProfiles::get(const std::string& name) const {
    auto entry = profiles_.find(name);
    if (entry == profiles_.end()) {
        return v3d::type::camera::Profile(name);
    }
    return entry->second;
}

/**
 **/
bool CameraProfiles::has(const std::string& name) const {
    return profiles_.contains(name);
}

/**
 **/
const std::vector<std::string>& CameraProfiles::names() const noexcept {
    return names_;
}

};  // namespace v3d::config
