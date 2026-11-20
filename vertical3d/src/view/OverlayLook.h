/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <array>

#include <glm/vec4.hpp>

namespace v3d::editor::view {

/**
 * The colours the overlays are drawn in.
 **/
struct OverlayLook final {
    static constexpr glm::vec4 X_AXIS{ 0.851F, 0.349F, 0.373F, 1.0F };
    static constexpr glm::vec4 Y_AXIS{ 0.447F, 0.690F, 0.322F, 1.0F };
    static constexpr glm::vec4 Z_AXIS{ 0.333F, 0.565F, 0.910F, 1.0F };
    static constexpr glm::vec4 LABEL{ 0.067F, 0.071F, 0.078F, 1.0F };
    static constexpr glm::vec4 REST{ 1.0F, 1.0F, 1.0F, 0.05F };
    static constexpr glm::vec4 HOVER{ 1.0F, 1.0F, 1.0F, 0.10F };
    static constexpr glm::vec4 PASSEPARTOUT{ 0.0F, 0.0F, 0.0F, 0.45F };
    static constexpr float TEXT = 11.0F;
    static constexpr float LINE = 2.0F;

    std::array<glm::vec4, 3> axes{ X_AXIS, Y_AXIS, Z_AXIS };   /**< x, y and z **/
    glm::vec4 label = LABEL;                                  /**< a letter on a ball **/
    glm::vec4 rest = REST;                                    /**< the disc behind the gizmo **/
    glm::vec4 hover = HOVER;                                  /**< the disc behind the gizmo under the cursor **/
    glm::vec4 lit{ 1.0F };                                    /**< the ring around the ball under the cursor **/
    glm::vec4 passepartout = PASSEPARTOUT;                    /**< laid over a camera's view outside the render's frame **/
    float text = TEXT;                                        /**< how large a letter is drawn, in pixels **/
    float line = LINE;                                        /**< how thick a line is, and a ring around a ball **/
};

};  // namespace v3d::editor::view
