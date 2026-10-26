/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/Camera.h>
#include <api/render/offline/rib/Requests.h>

#include <string>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>

namespace v3d::render::offline {

/**
 * RI's camera options, and the camera they give.
 *
 * Format sets the frame aspect, and the frame aspect sets the screen window. Once a scene names
 * the frame aspect or the screen window, that value stops following. freeze() builds the camera
 * from the options as they stand.
 **/
class CameraOptions final {
 public:
    /** The picture's size when no Format names one. **/
    static constexpr unsigned int defaultWidth = 320;
    static constexpr unsigned int defaultHeight = 240;
    /** RI's default frame aspect ratio, the aspect of defaultWidth by defaultHeight. **/
    static constexpr float defaultFrameAspectRatio = 4.0F / 3.0F;
    /** RI's default "fov" for a "perspective" projection, in degrees. **/
    static constexpr float defaultFov = 90.0F;
    /** RI's default clipping range: RI_EPSILON to RI_INFINITY. **/
    static constexpr float defaultHither = 1.0e-10F;
    static constexpr float defaultYon = 1.0e38F;

    /**
     * RiFormat: the pixel resolution, x then y, and the pixel aspect ratio. A side of zero or
     * less takes the default for that side. An aspect that is not a positive finite number is
     * square pixels.
     *
     * @return false, keeping the size the options had, when a side is above largestResolution
     **/
    bool format(const glm::ivec2 & resolution, float pixelAspect);
    /**
     * RiFrameAspectRatio: the ratio of the width of the whole image to its height. format()
     * sets it from the pixel resolution until a scene calls this, and from then on leaves it.
     **/
    void frameAspectRatio(float aspect);
    /**
     * RiScreenWindow: the rectangle of screen space the image covers. Until a scene calls this,
     * it follows the frame aspect a: [-a, a] by [-1, 1] when a is at least 1, and [-1, 1] by
     * [-1/a, 1/a] otherwise.
     **/
    void screenWindow(const rib::ScreenWindow & window);
    /** RiClipping: the camera space depth of the near and far clipping planes. **/
    void clipping(const rib::Clipping & planes);
    /**
     * RiProjection. An empty name is RI_NULL, which applies no projection, so camera space is
     * screen space. "perspective" reads fov, the full angle in degrees across screen x from -1
     * to 1. "orthographic" maps the screen window and the clipping range onto the canonical
     * volume. The projection applies after base, the transformation in force at the request.
     *
     * @return false, keeping the projection in force, for any other name
     **/
    bool projection(const std::string & name, float fov, const glm::mat4x4 & base);
    /** Whether a scene named a projection. Without one, a scene is seen orthographically. **/
    bool projectionNamed() const;

    unsigned int width() const;
    unsigned int height() const;
    float pixelAspect() const;
    bool perspective() const;
    float hither() const;
    float yon() const;

    /** Camera to screen: the projection after the transformation in force at RiProjection. **/
    glm::mat4x4 screen() const;
    /**
     * The camera the options give, seen from a world to camera transformation.
     **/
    Camera freeze(const glm::mat4x4 & view) const;

 private:
    /** The matrix the projection name, field of view, screen window and clipping give. **/
    glm::mat4x4 projectionMatrix() const;

    unsigned int width_ = defaultWidth;
    unsigned int height_ = defaultHeight;
    float pixelAspect_ = 1.0F;
    float frameAspect_ = defaultFrameAspectRatio;
    rib::ScreenWindow screen_{ .left = -defaultFrameAspectRatio, .right = defaultFrameAspectRatio, .bottom = -1.0F, .top = 1.0F };
    bool frameAspectNamed_ = false;
    bool screenNamed_ = false;
    bool projectionNamed_ = false;
    std::string projection_ = "orthographic";
    float fov_ = defaultFov;
    glm::mat4x4 base_ = glm::mat4x4(1.0F);
    float hither_ = defaultHither;
    float yon_ = defaultYon;
};

};  // namespace v3d::render::offline
