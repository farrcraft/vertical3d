/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#include "CameraProfile.h"

#include <iostream>
#include <cmath>
#include <string>

#include <glm/gtc/quaternion.hpp>

namespace v3d::type {

    CameraProfile::CameraProfile(const std::string& name) :
        name_(name),
        eye_(0.0f, 0.0f, -1.0f),
        direction_(0.0f, 0.0f, 1.0f),
        right_(1.0f, 0.0f, 0.0f),
        up_(0.0f, 1.0f, 0.0f),
        orthoZoom_(1.0f),
        pixelAspect_(1.33f),
        near_(0.001f),
        far_(100.0f),
        fov_(60.0f),
        options_(OPTION_ORTHOGRAPHIC),
        size_{ 0, 0 },
        // glm leaves the quaternion uninitialized, and Camera::createView casts it before
        // anything else has a chance to set it
        rotation_(1.0f, 0.0f, 0.0f, 0.0f) {
    }

    CameraProfile::CameraProfile(const std::string& name, const glm::vec3& eye, const glm::vec3& up,
        const glm::vec3& right, const glm::vec3& direction) :
        name_(name),
        eye_(eye),
        direction_(direction),
        right_(right),
        up_(up),
        orthoZoom_(1.0f),
        pixelAspect_(1.33f),
        near_(0.001f),
        far_(100.0f),
        fov_(60.0f),
        options_(OPTION_ORTHOGRAPHIC),
        size_{ 0, 0 },
        rotation_(1.0f, 0.0f, 0.0f, 0.0f) {
    }

    CameraProfile::~CameraProfile() {
    }

    std::string CameraProfile::name() const {
        return name_;
    }

    glm::vec2 CameraProfile::clipping() const {
        return glm::vec2(near_, far_);
    }

    float CameraProfile::fov() const {
        return fov_;
    }

    float CameraProfile::orthoZoom() const {
        return orthoZoom_;
    }

    float CameraProfile::pixelAspect() const {
        return pixelAspect_;
    }

    bool CameraProfile::orthographic() const {
        return (options_ & OPTION_ORTHOGRAPHIC) != 0;
    }

    bool CameraProfile::adaptiveProjection() const {
        return (options_ & OPTION_ADAPTIVE_PROJECTION) != 0;
    }

    bool CameraProfile::adaptivePosition() const {
        return (options_ & OPTION_ADAPTIVE_POSITION) != 0;
    }

    glm::vec3 CameraProfile::eye() const {
        return eye_;
    }

    glm::vec3 CameraProfile::up() const {
        return up_;
    }

    glm::vec3 CameraProfile::right() const {
        return right_;
    }

    glm::vec3 CameraProfile::direction() const {
        return direction_;
    }

    glm::quat CameraProfile::rotation() const {
        return rotation_;
    }

    glm::uvec2 CameraProfile::size() const {
        return glm::uvec2(size_[0], size_[1]);
    }

    void CameraProfile::name(const std::string& name) {
        name_ = name;
    }

    void CameraProfile::clipping(float near, float far) {
        near_ = near;
        far_ = far;
    }

    void CameraProfile::fov(float fov) {
        fov_ = fov;
    }

    void CameraProfile::orthoZoom(float zoom) {
        orthoZoom_ = zoom;
    }

    void CameraProfile::pixelAspect(float aspect) {
        pixelAspect_ = aspect;
    }

    void CameraProfile::orthographic(bool ortho) {
        if (ortho) {
            options_ |= OPTION_ORTHOGRAPHIC;
        } else {
            options_ &= ~OPTION_ORTHOGRAPHIC;
        }
    }

    void CameraProfile::adaptiveProjection(bool adaptive) {
        if (adaptive) {
            options_ |= OPTION_ADAPTIVE_PROJECTION;
        } else {
            options_ &= ~OPTION_ADAPTIVE_PROJECTION;
        }
    }

    void CameraProfile::adaptivePosition(bool adaptive) {
        if (adaptive) {
            options_ |= OPTION_ADAPTIVE_POSITION;
        } else {
            options_ &= ~OPTION_ADAPTIVE_POSITION;
        }
    }

    void CameraProfile::eye(const glm::vec3& position) {
        eye_ = position;
    }

    void CameraProfile::up(const glm::vec3& up) {
        up_ = up;
    }

    void CameraProfile::right(const glm::vec3& right) {
        right_ = right;
    }

    void CameraProfile::direction(const glm::vec3& direction) {
        direction_ = direction;
    }

    void CameraProfile::rotation(const glm::quat& rotation) {
        rotation_ = rotation;
    }

    void CameraProfile::size(unsigned int width, unsigned int height) {
        size_[0] = width;
        size_[1] = height;
    }

    void CameraProfile::lookat(const glm::vec3& center) {
        glm::vec3 x, y, z;

        // new direction vector
        z = center - eye_;
        z = glm::normalize(z);
        // start with original up vector
        y = up_;

        // normal of yz plane is new right vector
        x = glm::cross(y, z);
        x = glm::normalize(x);
        // normal of the xy plane is the new up vector
        y = glm::cross(z, x);
        y = glm::normalize(y);

        /*
            [  0,  1,  2,  3 ]
            [  4,  5,  6,  7 ]
            [  8,  9, 10, 11 ]
            [ 12, 13, 14, 15 ]

            x = [ 0, 1, 2  ] = right
            y = [ 4, 5, 6  ] = up
            z = [ 8, 9, 10 ] = direction

            glm (column-major ordering):
            [  0,  4,  8,  12 ]
            [  1,  5,  9,  13 ]
            [  2,  6, 10,  14 ]
            [  3,  7, 11,  15 ]
        */
        glm::mat4x4 m;

        m[0][0] = x[0];
        m[1][0] = x[1];
        m[2][0] = x[2];
        m[3][0] = 0.0;
        m[0][1] = y[0];
        m[1][1] = y[1];
        m[2][1] = y[2];
        m[3][1] = 0.0;
        m[0][2] = z[0];
        m[1][2] = z[1];
        m[2][2] = z[2];
        m[3][2] = 0.0;
        m[0][3] = 0.0;
        m[1][3] = 0.0;
        m[2][3] = 0.0;
        m[3][3] = 1.0;

        rotation_ = glm::quat_cast(m);
        up_ = y;
        direction_ = z;
        right_ = x;
    }

    void CameraProfile::clone(const CameraProfile& profile) {
        near_ = profile.near_;
        far_ = profile.far_;
        fov_ = profile.fov_;
        orthoZoom_ = profile.orthoZoom_;
        pixelAspect_ = profile.pixelAspect_;
        eye_ = profile.eye_;
        up_ = profile.up_;
        right_ = profile.right_;
        direction_ = profile.direction_;
        name_ = profile.name_;
        rotation_ = profile.rotation_;
        options_ = profile.options_;
        size_[0] = profile.size_[0];
        size_[1] = profile.size_[1];
    }

    CameraProfile& CameraProfile::operator = (const CameraProfile& p) {
        clone(p);
        return *this;
    }

};  // namespace v3d::type
