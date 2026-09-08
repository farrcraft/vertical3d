/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "RIBHandler.h"

#include <cmath>
#include <string>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat3x3.hpp>

namespace v3d::talyn {

namespace {

typedef v3d::render::offline::ParameterList ParameterList;

const float TOLERANCE = 1.0e-4f;

/**
 * Whether a 3x3 is a rotation: orthonormal, and not one that reverses handedness.
 **/
bool rotation(const glm::mat3 & m) {
    const glm::mat3 identity = glm::transpose(m) * m;
    for (int column = 0; column < 3; column++) {
        for (int row = 0; row < 3; row++) {
            const float expected = column == row ? 1.0f : 0.0f;
            if (std::fabs(identity[column][row] - expected) > TOLERANCE) {
                return false;
            }
        }
    }
    return glm::determinant(m) > 0.0f;
}

};  // namespace

RIBHandler::RIBHandler(const boost::shared_ptr<RenderContext> & rc) : rc_(rc) {
}

const std::string & RIBHandler::error() const {
    return error_;
}

void RIBHandler::format(unsigned int width, unsigned int height, float pixelAspect) {
    rc_->format(width, height);
    if (!frameAspectNamed_ && height > 0) {
        frameAspectRatio(width * pixelAspect / height);
        frameAspectNamed_ = false;
    }
}

void RIBHandler::frameAspectRatio(float aspect) {
    frameAspect_ = aspect;
    frameAspectNamed_ = true;
    if (screenNamed_) {
        return;
    }
    // the RI default: the wider dimension spans [-1, 1] and the other is the reciprocal
    if (frameAspect_ >= 1.0f) {
        screenWindow(-frameAspect_, frameAspect_, -1.0f, 1.0f);
    } else {
        screenWindow(-1.0f, 1.0f, -1.0f / frameAspect_, 1.0f / frameAspect_);
    }
    screenNamed_ = false;
}

void RIBHandler::screenWindow(float left, float right, float bottom, float top) {
    screen_[0] = left;
    screen_[1] = right;
    screen_[2] = bottom;
    screen_[3] = top;
    screenNamed_ = true;
}

void RIBHandler::projection(const std::string & name, const ParameterList & parameters) {
    projection_ = name.empty() ? "orthographic" : name;
    fov_ = parameters.number("fov", 90.0f);
    // RI marks the current system as camera space here and reinitialises the current
    // transformation, so what follows up to WorldBegin is the world to camera transform
    transform_ = glm::mat4x4(1.0f);
}

void RIBHandler::clipping(float hither, float yon) {
    near_ = hither;
    far_ = yon;
    clippingNamed_ = true;
}

bool RIBHandler::buildCamera() {
    // a centred screen window is what a CameraProfile can hold: it carries a field of
    // view and a pixel aspect, not four edges
    if (std::fabs(screen_[0] + screen_[1]) > TOLERANCE || std::fabs(screen_[2] + screen_[3]) > TOLERANCE) {
        error_ = "an off centre ScreenWindow is not supported";
        return false;
    }
    const float right = screen_[1];
    const float top = screen_[3];
    if (right <= 0.0f || top <= 0.0f) {
        error_ = "a ScreenWindow with no area is not supported";
        return false;
    }

    /*
        The world to camera transformation is a rotation and a translation, which is what
        createView() builds from an eye and a rotation: view = R' * T(-eye), so the matrix's
        upper 3x3 is R' and its translation column is R' applied to -eye.

        A matrix that reverses handedness cannot be one of those. RI's camera basis is left
        handed with respect to the world for any camera placed by a general lookat, so this
        refuses rather than rendering a mirrored picture that looks plausible.
    */
    const glm::mat3 basis(transform_);
    if (!rotation(basis)) {
        error_ = "a world to camera transformation that is not a rotation and a translation is not supported";
        return false;
    }

    v3d::type::CameraProfile & profile = rc_->scene().camera().profile();
    profile.rotation(glm::quat_cast(glm::transpose(basis)));
    profile.eye(-glm::transpose(basis) * glm::vec3(transform_[3]));
    profile.orthographic(projection_ != "perspective");
    // the screen window is what the projection writes into, so its half extents are the
    // camera's aperture and their ratio is the pixel aspect
    profile.pixelAspect(right / top);
    if (projection_ == "perspective") {
        // RI states fov as the full angle spanning screen space [-1, 1]; a window that is
        // not the unit square scales the tangent rather than the angle
        profile.fov(2.0f * glm::degrees(std::atan(top * std::tan(glm::radians(fov_) / 2.0f))));
    } else {
        profile.orthoZoom(top);
    }
    if (clippingNamed_) {
        profile.clipping(near_, far_);
    }
    return true;
}

void RIBHandler::worldBegin() {
    if (buildCamera()) {
        // inside the world block the current transformation is object to world
        transform_ = glm::mat4x4(1.0f);
    }
}

void RIBHandler::attributeBegin() {
    Attributes saved;
    saved.transform = transform_;
    saved.color = color_;
    attributes_.push_back(saved);
}

void RIBHandler::attributeEnd() {
    if (attributes_.empty()) {
        return;
    }
    transform_ = attributes_.back().transform;
    color_ = attributes_.back().color;
    attributes_.pop_back();
}

void RIBHandler::transformBegin() {
    transforms_.push_back(transform_);
}

void RIBHandler::transformEnd() {
    if (transforms_.empty()) {
        return;
    }
    transform_ = transforms_.back();
    transforms_.pop_back();
}

void RIBHandler::identity() {
    transform_ = glm::mat4x4(1.0f);
}

void RIBHandler::transform(const glm::mat4x4 & matrix) {
    transform_ = matrix;
}

void RIBHandler::concatTransform(const glm::mat4x4 & matrix) {
    transform_ = transform_ * matrix;
}

void RIBHandler::translate(float dx, float dy, float dz) {
    transform_ = glm::translate(transform_, glm::vec3(dx, dy, dz));
}

// RiRotate states its angle in degrees, which is the one place the interface disagrees
// with glm
void RIBHandler::rotate(float angle, float dx, float dy, float dz) {
    transform_ = glm::rotate(transform_, glm::radians(angle), glm::vec3(dx, dy, dz));
}

void RIBHandler::scale(float sx, float sy, float sz) {
    transform_ = glm::scale(transform_, glm::vec3(sx, sy, sz));
}

void RIBHandler::color(const glm::vec3 & value) {
    color_ = value;
}

void RIBHandler::fan(const std::vector<glm::vec3> & points, const std::vector<glm::vec3> & normals,
    const std::vector<unsigned int> & indices) {
    if (indices.size() < 3) {
        return;
    }
    /*
        A normal transforms by the inverse transpose rather than by the matrix that moves
        the points. The two agree under a rotation and a uniform scale, and part company the
        moment a scene scales one axis, which tilts a normal off its surface.

        A scene that gives no varying "N" falls through to the constructor that takes the
        triangle's own plane, which is built from points already in world space.
    */
    const glm::mat3 toWorldNormal = glm::transpose(glm::inverse(glm::mat3(transform_)));
    for (std::size_t i = 1; i + 1 < indices.size(); i++) {
        if (indices[0] >= points.size() || indices[i] >= points.size() || indices[i + 1] >= points.size()) {
            continue;
        }
        const glm::vec3 a(transform_ * glm::vec4(points[indices[0]], 1.0f));
        const glm::vec3 b(transform_ * glm::vec4(points[indices[i]], 1.0f));
        const glm::vec3 c(transform_ * glm::vec4(points[indices[i + 1]], 1.0f));
        if (indices[0] < normals.size() && indices[i] < normals.size() && indices[i + 1] < normals.size()) {
            rc_->scene().add(Triangle(a, b, c, color_,
                glm::normalize(toWorldNormal * normals[indices[0]]),
                glm::normalize(toWorldNormal * normals[indices[i]]),
                glm::normalize(toWorldNormal * normals[indices[i + 1]])));
        } else {
            rc_->scene().add(Triangle(a, b, c, color_));
        }
    }
}

void RIBHandler::polygon(unsigned int vertices, const ParameterList & parameters) {
    const std::vector<glm::vec3> points = parameters.points("P");
    const std::vector<glm::vec3> normals = parameters.points("N");
    std::vector<unsigned int> indices;
    for (unsigned int i = 0; i < vertices && i < points.size(); i++) {
        indices.push_back(i);
    }
    fan(points, normals, indices);
}

void RIBHandler::pointsPolygons(const std::vector<unsigned int> & counts, const std::vector<unsigned int> & indices,
    const ParameterList & parameters) {
    const std::vector<glm::vec3> points = parameters.points("P");
    const std::vector<glm::vec3> normals = parameters.points("N");
    std::size_t offset = 0;
    for (unsigned int count : counts) {
        if (offset + count > indices.size()) {
            return;
        }
        const std::vector<unsigned int> face(indices.begin() + offset, indices.begin() + offset + count);
        fan(points, normals, face);
        offset += count;
    }
}

};  // namespace v3d::talyn
