/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/brep/BRep.h>
#include <api/render/realtime/canvas/Line.h>
#include <vertical3d/src/view/ViewPort.h>

#include "Target.h"

#include <cstdint>

#include <boost/shared_ptr.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor {

/**
 * The handles that move, turn and resize the selection.
 *
 * A manipulator moves a Target: whole objects through their transforms, or the vertices of
 * selected components. It is drawn at the target's pivot, the mean of the objects' origins
 * or of the vertices, and turns and scales about it, so a drag does what the handles show.
 * For one object the pivot is its origin, where its transform pivots. For several, a turn
 * or a scale moves each object about the pivot, which changes its translation as well.
 *
 * Handles are picked by testing each one against the cursor on the CPU, and drawn through
 * canvas::Line. They are sized in world units taken from the view, so a handle keeps
 * roughly the same length on screen however far away the object is.
 *
 * A drag is applied one motion event at a time. apply() is given the cursor positions
 * before and after the move, and each manipulator measures a Motion from them, which the
 * target adds to what it moves. Writing an absolute position instead would snap the object
 * to the gesture, not move it by the gesture.
 **/
class Manipulator {
 public:
    /**
     * Which handle a drag is constrained to. None is the centre handle, which moves in
     * the plane of the screen rather than along an axis.
     **/
    enum class Axis : std::uint8_t {
        None,
        X,
        Y,
        Z
    };

    /**
     * Whether the handles line up with the world axes or with the object's own.
     **/
    enum class Space : std::uint8_t {
        Global,
        Local
    };

    Manipulator();
    virtual ~Manipulator();

    Manipulator(const Manipulator&) = delete;
    Manipulator& operator=(const Manipulator&) = delete;

    /**
     * @return which handle is highlighted. A drag is constrained to that handle.
     **/
    Axis axis() const noexcept;

    /**
     * Constrain a drag, or highlight what the cursor is over.
     **/
    void axis(Axis axis) noexcept;

    /**
     * @return whether any handle is highlighted at all
     **/
    bool active() const noexcept;

    /**
     * Whether any handle is highlighted. The axis alone cannot say: None is the centre
     * handle rather than the absence of one.
     **/
    void active(bool active) noexcept;

    /**
     **/
    Space space() const noexcept;

    /**
     **/
    void space(Space space) noexcept;

    /**
     * Where the handles are drawn, and how big.
     **/
    struct Placement final {
        /**
         * False when there is nothing to draw for: an empty target, or a view with no area.
         **/
        bool valid;
        /**
         * The target's pivot in world space. A turn or a scale is about this point.
         **/
        glm::vec3 origin;
        /**
         * What the handles are aligned to, and what a stretch is measured along. In local
         * space, the first object's axes.
         **/
        glm::quat orientation;
        /**
         * How long a handle is, in world units.
         **/
        float size;

        Placement();
    };

    /**
     * Where this manipulator sits for a target seen through a view.
     *
     * The view's camera matrices are rebuilt first, so the handles match where the camera
     * is now rather than where it was when the last frame was drawn.
     **/
    Placement placement(const Target& target, const ViewPort& view) const;

    /**
     * Which handle the cursor is over.
     *
     * @param axis set to the handle under the cursor, and untouched when there is none
     * @return whether a handle is under the cursor
     **/
    virtual bool grab(const Target& target, const ViewPort& view,
        const glm::vec2& cursor, Axis* axis) const;

    /**
     * Append the handles to a canvas, in world space. Nothing is drawn for an empty target.
     **/
    virtual void draw(const Target& target, const ViewPort& view,
        v3d::render::realtime::canvas::Line* canvas) const = 0;

    /**
     * Apply one motion event's worth of drag along the constrained axis to the target.
     *
     * @param from where the cursor was
     * @param to where it is
     **/
    void apply(const Target& target, const ViewPort& view, const glm::vec2& from, const glm::vec2& to) const;

 protected:
    /**
     * What one motion event's worth of drag along the constrained axis does.
     *
     * @param seat where the handles are
     * @return false when the drag does nothing, and the target is left alone
     **/
    virtual bool measure(const Placement& seat, const ViewPort& view, const glm::vec2& from, const glm::vec2& to,
        Motion* motion) const = 0;

    /**
     * How long a handle is drawn, in pixels. The world size a placement carries is this
     * converted through the view.
     **/
    static const float handlePixels;

    /**
     * How far from a handle, in pixels, still counts as being on it.
     **/
    static const float tolerance;

    /**
     * How many times the tolerance the centre handle reaches, so that it can be picked
     * where the axes crowd it.
     **/
    static constexpr float centreReach = 1.5F;

    /**
     * What the coordinate space means to this manipulator. Scale overrides it: a non
     * uniform scale is a vector in the object's own axes and has no global form.
     **/
    virtual Space effective() const noexcept;

    /**
     * A handle's direction in world space. The centre handle has none and returns zero.
     **/
    static glm::vec3 direction(const Placement& placement, Axis axis);

    /**
     * The colour a handle is drawn in, highlighted when it is the active one.
     **/
    glm::vec4 colour(Axis axis) const;

    /**
     * Where a world point lands in the view, in window pixels.
     * @return false if the view has no area, or the point is not in front of the camera
     **/
    static bool project(const ViewPort& view, const glm::vec3& point, glm::vec2* position);

    /**
     * The world units one pixel of the view covers at a point. Measured by unprojecting
     * a pixel step rather than from the projection's terms, so the perspective and the
     * orthographic cases are one piece of code.
     **/
    static float unitsPerPixel(const ViewPort& view, const glm::vec3& at);

    /**
     * The camera's axes in world space, read out of the view matrix the view draws with, so
     * the handles line up with what is on screen. A view with no camera gives the world's
     * axes, with forward along +z.
     **/
    struct Basis {
        glm::vec3 right{1.0F, 0.0F, 0.0F};
        glm::vec3 up{0.0F, 1.0F, 0.0F};
        glm::vec3 forward{0.0F, 0.0F, 1.0F};  /**< into the screen **/
    };

    static Basis basis(const ViewPort& view);

    /**
     * Two unit vectors spanning the plane a direction is normal to, for drawing a ring
     * or a cap about it.
     **/
    static void perpendiculars(const glm::vec3& direction, glm::vec3* first, glm::vec3* second);

    /**
     * How far a point is from a segment, both in screen pixels.
     *
     * A handle is a run of segments however it is drawn. A shaft is one segment, and a
     * ring is as many as it is approximated with. This test decides whether the cursor is
     * on one. Testing only the points a ring is drawn through would leave the gaps between
     * them ungrabbable, and the gaps grow with the ring.
     **/
    static float distanceToSegment(const glm::vec2& from, const glm::vec2& to, const glm::vec2& target);

    /**
     * How far along a handle a drag went, in world units.
     *
     * The handle is projected to the screen and the drag's component along it is taken,
     * so a gesture across the screen moves the object the way the handle points. A handle
     * pointing at the viewer projects to a point and returns zero rather than dividing
     * by its zero length.
     **/
    static float along(const ViewPort& view, const Placement& placement, Axis axis,
        const glm::vec2& from, const glm::vec2& to);

    /**
     * The box drawn at the centre handle and at the tip of a scale handle, oriented
     * with the placement.
     **/
    static void marker(v3d::render::realtime::canvas::Line* canvas, const Placement& placement,
        const glm::vec3& at, float size, const glm::vec4& colour);

 private:
    Axis axis_;
    bool active_;
    Space space_;
};

};  // namespace v3d::editor
