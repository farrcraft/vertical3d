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
    orthoZoom_(DEFAULT_ORTHO_ZOOM),
    pixelAspect_(DEFAULT_PIXEL_ASPECT),
    near_(DEFAULT_NEAR),
    far_(DEFAULT_FAR),
    fov_(DEFAULT_FOV),
    options_(OPTION_ORTHOGRAPHIC),
    size_(),
    // glm leaves the quaternion uninitialized, and Camera::createView casts it before
    // anything else has a chance to set it
rotation_(1.0f, 0.0f, 0.0f, 0.0f),
basisValid_(false) {
}

Profile::Profile(const std::string& name, const Frame& frame) :
    name_(name),
    eye_(frame.eye),
    direction_(frame.direction),
    right_(frame.right),
    up_(frame.up),
    hand_(Hand::UpCrossDirection),
    orthoZoom_(DEFAULT_ORTHO_ZOOM),
    pixelAspect_(DEFAULT_PIXEL_ASPECT),
    near_(DEFAULT_NEAR),
    far_(DEFAULT_FAR),
    fov_(DEFAULT_FOV),
    options_(OPTION_ORTHOGRAPHIC),
    size_(),
    rotation_(1.0f, 0.0f, 0.0f, 0.0f),
    basisValid_(false) {
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

void Profile::clipping(const glm::vec2& planes) {
    near_ = planes.x;
    far_ = planes.y;
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
    // whatever lookat() built is not this rotation, so createView() goes back to casting
    basisValid_ = false;
}

void Profile::turn(const glm::quat& local) {
    rotation_ = glm::normalize(rotation_ * local);
    basisValid_ = false;

    // the rotation's columns are the right handed basis lookat() builds, so the normals
    // come out of it the way lookat() hands them out
    const glm::mat3 axes = glm::mat3_cast(rotation_);
    right_ = hand_ == Hand::DirectionCrossUp ? -axes[0] : axes[0];
    up_ = axes[1];
    direction_ = axes[2];
}

glm::mat4x4 Profile::orientation() const {
    return basisValid_ ? basis_ : glm::mat4_cast(rotation_);
}

void Profile::size(const glm::uvec2& size) {
    size_[0] = size.x;
    size_[1] = size.y;
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

    // normal of the yz plane is the right vector, taken as up x direction, and the normal
    // of the xy plane is the up vector. The result is the unmirrored basis, the only one of
    // the two hands a quaternion can carry
    x = glm::normalize(glm::cross(y, z));
    // the component of the original up perpendicular to the direction, which is the same
    // vector whichever way round the right was taken - the two hands mirror horizontally
    // and agree about which way is up.
    //
    // Not normalized: z and x are unit and perpendicular, so their cross product is already
    // unit to within rounding. glm::lookAt does not normalize it either, and doing so would
    // change the last bits of every view built here compared with glm's
    y = glm::cross(z, x);

    /*
        the rotation takes the camera out of the default basis and into the one its three
        normals define, so the normals are its columns. Camera::createView() transposes it
        to get the world to view transform.

        The rotation is always built from the right handed basis, never from the mirrored
        one. A mirror is an improper transform and no quaternion represents one, so a
        quat_cast of it returns something that is not a rotation at all and the view matrix
        that comes out of it is not rigid. The hand is applied by Camera::createView()
        instead, which negates view x.

        glm indexes [column][row]:
        [  0,  4,  8,  12 ]
        [  1,  5,  9,  13 ]
        [  2,  6, 10,  14 ]
        [  3,  7, 11,  15 ]
    */
    glm::mat4x4 basis;

    basis[0][0] = x[0];
    basis[0][1] = x[1];
    basis[0][2] = x[2];
    basis[0][3] = 0.0;
    basis[1][0] = y[0];
    basis[1][1] = y[1];
    basis[1][2] = y[2];
    basis[1][3] = 0.0;
    basis[2][0] = z[0];
    basis[2][1] = z[1];
    basis[2][2] = z[2];
    basis[2][3] = 0.0;
    basis[3][0] = 0.0;
    basis[3][1] = 0.0;
    basis[3][2] = 0.0;
    basis[3][3] = 1.0;

    rotation_ = glm::quat_cast(basis);
    // and the matrix itself is kept, so createView() does not have to rebuild it out of
    // the quaternion it was just cast to
    basis_ = basis;
    basisValid_ = true;
    up_ = y;
    direction_ = z;
    // the normals follow the hand, so the mirrored hand reports right negated. Callers read
    // the normals and draw their own geometry against them; the rotation holds the unmirrored
    // basis
    right_ = hand_ == Hand::DirectionCrossUp ? -x : x;
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
    // the cache is copied with the rotation it describes, so a clone of a profile built by
    // lookat() builds the same view
    basis_ = profile.basis_;
    basisValid_ = profile.basisValid_;
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

Profile& Profile::operator = (const Profile& other) {
    if (this != &other) {
        clone(other);
    }
    return *this;
}

};  // namespace v3d::type::camera
