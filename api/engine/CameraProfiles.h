/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>
#include <api/type/camera/Profile.h>

#include <map>
#include <string>
#include <string_view>
#include <vector>

#include <boost/json/object.hpp>
#include <boost/shared_ptr.hpp>
#include <glm/vec3.hpp>

namespace v3d::engine {

/**
 * Named camera profiles, loaded from the document config.json names under DOCUMENT, such as
 * vertical3d/data/cameras.json.
 *
 * A profile entry gives the camera's eye, look-at point and up vector, not its basis vectors.
 * The three basis vectors and the rotation must agree, and only Profile::lookat() sets all
 * four together.
 **/
class CameraProfiles final {
 public:
    /**
     * The name config.json gives the kind of a camera document.
     **/
    static constexpr std::string_view DOCUMENT = "camera";

    /**
     * @param logger
     **/
    explicit CameraProfiles(const boost::shared_ptr<v3d::log::Logger>& logger);

    /**
     * Read every profile in the document.
     * The load fails at a profile with no name, an "adaptive" that is not a string, or a zoom
     * below Camera::MINIMUM_ZOOM. Profiles read before that one stay loaded. Every other field
     * has a default.
     *
     * @param doc the parsed cameras.json
     * @return whether every profile was understood
     **/
    bool load(const boost::json::object& doc);

    /**
     * @param name the profile name
     * @return the profile, or a default profile with that name when none was loaded
     **/
    v3d::type::camera::Profile get(const std::string& name) const;

    /**
     * @return whether a profile of that name was loaded
     **/
    bool has(const std::string& name) const;

    /**
     * @return the profile names, in the order the document listed them
     **/
    const std::vector<std::string>& names() const noexcept;

 private:
    // the values a profile entry gets when it leaves these out, where they differ from a new
    // Profile
    static constexpr float ENTRY_ZOOM = 10.0F;
    static constexpr float ENTRY_NEAR = 0.1F;
    static constexpr glm::vec3 ENTRY_EYE{0.0F, 0.0F, -10.0F};

    /**
     * Read a three element array as a vector.
     * @return the value read, or the fallback if the entry is missing, the wrong length, or holds
     *         something other than numbers
     **/
    static glm::vec3 vector(const boost::json::object& entry, const char* key, const glm::vec3& fallback);
    /**
     * @return the value read, or the fallback if the entry is missing or not a number
     **/
    static float number(const boost::json::object& entry, const char* key, float fallback);
    /**
     * @return the value read, or the fallback if the entry is missing or not a bool
     **/
    static bool flag(const boost::json::object& entry, const char* key, bool fallback);

    boost::shared_ptr<v3d::log::Logger> logger_;
    std::map<std::string, v3d::type::camera::Profile> profiles_;
    std::vector<std::string> names_;
};

};  // namespace v3d::engine
