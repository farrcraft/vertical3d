/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "CameraOptions.h"

#include <api/render/offline/Camera.h>
#include <api/render/offline/Sampling.h>
#include <api/render/offline/rib/Requests.h>

#include <cmath>
#include <string>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/trigonometric.hpp>

namespace v3d::render::offline {

bool CameraOptions::format(const glm::ivec2 & resolution, float pixelAspect) {
    // RI reads a side of zero or less as the device's default for that side
    const unsigned int width = resolution.x > 0 ? static_cast<unsigned int>(resolution.x) : defaultWidth;
    const unsigned int height = resolution.y > 0 ? static_cast<unsigned int>(resolution.y) : defaultHeight;
    if (width > largestResolution || height > largestResolution) {
        return false;
    }
    width_ = width;
    height_ = height;
    pixelAspect_ = pixelAspect > 0.0F && std::isfinite(pixelAspect) ? pixelAspect : 1.0F;
    if (!frameAspectNamed_) {
        frameAspectRatio(static_cast<float>(width_) * pixelAspect_ / static_cast<float>(height_));
        frameAspectNamed_ = false;
    }
    return true;
}

void CameraOptions::frameAspectRatio(float aspect) {
    frameAspect_ = aspect;
    frameAspectNamed_ = true;
    if (screenNamed_) {
        return;
    }
    // the RI default: the shorter side spans [-1, 1] and the longer spans [-a, a], so that
    // a frame which is not square does not stretch a square window across itself
    if (frameAspect_ >= 1.0F) {
        screenWindow({.left = -frameAspect_, .right = frameAspect_, .bottom = -1.0F, .top = 1.0F});
    } else {
        screenWindow({.left = -1.0F, .right = 1.0F, .bottom = -1.0F / frameAspect_, .top = 1.0F / frameAspect_});
    }
    screenNamed_ = false;
}

void CameraOptions::screenWindow(const rib::ScreenWindow & window) {
    screen_ = window;
    screenNamed_ = true;
}

void CameraOptions::clipping(const rib::Clipping & planes) {
    hither_ = planes.hither;
    yon_ = planes.yon;
}

bool CameraOptions::projection(const std::string & name, float fov, const glm::mat4x4 & base) {
    if (!name.empty() && name != "perspective" && name != "orthographic") {
        return false;
    }
    // only "perspective" reads fov
    projection_ = name;
    fov_ = fov;
    projectionNamed_ = true;
    base_ = base;
    return true;
}

bool CameraOptions::projectionNamed() const {
    return projectionNamed_;
}

unsigned int CameraOptions::width() const {
    return width_;
}

unsigned int CameraOptions::height() const {
    return height_;
}

float CameraOptions::pixelAspect() const {
    return pixelAspect_;
}

bool CameraOptions::perspective() const {
    return projection_ == "perspective";
}

float CameraOptions::hither() const {
    return hither_;
}

float CameraOptions::yon() const {
    return yon_;
}

glm::mat4x4 CameraOptions::screen() const {
    // RI states the composition in row vectors, with the projection on the right. Here a
    // matrix applies to what is on its right, so the projection goes on the left
    return projectionMatrix() * base_;
}

Camera CameraOptions::freeze(const glm::mat4x4 & view) const {
    Camera camera;
    camera.view = view;
    camera.screen = screen();
    /*
        The screen transform maps to the canonical volume [-1, 1], and the raster transform
        takes that to [0, xres] by [0, yres].

        y is negated because raster space has its origin at the upper left corner and
        counts downward. image::Image stores its rows in the same order, top row first.
    */
    const float x = static_cast<float>(width_) / 2.0F;
    const float y = static_cast<float>(height_) / 2.0F;
    glm::mat4x4 toRaster = glm::scale(glm::mat4x4(1.0F), glm::vec3(x, -y, 1.0F));
    toRaster = glm::translate(toRaster, glm::vec3(1.0F, -1.0F, 1.0F));
    camera.raster = toRaster * camera.screen;
    // raster divided by the resolution: x runs right and y runs down over [0, 1] from the
    // upper left corner. Screen space has y up over [-1, 1], so y is negated as raster
    // negates it. z is screen depth moved from [-1, 1] onto [0, 1]
    glm::mat4x4 normalised = glm::scale(glm::mat4x4(1.0F), glm::vec3(0.5F, -0.5F, 0.5F));
    normalised = glm::translate(normalised, glm::vec3(1.0F, -1.0F, 1.0F));
    camera.ndc = normalised * camera.screen;
    camera.perspective = perspective();
    camera.hither = hither_;
    camera.width = width_;
    camera.height = height_;
    return camera;
}

glm::mat4x4 CameraOptions::projectionMatrix() const {
    const float left = screen_.left;
    const float right = screen_.right;
    const float bottom = screen_.bottom;
    const float top = screen_.top;

    // the identity, which is RI_NULL's projection
    glm::mat4x4 projection(1.0F);
    if (projection_ == "perspective") {
        /*
            RI states fov as the full angle between screen space (-1, 0) and (1, 0), so a
            point at eye depth z reaches screen x = 1 at x = z * tan(fov / 2). The screen
            window then selects the part of screen space the image covers.

            The interface looks down +z, so w is +z rather than the -z a right handed
            system would write. Depth runs [-1, 1] to match the orthographic branch, and
            a renderer's frustum cull reads the same range.
        */
        const float tangent = std::tan(glm::radians(fov_) / 2.0F);
        projection = glm::mat4x4(0.0F);
        projection[0][0] = 2.0F / ((right - left) * tangent);
        projection[1][1] = 2.0F / ((top - bottom) * tangent);
        projection[2][0] = -(right + left) / (right - left);
        projection[2][1] = -(top + bottom) / (top - bottom);
        projection[2][2] = (yon_ + hither_) / (yon_ - hither_);
        projection[2][3] = 1.0F;
        projection[3][2] = -2.0F * yon_ * hither_ / (yon_ - hither_);
    } else if (projection_ == "orthographic") {
        /*
            The screen window maps to [-1, 1] on x and y, and [near, far] maps to [-1, 1]
            on z. glm indexes a matrix as [column][row], so the translation is column 3.

            [ 2 / (right - left)   0                    0                  -tx ]
            [ 0                    2 / (top - bottom)   0                  -ty ]
            [ 0                    0                    2 / (far - near)   -tz ]
            [ 0                    0                    0                    1 ]

            tx = (right + left) / (right - left)
            ty = (top + bottom) / (top - bottom)
            tz = (far + near) / (far - near)
        */
        projection[0][0] = 2.0F / (right - left);
        projection[3][0] = -(right + left) / (right - left);
        projection[1][1] = 2.0F / (top - bottom);
        projection[3][1] = -(top + bottom) / (top - bottom);
        // positive, because the interface looks down +z. The negated form is for a right
        // handed system, and here it would put the whole clip range behind the near plane
        projection[2][2] = 2.0F / (yon_ - hither_);
        projection[3][2] = -(yon_ + hither_) / (yon_ - hither_);
    }
    // any other projection is implementation specific. There is none, so it is the identity
    return projection;
}

};  // namespace v3d::render::offline
