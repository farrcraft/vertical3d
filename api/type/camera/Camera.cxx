/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Camera.h"

#include <iostream>
#include <algorithm>
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
glm::vec3 Camera::unproject(const glm::vec3& point, const int viewport[4]) const {
    glm::vec4 ndc;
    // normalize point to range [-1, 1]
    ndc[0] = ((point[0] - static_cast<float>(viewport[0])) * 2.0F / static_cast<float>(viewport[2])) - 1.0F;
    // clip space points y downward, the same way a screen coordinate does
    ndc[1] = ((point[1] - static_cast<float>(viewport[1])) * 2.0F / static_cast<float>(viewport[3])) - 1.0F;
    // depth is already the [0, 1] the projection wrote
    ndc[2] = point[2];
    ndc[3] = 1.0F;

    // get inverse transformation matrix
    glm::mat4x4 transform;
    glm::mat4x4 inv;
    transform = projection() * view();
    inv = glm::inverse(transform);
    const float w = (inv[0][3] * ndc[0]) + (inv[1][3] * ndc[1]) + (inv[2][3] * ndc[2]) + inv[3][3];

    return ((inv * ndc) / w);
}

/*
based on gluProject
takes a world space coordinate and the viewport dimensions
returns the screen space coordinate
*/
glm::vec3 Camera::project(const glm::vec3& point, const int viewport[4]) const {
    glm::vec4 projected;
    projected = view() * glm::vec4(point, 1.0F);
    projected = projection() * projected;

    // the perspective divide. An orthographic projection leaves w at one
    if (projected[3] != 0.0F) {
        projected /= projected[3];
    }

    projected[0] = static_cast<float>(viewport[0]) + ((1.0F + projected[0]) * static_cast<float>(viewport[2]) / 2.0F);
    // clip space y already points down, so this is a screen coordinate
    projected[1] = static_cast<float>(viewport[1]) + ((1.0F + projected[1]) * static_cast<float>(viewport[3]) / 2.0F);

    return projected;
}

std::optional<glm::vec3> Camera::screenPoint(const glm::vec3& point, const int viewport[4]) const {
    if (viewport[2] <= 0 || viewport[3] <= 0) {
        return std::nullopt;
    }
    // w is the depth in front of the eye for a perspective camera, and one for an
    // orthographic one. A point at or behind the eye has no place on the screen
    const glm::vec4 clip = projection() * (view() * glm::vec4(point, 1.0F));
    if (clip[3] <= 0.0F) {
        return std::nullopt;
    }
    return project(point, viewport);
}

std::optional<glm::vec3> Camera::pick(const glm::vec2& cursor, const int viewport[4],
    const geometry::Plane& plane) const {
    const geometry::Ray cast = ray(cursor, viewport);
    float distance = 0.0F;
    if (!cast.intersects(plane, &distance)) {
        return std::nullopt;
    }
    return cast.point(distance);
}

geometry::Ray Camera::ray(const glm::vec2& point, const int viewport[4]) const {
    // the two ends of the pixel's line through the frustum. Depth zero is the near
    // plane and one is the far one
    const glm::vec3 from = unproject(glm::vec3(point.x, point.y, 0.0F), viewport);
    const glm::vec3 to = unproject(glm::vec3(point.x, point.y, 1.0F), viewport);
    return geometry::Ray({.origin = from, .direction = to - from});
}

