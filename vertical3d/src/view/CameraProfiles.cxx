/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "CameraProfiles.h"

#include <string>
#include <vector>

namespace v3d::editor {

namespace {

/**
 * Read a three element array as a vector.
 * @return the value read, or the fallback if the entry is missing or the wrong shape
 **/
glm::vec3 vector(const boost::json::object& entry, const char* key, const glm::vec3& fallback) {
    if (!entry.contains(key) || !entry.at(key).is_array()) {
        return fallback;
    }
    const boost::json::array& values = entry.at(key).as_array();
    if (values.size() != 3) {
        return fallback;
    }
    return glm::vec3(
        static_cast<float>(values[0].to_number<double>()),
        static_cast<float>(values[1].to_number<double>()),
        static_cast<float>(values[2].to_number<double>()));
}

/**
 * @return the value read, or the fallback if the entry is missing or not a number
 **/
float number(const boost::json::object& entry, const char* key, float fallback) {
    if (!entry.contains(key) || !entry.at(key).is_number()) {
        return fallback;
    }
    return static_cast<float>(entry.at(key).to_number<double>());
}

/**
 * @return the value read, or the fallback if the entry is missing or not a bool
 **/
bool flag(const boost::json::object& entry, const char* key, bool fallback) {
    if (!entry.contains(key) || !entry.at(key).is_bool()) {
        return fallback;
    }
    return entry.at(key).as_bool();
}

};  // namespace

/**
 **/
CameraProfiles::CameraProfiles(const boost::shared_ptr<v3d::log::Logger>& logger) :
    logger_(logger) {
}

/**
 **/
bool CameraProfiles::load(const boost::shared_ptr<v3d::asset::Json>& config) {
    if (!config) {
        return false;
    }
    auto const doc = config->document();
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
        if (!entry.contains("name")) {
            logger_->get()->error("A camera profile needs a name");
            return false;
        }
        std::string name = boost::json::value_to<std::string>(entry.at("name"));

        v3d::type::CameraProfile profile(name);
        profile.orthographic(flag(entry, "orthographic", true));
        profile.orthoZoom(number(entry, "zoom", 10.0f));
        profile.fov(number(entry, "fov", 60.0f));
        profile.pixelAspect(number(entry, "aspect", 1.33f));
        profile.clipping(number(entry, "near", 0.1f), number(entry, "far", 100.0f));

        const std::string adaptive = entry.contains("adaptive") ?
            boost::json::value_to<std::string>(entry.at("adaptive")) : std::string("none");
        profile.adaptiveProjection(adaptive == "both" || adaptive == "projection");
        profile.adaptivePosition(adaptive == "both" || adaptive == "position");

        profile.eye(vector(entry, "eye", glm::vec3(0.0f, 0.0f, -10.0f)));
        // up before lookat: lookat starts from the up vector and derives the other two
        // and the rotation from it, so an up that arrives afterwards is overwritten
        profile.up(vector(entry, "up", glm::vec3(0.0f, 1.0f, 0.0f)));
        profile.lookat(vector(entry, "lookat", glm::vec3(0.0f, 0.0f, 0.0f)));

        if (profiles_.find(name) == profiles_.end()) {
            names_.push_back(name);
        }
        profiles_.insert_or_assign(name, profile);
    }

    return true;
}

/**
 **/
v3d::type::CameraProfile CameraProfiles::get(const std::string& name) const {
    auto entry = profiles_.find(name);
    if (entry == profiles_.end()) {
        return v3d::type::CameraProfile(name);
    }
    return entry->second;
}

/**
 **/
bool CameraProfiles::has(const std::string& name) const {
    return profiles_.find(name) != profiles_.end();
}

/**
 **/
const std::vector<std::string>& CameraProfiles::names() const noexcept {
    return names_;
}

};  // namespace v3d::editor
