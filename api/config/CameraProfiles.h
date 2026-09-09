/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/asset/kind/Json.h>
#include <api/log/Logger.h>
#include <api/type/CameraProfile.h>

#include <map>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::config {

/**
 * Named camera profiles, loaded from a document the config names as Type::Camera - the
 * editor's data/cameras.json is the one in the tree.
 *
 * A profile is described by where the camera is and what it looks at rather than by
 * its three normals: the basis and the rotation have to agree, and
 * CameraProfile::lookat() is the one call that writes all four consistently.
 **/
class CameraProfiles final {
 public:
    /**
     * @param logger
     **/
    explicit CameraProfiles(const boost::shared_ptr<v3d::log::Logger>& logger);

    /**
     * Read every profile in the document.
     * A profile missing a name is rejected; every other field has a default.
     *
     * @param config the parsed cameras.json
     * @return whether the table was understood
     **/
    bool load(const boost::shared_ptr<v3d::asset::kind::Json>& config);

    /**
     * @param name the profile name, as whatever names a view names it
     * @return the profile, or an unnamed default when there is no such profile
     **/
    v3d::type::CameraProfile get(const std::string& name) const;

    /**
     * @return whether a profile of that name was loaded
     **/
    bool has(const std::string& name) const;

    /**
     * @return the profile names, in the order the document listed them
     **/
    const std::vector<std::string>& names() const noexcept;

 private:
    boost::shared_ptr<v3d::log::Logger> logger_;
    std::map<std::string, v3d::type::CameraProfile> profiles_;
    std::vector<std::string> names_;
};

};  // namespace v3d::config
