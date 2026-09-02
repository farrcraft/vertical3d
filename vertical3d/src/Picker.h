/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Scene.h"
#include "SceneVisitor.h"
#include "SelectMask.h"
#include "ViewPort.h"

#include "../../api/brep/BRep.h"
#include "../../api/type/Camera.h"
#include "../../api/type/Ray.h"

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>

namespace v3d::editor {

    /**
     * What a click at a screen point hit, per ADR-0014.
     *
     * An object and a face are hit by the ray meeting a triangle of the mesh; a vertex and
     * an edge, by being drawn within a few pixels of the cursor.
     *
     * Component masks only consider meshes that are already selected: an object has to be
     * selected before any of its components may be.
     **/
    class Picker final : public SceneVisitor {
     public:
        /**
         * What one pick found.
         **/
        struct Hit final {
            /**
             * Whether anything was hit at all. Everything below is meaningless if not.
             **/
            bool valid;
            /**
             * The dag::Node id of the mesh, which is what a selection names.
             **/
            unsigned int mesh;
            /**
             * Which kind of thing was hit, which is the mask the pick was made under.
             **/
            SelectMask kind;
            /**
             * The index of the vertex, half edge or face within that mesh. Zero and
             * meaningless when the kind is Object.
             **/
            unsigned int component;

            Hit();
        };

        /**
         * @param tolerance how far from a vertex or an edge, in pixels, still counts as
         *        being on it
         **/
        explicit Picker(float tolerance = 5.0f);

        /**
         * Cast a click into a view.
         *
         * The view's camera matrices are rebuilt first, so a pick does not depend on a
         * frame having been drawn since the camera last moved.
         *
         * @param scene what to test against
         * @param view which view was clicked in, whose region the cursor is measured
         *        against
         * @param cursor where the click was, in window pixels
         * @param mask what kind of thing the click is looking for
         **/
        Hit pick(const Scene& scene, const ViewPort& view, const glm::vec2& cursor, SelectMask mask);

        /**
         * One mesh of the scene, during a pick. Public because a scene is walked through
         * SceneVisitor; nothing else should call it.
         **/
        void visit(const boost::shared_ptr<v3d::brep::BRep>& mesh) override;

     private:
        /**
         * Ray against every triangle of the mesh, nearest along the ray winning.
         **/
        void surface(const boost::shared_ptr<v3d::brep::BRep>& mesh);

        /**
         * Screen distance to every vertex, nearest to the camera among those within
         * tolerance winning.
         **/
        void vertices(const boost::shared_ptr<v3d::brep::BRep>& mesh);

        /**
         * Screen distance to every edge, resolved the same way.
         **/
        void edges(const boost::shared_ptr<v3d::brep::BRep>& mesh);

        /**
         * Where a point of the mesh lands on the screen, and whether it is in front of the
         * camera at all.
         **/
        bool screen(const glm::vec3& point, glm::vec2* position, float* depth) const;

        /**
         * Take the hit if it is nearer than the best so far.
         * @param distance along the ray, or the depth for a screen space pick
         **/
        void offer(unsigned int mesh, unsigned int component, float distance);

        float tolerance_;

        // the state of one pick, valid only for the duration of the walk
        SelectMask mask_;
        v3d::type::Ray ray_;
        v3d::type::Camera* camera_;
        int viewport_[4];
        glm::vec2 cursor_;
        glm::mat4 model_;
        float nearest_;
        Hit hit_;
    };

};  // namespace v3d::editor
