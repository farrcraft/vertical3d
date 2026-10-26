/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace v3d::render::offline {

/**
 * The camera a picture is taken through, as WorldBegin fixes it.
 *
 * Every matrix but the view maps out of camera space, and each is the identity until a
 * renderer sets the camera. A renderer builds one from CameraOptions; a test may fill one in
 * directly.
 **/
class Camera final {
 public:
    /** World to camera. It defines a point's "camera" space, and the position of E. **/
    glm::mat4x4 view = glm::mat4x4(1.0F);
    /** Camera to screen: the projection, with the visible picture over [-1, 1] in x and y. **/
    glm::mat4x4 screen = glm::mat4x4(1.0F);
    /** Camera to raster: pixels, x to the right and y down from the picture's upper left. **/
    glm::mat4x4 raster = glm::mat4x4(1.0F);
    /** Camera to NDC: raster divided by the resolution, over [0, 1], with z [0, 1] near to far. **/
    glm::mat4x4 ndc = glm::mat4x4(1.0F);
    /** Whether the projection is "perspective". Only a perspective camera has a lens. **/
    bool perspective = false;
    /** The near clipping plane, in camera space z. A primary ray starts there. **/
    float hither = 0.0F;
    /** The picture's size in pixels. **/
    unsigned int width = 0;
    unsigned int height = 0;

    /** Where the camera is, in world space. **/
    glm::vec3 eye() const;

    /**
     * The matrix from world space into one of the spaces the camera defines: "world",
     * "camera", "screen", "raster" or "NDC".
     *
     * @return false, leaving matrix untouched, for any other name
     **/
    bool fromWorld(const std::string & name, glm::mat4x4* matrix) const;
    /**
     * The matrix from camera space into the same spaces. Each is built without going through
     * world space, so "camera" is exactly the identity.
     *
     * @return false, leaving matrix untouched, for any other name
     **/
    bool fromCamera(const std::string & name, glm::mat4x4* matrix) const;
};

};  // namespace v3d::render::offline
