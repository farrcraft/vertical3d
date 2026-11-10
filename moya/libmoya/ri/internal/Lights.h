/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/log/Logger.h>
#include <api/render/offline/rib/Parameters.h>
#include <api/render/offline/rib/requests/LightSource.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Scene.h>

#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/vec3.hpp>

#include "Objects.h"
#include "Sampled.h"
#include "Warnings.h"

namespace v3d::moya::ri::internal {

/**
 * The lights a world makes from RI's light shaders, and the instances each one lights.
 *
 * `pointlight`, `distantlight` and `spotlight` become a sphere light of radius zero, a distant
 * light and a sphere light of radius zero shaped by a cone. Each points down its `to` from its
 * `from`. A sphere light of radius zero is normalised, with four times RI's intensity, so its
 * radiant intensity is RI's. Another light shader is logged and makes no light. An area light
 * makes a mesh light of a geometry.
 **/
class Lights final {
 public:
    /** The scene, the objects and the warnings outlive the lights. **/
    Lights(scene::Scene * scene, Objects * objects, Warnings * warnings, const boost::shared_ptr<v3d::log::Logger> & logger);

    /**
     * Forget the lights and the instances they lit, for a new world. Objects::clear() removes
     * the lights from the scene.
     **/
    void clear();

    /**
     * Make the light a LightSource request names, at a transformation.
     *
     * @param name what the light is called before it is made unique
     * @return whether a light was made, so that it is on
     **/
    bool source(const v3d::render::offline::rib::requests::LightSource & request, const v3d::render::offline::rib::ParameterList & parameters,
        const std::string & name, const Sampled & transformation);

    /**
     * Whether Illuminate can turn a light on or off. A light whose shader made nothing is
     * ignored, and a handle no light holds is logged once.
     **/
    bool named(const std::string & handle);

    /** Record an instance that is lit, and the lights that are on for it. **/
    void lit(const scene::Handle & instance, const std::set<std::string> & on);

    /** Make a mesh light of a geometry, from an area light's shader name and parameters. **/
    void glow(const scene::Handle & geometry, const std::string & name, const v3d::render::offline::rib::ParameterList & parameters,
        const Sampled & transformation);

    /**
     * Give each light the instances it lit. A light that lit every instance keeps `includeRoot`.
     * Any other clears it and names the instances made while it was on, which may be none.
     **/
    void finish();

 private:
    /** A light the world made, and the instances made while it was on. **/
    struct Light {
        scene::Handle light{};
        std::vector<scene::Handle> lit{};
    };

    /** RI's spotlight defaults, in radians. **/
    static constexpr float kConeAngle = 0.5235988F;
    static constexpr float kConeDelta = 0.0872665F;

    /** A point a light's parameters give, or the fallback when they give none. **/
    static glm::vec3 pointOf(const v3d::render::offline::rib::ParameterList & parameters, const std::string & name, const glm::vec3 & fallback);

    /** A light at a transformation, from `from` towards `to`. **/
    scene::Handle light(const std::string & kind, const std::string & name, const v3d::render::offline::rib::ParameterList & parameters,
        const glm::vec3 & defaultTo, const Sampled & transformation);

    scene::Scene * scene_;
    Objects * objects_;
    Warnings * warnings_;
    boost::shared_ptr<v3d::log::Logger> logger_;

    std::map<std::string, Light> lights_;
    /** The handles of lights a scene declared and nothing was made of. **/
    std::set<std::string> unmade_;
    /** How many lit instances the world holds. **/
    std::size_t instances_ { 0 };
};

};  // namespace v3d::moya::ri::internal
