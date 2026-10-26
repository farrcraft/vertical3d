/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/Area.h>
#include <api/render/offline/sl/Placed.h>

#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya {

/**
 * The attributes a primitive is submitted under: everything RiAttributeBegin saves and
 * RiAttributeEnd puts back, but the transformation, which Transforms holds.
 *
 * The whole value is saved and restored, so an attribute added here is restored with the rest.
 **/
class Attributes final {
 public:
    /** RiColor: the colour a primitive carries. A vertex with a "Cs" of its own keeps it. **/
    glm::vec3 color = glm::vec3(1.0F);
    /** RiOpacity. **/
    glm::vec3 opacity = glm::vec3(1.0F);
    /** RiShadingRate: the area of one micropolygon, in pixels. **/
    float shadingRate = 1.0F;
    /** RiSurface, placed in camera space. No shader is the default, "constant". **/
    v3d::render::offline::sl::Placed surface;
    /** RiDisplacement, placed in camera space, and how far it may move a point there. **/
    v3d::render::offline::sl::Placed displacement;
    float displacementBound = 0.0F;
    /**
     * Which lights are switched on, by handle. The lights themselves last for the frame, so
     * AttributeEnd restores only this set.
     **/
    std::vector<std::string> lit;
    /** The area light the geometry being declared joins, or null. **/
    boost::shared_ptr<v3d::render::offline::Area> areaLight;
    /** `Attribute "light" "nsamples"`, which an area light takes when it is made. **/
    unsigned int lightSamples = 16;

    /** RiIlluminate: switch a light on or off. **/
    void illuminate(const std::string & handle, bool on);
    /** Whether a light is switched on. **/
    bool shines(const std::string & handle) const;
};

};  // namespace v3d::moya