/*
build either an orthographic or perspective projection matrix

both are vulkan clip space: y points down and depth runs from zero at the
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
            C = depthScale = far / (far - near)
            D = depthOffset = -(far * near) / (far - near)

            depthScale and depthOffset put the near plane at depth zero and the far one at depth
            one; the negated y is the flip into vulkan's downward clip space.
        */
        const float near = profile_.clipping()[0];
        const float far = profile_.clipping()[1];
        const float ymax = near * tan(profile_.fov() * glm::pi<float>() / 360.0F);
        const float xmax = ymax * aspect;

        const float x = near / xmax;
        const float y = near / ymax;
        const float depthScale = far / (far - near);
        const float depthOffset = -(far * near) / (far - near);

        projection_[0][0] = x;
        projection_[1][0] = 0.0F;
        projection_[2][0] = 0.0F;
        projection_[3][0] = 0.0F;
        projection_[0][1] = 0.0F;
        projection_[1][1] = -y;
        projection_[2][1] = 0.0F;
        projection_[3][1] = 0.0F;
        projection_[0][2] = 0.0F;
        projection_[1][2] = 0.0F;
        projection_[2][2] = depthScale;
        projection_[3][2] = depthOffset;
        projection_[0][3] = 0.0F;
        projection_[1][3] = 0.0F;
        // w is the view z, because the camera looks along +z
        projection_[2][3] = 1.0F;
        projection_[3][3] = 0.0F;
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
        const float left = -1.0F * aspect;
        const float right = 1.0F * aspect;
        const float top = 1.0F * profile_.orthoZoom();
        const float bottom = -1.0F * profile_.orthoZoom();
        const float far_val = profile_.clipping()[1];
        const float near_val = profile_.clipping()[0];

        const float tx = -(right + left) / (right - left);
        // the y flip takes the sign of ty with it
        const float ty = (top + bottom) / (top - bottom);
        const float tz = -near_val / (far_val - near_val);

        projection_[0][0] = 2.0F / (right - left);
        projection_[1][0] = 0.0F;
        projection_[2][0] = 0.0F;
        projection_[3][0] = tx;
        projection_[0][1] = 0.0F;
        projection_[1][1] = -2.0F / (top - bottom);
        projection_[2][1] = 0.0F;
        projection_[3][1] = ty;
        projection_[0][2] = 0.0F;
        projection_[1][2] = 0.0F;
        projection_[2][2] = 1.0F / (far_val - near_val);
        projection_[3][2] = tz;
        projection_[0][3] = 0.0F;
        projection_[1][3] = 0.0F;
        projection_[2][3] = 0.0F;
        projection_[3][3] = 1.0F;
    }
}

void Camera::createView() {
    // a world point is translated to the eye and then rotated into the camera's axes.
    // The rotation applied is the profile's inverse, which for a pure rotation is its
    // transpose, and it has to come after the translation or it turns the eye offset too
    const glm::vec3 eyeOffset = -profile_.eye();
    // the basis lookat() built when it is still the rotation in force, and the rotation
    // cast back to a matrix when anything else has set it since. The two agree to about
    // 2e-6 of an element; the cached one is the same arithmetic glm::lookAt does, so a
    // view built through lookat() matches one built through glm::lookAt exactly
    view_ = glm::transpose(profile_.orientation());
    view_ = glm::translate(view_, eyeOffset);
    if (profile_.hand() == Profile::Hand::DirectionCrossUp) {
        // the mirrored basis is the rotation with view x negated. A quaternion cannot carry
        // a mirror, so the negation is applied here. Applied on the left, so it mirrors the
        // view rather than the world the rotation is turning
        glm::mat4x4 mirror(1.0F);
        mirror[0][0] = -1.0F;
        view_ = mirror * view_;
    }
}

/*
    pan - move horizontally around a fixed axis (the camera's y axis) - camera rotation & look at position changes
            but eye position doesn't - look left/right
    pan and tilt are types of rotation with restrictions
*/
void Camera::pan(float angle) {
    profile_.turn(glm::angleAxis(angle, glm::vec3(0.0F, 1.0F, 0.0F)));
}

/*
    tilt - move vertically around a fixed axis (camera's x axis) - look up/down
*/
void Camera::tilt(float angle) {
    profile_.turn(glm::angleAxis(angle, glm::vec3(1.0F, 0.0F, 0.0F)));
}

/*
    dolly - move eye forward or backward along direction of view
    dolly, truck, pedestal are types of translations with special restrictions
    dolly - same as pedestal but use direction vector instead of up vector
*/
void Camera::dolly(float distance) {
    profile_.eye(profile_.eye() + profile_.direction() * distance);
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
    profile_.orthoZoom(std::max(profile_.orthoZoom() + z, MINIMUM_ZOOM));
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
        return 0.0F;
    }
    return (profile_.orthoZoom() * 2.0F * profile_.pixelAspect()) / static_cast<float>(profile_.size()[0]);
}

float Camera::orthoFactorVertical() const {
    if (profile_.size()[1] == 0) {
        return 0.0F;
    }
    return (profile_.orthoZoom() * 2.0F) / static_cast<float>(profile_.size()[1]);
}

void Camera::rotate(const glm::quat& new_rot) {
    // ArcBall::drag returns a zero quaternion for a drag too short to have an axis, which
    // is no rotation at all rather than one to compose
    if (glm::dot(new_rot, new_rot) <= glm::epsilon<float>()) {
        return;
    }
    profile_.turn(new_rot);
}

};  // namespace v3d::type::camera
