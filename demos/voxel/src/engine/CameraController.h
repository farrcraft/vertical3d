/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/


#pragma once

#include <api/type/camera/Camera.h>

#include <glm/vec3.hpp>

/**
 * Drives a type::camera::Camera as a first person view: a heading about the world's up axis, a
 * pitch that stops at straight up and straight down, and movement with a velocity that gathers
 * and loses speed.
 *
 * The controller holds the heading, the pitch and the velocity. The camera holds the eye, the
 * orientation and the lens, and the controller writes them into it after every change, so its
 * view() and projection() are always current.
 *
 * The camera looks down -z at a heading and pitch of zero, with +y up and +x to the right. Its
 * profile is set to the hand glm::lookAt builds, which voxel's geometry is wound for.
 **/
class CameraController final {
 public:
    /** The lens a camera has until it is given one: degrees across, and the planes in blocks. **/
    static constexpr float DEFAULT_FOVX = 90.0F;
    static constexpr float DEFAULT_ZNEAR = 0.1F;
    static constexpr float DEFAULT_ZFAR = 1000.0F;

    /** The horizontal field of view in degrees, the aspect, and the near and far planes. **/
    struct Lens {
        float fovx = DEFAULT_FOVX;
        float aspect = 1.0F;
        float znear = DEFAULT_ZNEAR;
        float zfar = DEFAULT_ZFAR;
    };

    /** How fast the velocity changes, and the most it reaches, per axis of the view. **/
    struct Motion {
        glm::vec3 acceleration{0.0F};  /**< units a second squared **/
        glm::vec3 limit{0.0F};         /**< units a second **/
    };

    /**
     * Give the camera a perspective lens of the default size, at the origin, with no heading
     * and no pitch.
     *
     * @param camera the camera to drive. The caller owns it, and it outlives this
     **/
    CameraController(v3d::type::camera::Camera* camera, const Motion& motion);

    /**
     * Give the camera a perspective lens that keeps the horizontal field of view at any aspect.
     **/
    void perspective(const Lens& lens);

    /**
     * Turn the view. A positive heading turns it right, about the world's up axis, and a
     * positive pitch turns it down. The pitch stops at straight up and straight down, so the
     * view never rolls or turns over.
     **/
    void look(float headingDegrees, float pitchDegrees);

    /**
     * How far the current velocity carries the camera in a step, in the view's own axes: x to
     * the right, y up, and z forward.
     *
     * An axis the direction does not drive and whose velocity has reached zero does not move,
     * so rounding cannot creep the camera along it. At rest the camera does not move at all,
     * so a step from rest only gathers speed.
     *
     * @param direction which way each axis is driven, from -1 to 1
     * @param step the step's length in seconds
     **/
    glm::vec3 displacement(const glm::vec3& direction, float step) const;

    /**
     * Where the eye would be after a displacement() in the view's axes. Forward is along the
     * ground, whatever the pitch, and up is the world's up, so looking down does not slow a
     * walk.
     **/
    glm::vec3 moved(const glm::vec3& displacement) const;

    /**
     * Accelerate each axis the direction drives toward its limit, and slow each axis it does not
     * drive toward rest, over a step.
     *
     * @param direction which way each axis is driven, from -1 to 1
     * @param step the step's length in seconds
     **/
    void accelerate(const glm::vec3& direction, float step);

    /**
     * Put the eye somewhere, keeping the heading and the pitch.
     **/
    void place(const glm::vec3& eye);

    /**
     * @return the velocity, in units a second along the view's axes
     **/
    const glm::vec3& velocity() const noexcept;

 private:
    /**
     * Write the eye and the orientation into the camera, and rebuild its view.
     **/
    void update();

    v3d::type::camera::Camera* camera_;
    Motion motion_;
    float heading_ = 0.0F;
    float pitch_ = 0.0F;
    glm::vec3 velocity_{0.0F};
};
