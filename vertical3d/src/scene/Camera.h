/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>

#include "Item.h"

#include <glm/vec2.hpp>

namespace v3d::editor {

/**
 * A camera placed in the scene, which a render can be made from.
 *
 * It is not a view's camera: each view owns its own, and this one is part of the document.
 * It looks along its own +z axis with +y up, as a camera profile with no rotation does, and
 * its transform places and turns it. Its scale is ignored.
 **/
class Camera final : public Item {
 public:
    static constexpr Kind KIND = Kind::Camera;

    enum class Projection : std::uint8_t { Perspective, Orthographic };

    /**
     * A new camera's angle of view in degrees, and its near and far clipping distances.
     **/
    static constexpr float defaultFov = 45.0F;
    static constexpr float defaultNear = 0.1F;
    static constexpr float defaultFar = 1000.0F;

    /**
     * How the camera projects the scene.
     **/
    struct Lens {
        Projection projection = Projection::Perspective;
        /**
         * The full angle across the shorter side of the picture, in degrees, for a
         * perspective camera.
         **/
        float fov = defaultFov;
        /**
         * Half the height of what an orthographic camera sees, in world units.
         **/
        float extent = 1.0F;
        /**
         * The near and far clipping distances along the view direction, in that order.
         **/
        glm::vec2 clipping{ defaultNear, defaultFar };

        /**
         * Whether every field is finite and in its range. The angle of view is above zero and
         * below a straight angle, and the extent is above zero. The near distance is above
         * zero, and the far one is beyond it. A scene document holds only a valid lens.
         **/
        bool valid() const;
    };

    Camera();

    Kind kind() const noexcept override;

    const Lens& lens() const noexcept;
    void lens(const Lens& lens) noexcept;

 private:
    Lens lens_;
};

};  // namespace v3d::editor
