/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/


#pragma once

#include <cstdint>
#include <utility>

#include "Aligned.h"

#include <glm/glm.hpp>

/**
 * Player camera class
 * Based on the source here: http://www.dhpoware.com/demos/index.html
 */
class Camera : public Aligned<16> {
 public:
    typedef enum CameraBehavior : std::uint8_t {
        CAMERA_BEHAVIOR_FIRST_PERSON,
        CAMERA_BEHAVIOR_FLIGHT
    } CameraBehavior;

    Camera();

    void lookAt(const glm::vec3 &target);
    /** Where the eye is, the point it looks at, and which way is up. **/
    struct Aim {
        glm::vec3 eye{0.0F};
        glm::vec3 target{0.0F, 0.0F, -1.0F};
        glm::vec3 up{0.0F, 1.0F, 0.0F};
    };

    void lookAt(const Aim& aim);

    void move(float dx, float dy, float dz);
    glm::vec3 calculateMovement(const glm::vec3 & delta);

    void move(const glm::vec3 &direction, const glm::vec3 &amount);

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

    void perspective(const Lens& lens);
    void rotate(float headingDegrees, float pitchDegrees, float rollDegrees);

    void updatePosition(const glm::vec3 &direction, float elapsedTimeSec);
    glm::vec3 calculateDisplacement(const glm::vec3 &direction, float elapsedTimeSec);
    /**
     * Accelerate each axis that the direction drives toward its velocity limit, and slow each
     * axis it does not drive toward rest, over the elapsed time. The direction is in [-1, 1].
     **/
    void updateVelocity(const glm::vec3 &direction, float elapsedTimeSec);

    const glm::vec3 &acceleration() const;
    CameraBehavior behavior() const;
    const glm::vec3 &currentVelocity() const;
    const glm::vec3 &position() const;
    const glm::mat4 &projection() const;
    const glm::vec3 &velocity() const;
    const glm::vec3 &direction() const;
    const glm::mat4 &view() const;
    const glm::vec3 &xAxis() const;
    const glm::vec3 &yAxis() const;
    const glm::vec3 &zAxis() const;
    bool dirty() const;

    void acceleration(float x, float y, float z);
    void acceleration(const glm::vec3 &acceleration);
    void behavior(CameraBehavior newBehavior);
    void currentVelocity(float x, float y, float z);
    void currentVelocity(const glm::vec3 &currentVelocity);
    void position(float x, float y, float z);
    void position(const glm::vec3 &position);
    void velocity(float x, float y, float z);
    void velocity(const glm::vec3 &velocity);
    void dirty(bool status);

 protected:
    void rotateFlight(float headingDegrees, float pitchDegrees, float rollDegrees);
    void rotateFirstPerson(float headingDegrees, float pitchDegrees);
    void updateViewMatrix(bool orthogonalizeAxes);

 private:
    CameraBehavior behavior_;
    float fovx_;
    float near_;
    float far_;
    float aspect_;
    float pitch_;
    glm::vec3 eye_;
    glm::vec3 xAxis_;
    glm::vec3 yAxis_;
    glm::vec3 zAxis_;
    glm::vec3 direction_;
    glm::vec3 acceleration_;
    glm::vec3 currentVelocity_;
    glm::vec3 velocity_;
    glm::mat4 view_;
    glm::mat4 projection_;
    bool dirty_;
};
