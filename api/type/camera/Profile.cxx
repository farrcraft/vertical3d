/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#include "Profile.h"

#include <iostream>
#include <cmath>
#include <string>

#include <glm/gtc/quaternion.hpp>

namespace v3d::type::camera {

Profile::Profile(const std::string& name) :
    name_(name),
    eye_(0.0f, 0.0f, -1.0f),
    direction_(0.0f, 0.0f, 1.0f),
    right_(1.0f, 0.0f, 0.0f),
    up_(0.0f, 1.0f, 0.0f),
    hand_(Hand::UpCrossDirection),
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

Profile::Profile(const std::string& name, const glm::vec3& eye, const glm::vec3& up,
    const glm::vec3& right, const glm::vec3& direction) :
    name_(name),
    eye_(eye),
    direction_(direction),
    right_(right),
    up_(up),
    hand_(Hand::UpCrossDirection),
    orthoZoom_(1.0f),
    pixelAspect_(1.33f),
    near_(0.001f),
    far_(100.0f),
    fov_(60.0f),
    options_(OPTION_ORTHOGRAPHIC),
size_{ 0, 0 },
    rotation_(1.0f, 0.0f, 0.0f, 0.0f) {
}

Profile::~Profile() {
}

std::string Profile::name() const {
    return name_;
}

glm::vec2 Profile::clipping() const {
    return glm::vec2(near_, far_);
}

float Profile::fov() const {
    return fov_;
}

float Profile::orthoZoom() const {
    return orthoZoom_;
}

float Profile::pixelAspect() const {
    return pixelAspect_;
}

bool Profile::orthographic() const {
    return (options_ & OPTION_ORTHOGRAPHIC) != 0;
}

bool Profile::adaptiveProjection() const {
    return (options_ & OPTION_ADAPTIVE_PROJECTION) != 0;
}

bool Profile::adaptivePosition() const {
    return (options_ & OPTION_ADAPTIVE_POSITION) != 0;
}

glm::vec3 Profile::eye() const {
    return eye_;
}

glm::vec3 Profile::up() const {
    return up_;
}

glm::vec3 Profile::right() const {
    return right_;
}

glm::vec3 Profile::direction() const {
    return direction_;
}

glm::quat Profile::rotation() const {
    return rotation_;
}

glm::uvec2 Profile::size() const {
    return glm::uvec2(size_[0], size_[1]);
}

void Profile::name(const std::string& name) {
    name_ = name;
}

void Profile::clipping(float near, float far) {
    near_ = near;
    far_ = far;
}

void Profile::fov(float fov) {
    fov_ = fov;
}

void Profile::orthoZoom(float zoom) {
    orthoZoom_ = zoom;
}

void Profile::pixelAspect(float aspect) {
    pixelAspect_ = aspect;
}

void Profile::orthographic(bool ortho) {
    if (ortho) {
        options_ |= OPTION_ORTHOGRAPHIC;
    } else {
        options_ &= ~OPTION_ORTHOGRAPHIC;
    }
}

void Profile::adaptiveProjection(bool adaptive) {
    if (adaptive) {
        options_ |= OPTION_ADAPTIVE_PROJECTION;
    } else {
        options_ &= ~OPTION_ADAPTIVE_PROJECTION;
    }
}

void Profile::adaptivePosition(bool adaptive) {
    if (adaptive) {
        options_ |= OPTION_ADAPTIVE_POSITION;
    } else {
        options_ &= ~OPTION_ADAPTIVE_POSITION;
    }
}

void Profile::eye(const glm::vec3& position) {
    eye_ = position;
}

void Profile::up(const glm::vec3& up) {
    up_ = up;
}

void Profile::right(const glm::vec3& right) {
    right_ = right;
}

void Profile::direction(const glm::vec3& direction) {
    direction_ = direction;
}

void Profile::rotation(const glm::quat& rotation) {
    rotation_ = rotation;
}

void Profile::size(unsigned int width, unsigned int height) {
    size_[0] = width;
    size_[1] = height;
}

void Profile::lookat(const glm::vec3& center) {
    glm::vec3 x;
    glm::vec3 y;
    glm::vec3 z;

    // new direction vector
    z = center - eye_;
    z = glm::normalize(z);
    // start with original up vector
    y = up_;

    // normal of the yz plane is the new right vector. Which way round the two are crossed
    // is what the hand names: the two answers are negatives of each other, so a scene drawn
    // through one is the mirror of the same scene drawn through the other
    x = hand_ == Hand::DirectionCrossUp ? glm::cross(z, y) : glm::cross(y, z);
    x = glm::normalize(x);
    // normal of the xy plane is the new up vector, crossed the same way round as the right
    // was. Both orders give the component of the original up perpendicular to the direction,
    // so the two hands mirror horizontally and agree about which way is up
    y = hand_ == Hand::DirectionCrossUp ? glm::cross(x, z) : glm::cross(z, x);
    y = glm::normalize(y);

    /*
        the rotation takes the camera out of the default basis and into the one its three
        normals define, so the normals are its columns. Camera::createView() transposes it
        to get the world to view transform.

        glm indexes [column][row]:
        [  0,  4,  8,  12 ]
        [  1,  5,  9,  13 ]
        [  2,  6, 10,  14 ]
        [  3,  7, 11,  15 ]
    */
    glm::mat4x4 m;

    m[0][0] = x[0];
    m[0][1] = x[1];
    m[0][2] = x[2];
    m[0][3] = 0.0;
    m[1][0] = y[0];
    m[1][1] = y[1];
    m[1][2] = y[2];
    m[1][3] = 0.0;
    m[2][0] = z[0];
    m[2][1] = z[1];
    m[2][2] = z[2];
    m[2][3] = 0.0;
    m[3][0] = 0.0;
    m[3][1] = 0.0;
    m[3][2] = 0.0;
    m[3][3] = 1.0;

    rotation_ = glm::quat_cast(m);
    up_ = y;
    direction_ = z;
    right_ = x;
}

void Profile::clone(const Profile& profile) {
    near_ = profile.near_;
    far_ = profile.far_;
    fov_ = profile.fov_;
    orthoZoom_ = profile.orthoZoom_;
    pixelAspect_ = profile.pixelAspect_;
    eye_ = profile.eye_;
    up_ = profile.up_;
    hand_ = profile.hand_;
    right_ = profile.right_;
    direction_ = profile.direction_;
    name_ = profile.name_;
    rotation_ = profile.rotation_;
    options_ = profile.options_;
    size_[0] = profile.size_[0];
    size_[1] = profile.size_[1];
}

Profile::Hand Profile::hand() const {
    return hand_;
}

void Profile::hand(Hand hand) {
    hand_ = hand;
}

Profile& Profile::operator = (const Profile& p) {
    if (this != &p) {
        clone(p);
    }
    return *this;
}

};  // namespace v3d::type::camera
