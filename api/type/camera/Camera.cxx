/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Camera.h"

#include <iostream>
#include <cmath>

#include <glm/gtc/quaternion.hpp>

namespace v3d::type::camera {

Camera::Camera() : profile_("Unnamed") {
}

Camera::Camera(const Profile& profile) : profile_(profile) {
}

Camera::~Camera() {
}

Profile& Camera::profile() {
    return profile_;
}

const Profile& Camera::profile() const {
    return profile_;
}

/*
based on gluUnProject
takes a screen space coordinate and the viewport dimensions
and returns the world space coordinate
*/
glm::vec3 Camera::unproject(const glm::vec3& point, int viewport[4]) {
    glm::vec4 p;
    // normalize point to range [-1, 1]
    p[0] = (point[0] - viewport[0]) * 2.0f / viewport[2] - 1.0f;
    // clip space points y downward, the same way a screen coordinate does
    p[1] = (point[1] - viewport[1]) * 2.0f / viewport[3] - 1.0f;
    // depth is already the [0, 1] the projection wrote
    p[2] = point[2];
    p[3] = 1.0f;

    // get inverse transformation matrix
    glm::mat4x4 m;
    glm::mat4x4 inv;
    m = projection() * view();
    inv = glm::inverse(m);
    float w = inv[0][3] * p[0] + inv[1][3] * p[1] + inv[2][3] * p[2] + inv[3][3];

    return ((inv * p) / w);
}

/*
based on gluProject
takes a world space coordinate and the viewport dimensions
returns the screen space coordinate
*/
glm::vec3 Camera::project(const glm::vec3& point, int viewport[4]) {
    glm::vec4 p;
    p = view() * glm::vec4(point, 1.0f);
    p = projection() * p;

    // the perspective divide. An orthographic projection leaves w at one
    if (p[3] != 0.0f) {
        p /= p[3];
    }

    p[0] = viewport[0] + (1.0f + p[0]) * viewport[2] / 2.0f;
    // clip space y already points down, so this is a screen coordinate
    p[1] = viewport[1] + (1.0f + p[1]) * viewport[3] / 2.0f;

    return p;
}

geometry::Ray Camera::ray(const glm::vec2& point, int viewport[4]) {
    // the two ends of the pixel's line through the frustum. Depth zero is the near
    // plane and one is the far one, per ADR-0012
    const glm::vec3 from = unproject(glm::vec3(point.x, point.y, 0.0f), viewport);
    const glm::vec3 to = unproject(glm::vec3(point.x, point.y, 1.0f), viewport);
    return geometry::Ray(from, to - from);
}

/*
build either an orthographic or perspective projection matrix

both are vulkan clip space, per ADR-0012: y points down and depth runs from zero at the
near plane to one at the far one. The camera looks along its own direction vector, which
the profile documents as +z of the basis its three normals define - so a point in front
of the camera has a positive view z, and w is that z rather than its negation.

the frustum the fov and the pixel aspect describe is symmetric about both axes, so there
are no off centre terms in the third column.
*/
void Camera::createProjection() {  // active scene bound
    float aspect = profile_.pixelAspect();
    if (!orthographic()) {
        /*
            [x	 0	 0	0]
            [0	-y	 0	0]
            [0	 0	 C	D]
            [0	 0	 1	0]

            x = near / xmax, the half width of the near plane
            y = near / ymax
            C = far / (far - near)
            D = -(far * near) / (far - near)

            C and D put the near plane at depth zero and the far one at depth one; the
            negated y is the flip into vulkan's downward clip space.
        */
        const float near = profile_.clipping()[0];
        const float far = profile_.clipping()[1];
        float ymax = near * tan(profile_.fov() * glm::pi<float>() / 360.0f);
        float xmax = ymax * aspect;

        float x = near / xmax;
        float y = near / ymax;
        float C = far / (far - near);
        float D = -(far * near) / (far - near);

        projection_[0][0] = x;
        projection_[1][0] = 0.0f;
        projection_[2][0] = 0.0f;
        projection_[3][0] = 0.0f;
        projection_[0][1] = 0.0f;
        projection_[1][1] = -y;
        projection_[2][1] = 0.0f;
        projection_[3][1] = 0.0f;
        projection_[0][2] = 0.0f;
        projection_[1][2] = 0.0f;
        projection_[2][2] = C;
        projection_[3][2] = D;
        projection_[0][3] = 0.0f;
        projection_[1][3] = 0.0f;
        // w is the view z, because the camera looks along +z
        projection_[2][3] = 1.0f;
        projection_[3][3] = 0.0f;
    } else {  // orthographic
        /*
            [2/(right-left)	 0					0				tx]
            [0				-2/(top-bottom)		0				ty]
            [0				 0					1/(far-near)	tz]
            [0				 0					0				 1]

            tx = -(right + left) / (right - left)
            ty =  (top + bottom) / (top - bottom)
            tz = -near / (far - near)

            near and far are distances from the camera along its direction of view; left,
            right, top and bottom are points on the respective clipping planes.
        */
        aspect *= profile_.orthoZoom();
        float left = -1.0f * aspect;
        float right = 1.0f * aspect;
        float top = 1.0f * profile_.orthoZoom();
        float bottom = -1.0f * profile_.orthoZoom();
        float far_val = profile_.clipping()[1];
        float near_val = profile_.clipping()[0];

        float tx = -(right + left) / (right - left);
        // the y flip takes the sign of ty with it
        float ty = (top + bottom) / (top - bottom);
        float tz = -near_val / (far_val - near_val);

        projection_[0][0] = 2.0f / (right - left);
        projection_[1][0] = 0.0f;
        projection_[2][0] = 0.0f;
        projection_[3][0] = tx;
        projection_[0][1] = 0.0f;
        projection_[1][1] = -2.0f / (top - bottom);
        projection_[2][1] = 0.0f;
        projection_[3][1] = ty;
        projection_[0][2] = 0.0f;
        projection_[1][2] = 0.0f;
        projection_[2][2] = 1.0f / (far_val - near_val);
        projection_[3][2] = tz;
        projection_[0][3] = 0.0f;
        projection_[1][3] = 0.0f;
        projection_[2][3] = 0.0f;
        projection_[3][3] = 1.0f;
    }
}

void Camera::createView() {
    // a world point is translated to the eye and then rotated into the camera's axes.
    // The rotation applied is the profile's inverse, which for a pure rotation is its
    // transpose, and it has to come after the translation or it turns the eye offset too
    glm::vec3 e = -profile_.eye();
    // the basis lookat() built when it is still the rotation in force, and the rotation
    // cast back to a matrix when anything else has set it since. The two agree to about
    // 2e-6 of an element; the cached one is the same arithmetic glm::lookAt does, so a
    // view built through lookat() matches one built through glm::lookAt exactly
    view_ = glm::transpose(profile_.orientation());
    view_ = glm::translate(view_, e);
    if (profile_.hand() == Profile::Hand::DirectionCrossUp) {
        // the mirrored basis is the rotation with view x negated, and this is where that
        // happens rather than in the rotation, which carries no mirror - ADR-0052. Applied
        // on the left, so it mirrors the view rather than the world the rotation is turning
        glm::mat4x4 mirror(1.0f);
        mirror[0][0] = -1.0f;
        view_ = mirror * view_;
    }
}

/*
    pan - move horizontally around a fixed axis (the camera's y axis) - camera rotation & look at position changes
            but eye position doesn't - look left/right
    pan and tilt are types of rotation with restrictions
*/
void Camera::pan(float angle) {
    profile_.turn(glm::angleAxis(angle, glm::vec3(0.0f, 1.0f, 0.0f)));
}

/*
    tilt - move vertically around a fixed axis (camera's x axis) - look up/down
*/
void Camera::tilt(float angle) {
    profile_.turn(glm::angleAxis(angle, glm::vec3(1.0f, 0.0f, 0.0f)));
}

/*
    dolly - move eye forward or backward along direction of view
    dolly, truck, pedestal are types of translations with special restrictions
    dolly - same as pedestal but use direction vector instead of up vector
*/
void Camera::dolly(float d) {
    profile_.eye(profile_.eye() + profile_.direction() * d);
}

/*
    truck - move eye on axis perpendicular to direction of view and up axis - move left/right
    truck - multiply delta value and right vector to get eye delta - right vector must be
    normalized - add eye delta to current eye position
*/
void Camera::truck(float delta) {
    profile_.eye(profile_.eye() + profile_.right() * delta);
}

/*
    zoom - affects the camera lens to zoom in or out (dolly without moving camera)
*/
void Camera::zoom(float z) {
    profile_.orthoZoom(profile_.orthoZoom() + z);
}

/*
    pedestal - move eye on up axis - move up/down
    pedestal - same as truck but use up vector instead of right vector
*/
void Camera::pedestal(float delta) {
    profile_.eye(profile_.eye() + profile_.up() * delta);
}

glm::mat4x4 Camera::view() const {
    return view_;
}

glm::mat4x4 Camera::projection() const {
    return projection_;
}

bool Camera::orthographic() const {
    return profile_.orthographic();
}

void Camera::orthographic(bool ortho) {
    profile_.orthographic(ortho);
}

float Camera::orthoFactorHorizontal() const {
    if (profile_.size()[0] == 0) {
        return 0.0f;
    }
    return (profile_.orthoZoom() * 2.0f * profile_.pixelAspect()) / profile_.size()[0];
}

float Camera::orthoFactorVertical() const {
    if (profile_.size()[1] == 0) {
        return 0.0f;
    }
    return (profile_.orthoZoom() * 2.0f) / profile_.size()[1];
}

void Camera::rotate(const glm::quat& new_rot) {
    // ArcBall::drag answers a zero quaternion for a drag too short to have an axis, which
    // is no rotation at all rather than one to compose
    if (glm::dot(new_rot, new_rot) <= glm::epsilon<float>()) {
        return;
    }
    profile_.turn(new_rot);
}

};  // namespace v3d::type::camera
