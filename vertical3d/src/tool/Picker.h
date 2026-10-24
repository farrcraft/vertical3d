/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/brep/BRep.h>
#include <api/type/camera/Camera.h>
#include <api/type/geometry/Ray.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/scene/SceneVisitor.h>
#include <vertical3d/src/view/ViewPort.h>

#include "SelectMask.h"

#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>

namespace v3d::editor {

/**
 * What a click at a screen point hit, found by a ray cast on the CPU, and what a rectangle
 * on the screen holds.
 *
 * An object or a face is hit when the ray meets a triangle of the mesh. A vertex or an edge
 * is hit when it is drawn within a few pixels of the cursor.
 *
 * A component mask considers only meshes that are already selected: an object has to be
 * selected before any of its components can be.
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
         * The dag::Node id of the mesh. A selection names a mesh by this id.
         **/
        unsigned int mesh;
        /**
         * Which kind of thing was hit. It is the mask the pick was made under.
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
     * How far from a vertex or an edge, in pixels, a click still counts as being on it
     * when no tolerance is given.
     **/
    static constexpr float defaultTolerance = 5.0F;

    /**
     * @param tolerance how far from a vertex or an edge, in pixels, still counts as
     *        being on it
     **/
    explicit Picker(float tolerance = defaultTolerance);

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
     * Find everything a rectangle on the screen holds, as a rubber band selects it.
     *
     * An object is inside when every corner of its bound projects inside. A vertex is
     * inside when it projects inside, an edge when both its ends do, and a face when every
     * vertex of its loop does. Hidden parts count, because this tests geometry and not
     * pixels. A point behind the camera is never inside.
     *
     * @param corner one corner of the rectangle, in window pixels
     * @param opposite the corner across from it. Either corner may be the smaller
     * @return one hit for each thing inside, in scene order
     **/
    std::vector<Hit> inside(const Scene& scene, const ViewPort& view, const glm::vec2& corner,
        const glm::vec2& opposite, SelectMask mask);

    /**
     * One mesh of the scene, during a pick. Public because the scene calls it through
     * SceneVisitor. Only the scene calls it.
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
     * Everything of the mesh inside the rectangle, under the mask.
     **/
    void enclosed(const boost::shared_ptr<v3d::brep::BRep>& mesh);

    /**
     * The mesh, if every corner of its bound is inside the rectangle.
     **/
    void enclosedObject(const boost::shared_ptr<v3d::brep::BRep>& mesh);

    /**
     * Every vertex inside the rectangle.
     **/
    void enclosedVertices(const boost::shared_ptr<v3d::brep::BRep>& mesh);

    /**
     * Every edge or face inside the rectangle, found by walking each face's loop.
     **/
    void enclosedLoops(const boost::shared_ptr<v3d::brep::BRep>& mesh);

    /**
     * Take the view's camera and region for one traversal.
     * @return false when the view has no camera or no area, and nothing can be found in it
     **/
    bool begin(const ViewPort& view, SelectMask mask);

    /**
     * Where a point of the mesh lands on the screen, and whether it is in front of the
     * camera at all.
     **/
    bool screen(const glm::vec3& point, glm::vec2* position, float* depth) const;

    /**
     * Whether a point of the mesh lands inside the rectangle.
     **/
    bool encloses(const glm::vec3& point) const;

    /**
     * A mesh, a component of it, and how far away the pick found it: along the ray, or the
     * depth for a screen space pick.
     **/
    struct Candidate {
        unsigned int mesh = 0;
        unsigned int component = 0;
        float distance = 0.0F;
    };

    /**
     * Take the candidate if it is nearer than the best so far.
     **/
    void offer(const Candidate& candidate);

    float tolerance_;

    // the state of one pick, valid only while the scene is being traversed
    SelectMask mask_;
    v3d::type::geometry::Ray ray_;
    v3d::type::camera::Camera* camera_;
    int viewport_[4];
    glm::vec2 cursor_;
    glm::mat4 model_;
    float nearest_;
    Hit hit_;

    // the state of one rectangle, valid only while the scene is being traversed
    bool band_;
    glm::vec2 min_;
    glm::vec2 max_;
    std::vector<Hit> inside_;
};

};  // namespace v3d::editor
