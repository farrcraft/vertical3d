/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/Camera.h>
#include <api/render/offline/MovingTransform.h>

#include <map>
#include <string>
#include <vector>

#include <glm/mat4x4.hpp>

namespace v3d::moya {

/**
 * The current transformation, the transformations TransformBegin saved, and the coordinate
 * systems a scene named.
 **/
class Transforms final {
 public:
    /** The current transformation, which moves inside a motion block. **/
    v3d::render::offline::MovingTransform & current();
    const v3d::render::offline::MovingTransform & current() const;

    /** RiTransformBegin and RiTransformEnd. A pop with nothing saved does nothing. **/
    void push();
    void pop();

    /** Whether a name is one of the spaces the camera defines, which a scene cannot save over. **/
    static bool predefined(const std::string & name);
    /**
     * RiCoordinateSystem: name the current transformation, which is then the named space's
     * transformation into world space.
     *
     * @return false, saving nothing, for a predefined name
     **/
    bool save(const std::string & name);
    /** The spaces a scene named, each held as its transformation into world space. **/
    const std::map<std::string, glm::mat4x4> & named() const;
    /** Whether a name is predefined or named by the scene. **/
    bool known(const std::string & name) const;
    /**
     * The matrix of a coordinate system. "camera" is world to camera, and "screen", "raster"
     * and "NDC" are each from camera space, as the camera holds them. "object" and "world" are
     * the identity. A named space is its transformation into world space.
     *
     * @return false, leaving matrix untouched, for a name that is neither predefined nor named
     **/
    bool space(const std::string & name, const v3d::render::offline::Camera & camera, glm::mat4x4* matrix) const;

 private:
    v3d::render::offline::MovingTransform current_;
    std::vector<v3d::render::offline::MovingTransform> saved_;
    std::map<std::string, glm::mat4x4> named_;
};

};  // namespace v3d::moya
