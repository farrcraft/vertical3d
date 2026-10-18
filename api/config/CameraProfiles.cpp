/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "CameraProfiles.h"

#include <api/asset/Json.h>
#include <api/log/Logger.h>
#include <api/type/camera/Camera.h>
#include <api/type/camera/Profile.h>

#include <optional>
#include <string>
#include <vector>

#include <boost/json.hpp>

namespace v3d::config {

namespace {

// the values a profile entry gets when it leaves these out, where they differ from a new Profile
constexpr float ENTRY_ZOOM = 10.0F;
constexpr float ENTRY_NEAR = 0.1F;
constexpr glm::vec3 ENTRY_EYE{0.0F, 0.0F, -10.0F};

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

        // the zoom is the orthographic half height. A zoom drag scales each step by the half
        // height, so one below Camera::MINIMUM_ZOOM could never zoom back out
        const float zoom = number(entry, "zoom", ENTRY_ZOOM);
        if (zoom < v3d::type::camera::Camera::MINIMUM_ZOOM) {
            logger_->get()->error("Camera profile {} has a zoom of {}, which is below {}", name, zoom,
                v3d::type::camera::Camera::MINIMUM_ZOOM);
            return false;
        }

        v3d::type::camera::Profile profile(name);
        profile.orthographic(flag(entry, "orthographic", true));
        profile.orthoZoom(zoom);
        profile.fov(number(entry, "fov", v3d::type::camera::Profile::DEFAULT_FOV));
        profile.pixelAspect(number(entry, "aspect", v3d::type::camera::Profile::DEFAULT_PIXEL_ASPECT));
        profile.clipping(glm::vec2(number(entry, "near", ENTRY_NEAR), number(entry, "far", v3d::type::camera::Profile::DEFAULT_FAR)));

        const std::string adaptive = adaptiveRead.value_or("none");
        profile.adaptiveProjection(adaptive == "both" || adaptive == "projection");
        profile.adaptivePosition(adaptive == "both" || adaptive == "position");

        profile.eye(vector(entry, "eye", ENTRY_EYE));
        // set up before lookat. lookat() derives the other two basis vectors and the rotation
        // from the up vector, so an up set afterwards would not match them
        profile.up(vector(entry, "up", glm::vec3(0.0F, 1.0F, 0.0F)));
        profile.lookat(vector(entry, "lookat", glm::vec3(0.0F, 0.0F, 0.0F)));

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
