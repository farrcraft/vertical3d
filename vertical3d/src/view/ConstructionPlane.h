/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/LineCanvas.h>
#include <api/type/camera/Camera.h>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor {

/**
 * The grid a viewport measures against, drawn as lines.
 *
 * A square of evenly spaced lines, with every nth one emphasised and the two through
 * the origin emphasised again. The emphasis is colour rather than width, because lines
 * are one pixel wide - ADR-0011.
 *
 * The plane the grid lies in follows the camera. An orthographic view is looking along
 * one axis, so the grid lies in the plane of the other two and the view reads as a
 * drafting elevation; a perspective view gets the ground plane.
 *
 * This belongs to the editor rather than to the api: LineCanvas offers primitives, and
 * how big a grid is, how far apart its lines are and which way it faces are all policy.
 **/
class ConstructionPlane final {
 public:
    /**
     **/
    ConstructionPlane();

    /**
     * The world distance between adjacent lines.
     **/
    void spacing(float spacing) noexcept;

    /**
     * @return the world distance between adjacent lines
     **/
    float spacing() const noexcept;

    /**
     * How many lines apart the emphasised ones are.
     **/
    void intervals(unsigned int intervals) noexcept;

    /**
     * @return how many lines apart the emphasised ones are
     **/
    unsigned int intervals() const noexcept;

    /**
     * How many lines the grid is across.
     **/
    void lines(unsigned int lines) noexcept;

    /**
     * @return how many lines the grid is across
     **/
    unsigned int lines() const noexcept;

    /**
     * Append the grid to a canvas, in the plane the camera makes sense of.
     *
     * @param camera the view the grid is being drawn for, which decides its plane
     * @param canvas where the segments are appended - nothing is cleared
     **/
    void draw(const v3d::type::camera::Camera& camera, v3d::render::realtime::LineCanvas* canvas) const;

 private:
    /**
     * The two axes the grid is spanned by, which are the camera's own right and up under
     * an orthographic view and the ground plane under a perspective one.
     **/
    static void axes(const v3d::type::camera::Camera& camera, glm::vec3* right, glm::vec3* up);

    float spacing_;
    unsigned int intervals_;
    unsigned int lines_;
    glm::vec4 minor_;
    glm::vec4 major_;
    glm::vec4 origin_;
};

};  // namespace v3d::editor
