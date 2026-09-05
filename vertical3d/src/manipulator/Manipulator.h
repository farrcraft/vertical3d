/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "../view/ViewPort.h"

#include "../../../api/brep/BRep.h"
#include "../../../api/render/realtime/LineCanvas.h"

#include <boost/shared_ptr.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor {

    /**
     * The handles that move, turn and resize the selection.
     *
     * A manipulator writes through the mesh's dag::Transform and never through its
     * geometry, so it acts on the object even when a component is what is selected. That is
     * also why it is drawn at the object's own origin: the transform pivots there, and
     * handles drawn anywhere else would lie about where a rotation turns.
     *
     * Handles are picked by their own test against the cursor, per ADR-0014, and drawn
     * through LineCanvas, which is the only primitive there is. They are sized in world
     * units taken from the view, so a handle keeps roughly the same length on screen
     * however far away the object is.
     *
     * A drag is one motion event at a time: apply() is given the two cursor positions
     * either side of the move and adds what it measures to the transform. Writing an
     * absolute position instead would snap the object to the gesture rather than move it
     * by the gesture.
     **/
    class Manipulator {
     public:
        /**
         * Which handle a drag is constrained to. None is the centre handle, which moves in
         * the plane of the screen rather than along an axis.
         **/
        enum class Axis {
            None,
            X,
            Y,
            Z
        };

        /**
         * Whether the handles line up with the world axes or with the object's own.
         **/
        enum class Space {
            Global,
            Local
        };

        Manipulator();
        virtual ~Manipulator();

        Manipulator(const Manipulator&) = delete;
        Manipulator& operator=(const Manipulator&) = delete;

        /**
         * @return which handle is highlighted, which is what a drag is constrained to
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
             * False when there is nothing to draw for - no mesh, or a view with no area.
             **/
            bool valid;
            /**
             * The object's origin in world space, which is where its transform pivots.
             **/
            glm::vec3 origin;
            /**
             * What the handles are aligned to.
             **/
            glm::quat orientation;
            /**
             * How long a handle is, in world units.
             **/
            float size;

            Placement();
        };

        /**
         * Where this manipulator sits for a mesh seen through a view.
         *
         * The view's camera matrices are rebuilt first, so a handle answers for where the
         * camera is now rather than for where it was when the last frame was drawn.
         **/
        Placement placement(const boost::shared_ptr<v3d::brep::BRep>& mesh, const ViewPort& view) const;

        /**
         * Which handle the cursor is over.
         *
         * @param axis where the answer goes - untouched when nothing is under the cursor
         * @return whether a handle is
         **/
        virtual bool grab(const boost::shared_ptr<v3d::brep::BRep>& mesh, const ViewPort& view,
            const glm::vec2& cursor, Axis* axis) const;

        /**
         * Append the handles to a canvas, in world space. Nothing is drawn without a
         * selection.
         **/
        virtual void draw(const boost::shared_ptr<v3d::brep::BRep>& mesh, const ViewPort& view,
            v3d::render::realtime::LineCanvas* canvas) const = 0;

        /**
         * Apply one motion event's worth of drag along the constrained axis.
         *
         * @param from where the cursor was
         * @param to where it is
         **/
        virtual void apply(const boost::shared_ptr<v3d::brep::BRep>& mesh, const ViewPort& view,
            const glm::vec2& from, const glm::vec2& to) const = 0;

     protected:
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
         * What the coordinate space means to this manipulator. Scale overrides it: a non
         * uniform scale is a vector in the object's own axes and has no global form.
         **/
        virtual Space effective() const noexcept;

        /**
         * A handle's direction in world space. The centre handle has none and answers with
         * zero.
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
         * The camera's axes in world space, read out of the view matrix rather than off the
         * profile - the profile's normals do not follow its rotation.
         * @param forward the direction into the screen - may be null
         **/
        static void basis(const ViewPort& view, glm::vec3* right, glm::vec3* up, glm::vec3* forward);

        /**
         * Two unit vectors spanning the plane a direction is normal to, for drawing a ring
         * or a cap about it.
         **/
        static void perpendiculars(const glm::vec3& direction, glm::vec3* first, glm::vec3* second);

        /**
         * How far a point is from a segment, both in screen pixels.
         *
         * A handle is a run of segments however it is drawn - a shaft is one and a ring is
         * as many as it is approximated with - so this is what says whether the cursor is on
         * one. Testing the points a ring is drawn through instead would leave the gaps
         * between them ungrabbable, and the gaps grow with the ring.
         **/
        static float distanceToSegment(const glm::vec2& from, const glm::vec2& to, const glm::vec2& target);

        /**
         * How far along a handle a drag went, in world units.
         *
         * The handle is projected to the screen and the drag's component along it taken,
         * which is what makes a gesture across the screen move the object the way the
         * handle points. A handle pointing at the viewer projects to nothing and answers
         * with zero rather than with a division by it.
         **/
        float along(const ViewPort& view, const Placement& placement, Axis axis,
            const glm::vec2& from, const glm::vec2& to) const;

        /**
         * The box drawn at the centre handle and at the tip of a scale handle, oriented
         * with the placement.
         **/
        void marker(v3d::render::realtime::LineCanvas* canvas, const Placement& placement,
            const glm::vec3& at, float size, const glm::vec4& colour) const;

     private:
        Axis axis_;
        bool active_;
        Space space_;
    };

};  // namespace v3d::editor
