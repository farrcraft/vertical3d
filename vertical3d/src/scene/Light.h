/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>

#include "Item.h"

#include <glm/vec3.hpp>

namespace v3d::editor {

/**
 * A light placed in the scene: a point, distant or spot light, as RI's standard
 * pointlight, distantlight and spotlight shaders define them.
 *
 * Its transform places it, and a distant or spot light shines along its own +z axis. Its
 * scale is ignored.
 **/
class Light final : public Item {
 public:
    static constexpr Kind KIND = Kind::Light;

    enum class Type : std::uint8_t { Point, Distant, Spot };

    /**
     * The RI spotlight's own cone: a half angle of 30 degrees with a soft edge of 5, in
     * radians.
     **/
    static constexpr float defaultCone = 0.523599F;
    static constexpr float defaultDelta = 0.0872665F;

    /**
     * What the light gives off. The defaults are the RI shaders' own.
     **/
    struct Emission {
        Type type = Type::Point;
        glm::vec3 colour{ 1.0F, 1.0F, 1.0F };
        float intensity = 1.0F;
        /**
         * A spot light's half angle, and the width of its soft edge inside that angle, both
         * in radians.
         **/
        float cone = defaultCone;
        float delta = defaultDelta;

        /**
         * Whether every field is finite and in its range. The colour and the intensity are
         * zero or more. The cone is above zero and below half a turn, and the soft edge is
         * from zero up to the cone. A scene document holds only a valid emission.
         **/
        bool valid() const;
    };

    Light();

    Kind kind() const noexcept override;

    const Emission& emission() const noexcept;
    void emission(const Emission& emission) noexcept;

 private:
    Emission emission_;
};

};  // namespace v3d::editor
