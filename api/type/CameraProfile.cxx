/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#include "CameraProfile.h"

#include <iostream>
#include <cmath>

#include <glm/gtc/quaternion.hpp>

namespace v3d::type {

    CameraProfile::CameraProfile(const std::string& name) :
        near_(0.001f),
        far_(100.0f),
        fov_(60.0f),
        orthoZoom_(1.0f),
        pixelAspect_(1.33f),
        eye_(0.0f, 0.0f, -1.0f),
        up_(0.0f, 1.0f, 0.0f),
        right_(1.0f, 0.0f, 0.0f),
        direction_(0.0f, 0.0f, 1.0f),
        options_(OPTION_ORTHOGRAPHIC | OPTION_DEFAULT) {
    }

    CameraProfile::~CameraProfile() {
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
    }

    CameraProfile& CameraProfile::operator = (const CameraProfile& p) {
        clone(p);
        return *this;
    }

    void CameraProfile::eye(const glm::vec3& position) {
        eye_ = position;
    }

    void CameraProfile::clipping(float near, float far) {
        near_ = near;
        far_ = far;
    }

};  // namespace v3d::type
