/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/brep/BRep.h>
#include <api/brep/Index.h>
#include <api/type/Transform.h>
#include <vertical3d/src/scene/Evaluation.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Mesh.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/selection/SelectMask.h>

#include <utility>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace v3d::editor {

/**
 * One motion event's worth of a manipulator's drag, about a pivot.
 *
 * A point moves by stretching along the frame's axes about the pivot, then turning about the
 * pivot, then moving by the offset. Each manipulator sets only its own part and leaves the
 * others at rest.
 **/
struct Motion {
    glm::vec3 offset{0.0F, 0.0F, 0.0F};
    glm::quat turn{1.0F, 0.0F, 0.0F, 0.0F};
    /**
     * How much to stretch along each axis of the frame. An object's own scale is
     * multiplied by it in the object's own axes.
     **/
    glm::vec3 stretch{1.0F, 1.0F, 1.0F};
};

/**
 * What a manipulator moves, and where its pivot is.
 *
 * In object mode the target is the selected objects, which move through their transforms.
 * A motion is measured in world space and written back in each object's parent's space, so
 * an object beneath another moves as the motion says. In a component mode it is the distinct
 * vertices of the selected components, which move in world space and are written back in
 * each object's own space, and the objects' transforms are left alone. A component target may
 * span several objects.
 *
 * The parents' world matrices, and in a component mode the objects' own, are taken from an
 * evaluation of the scene when the target is made. A drag moves none of them, because no
 * object of an object target is beneath another. An object target also takes each object's
 * placement from the evaluation, which a curve may give it, and a drag moves the object on
 * from there. A move writes the whole placement into the object.
 *
 * The pivot is the mean of the objects' origins, or of the distinct vertices, in world space.
 * For one object it is the object's origin.
 **/
class Target final {
 public:
    /**
     * The vertices one object contributes to a component target, and the faces around
     * them, whose normals a move changes.
     **/
    struct Part {
        boost::shared_ptr<Mesh> mesh;
        std::vector<v3d::brep::Index> vertices;
        std::vector<v3d::brep::Index> faces;
        // where the mesh is in the world
        glm::mat4 world{ 1.0F };
    };

    /**
     * No scale factor, and no factor a distance from the pivot is scaled by, goes below
     * this, so a drag cannot turn an object or a set inside out.
     **/
    static constexpr float minimum = 0.01F;

    /**
     * An empty target, which has no handles.
     **/
    Target();

    /**
     * Whole objects, of any kind, none of which may be beneath another.
     *
     * @param evaluation where the objects and their parents are
     **/
    static Target objects(const Items& items, const Evaluation& evaluation);

    /**
     * The vertices of the selected components of one kind, in every mesh of the set. An
     * edge brings its two vertices and a face every vertex of its loop. A vertex shared by
     * two selected components is in the target once.
     *
     * @param evaluation where the meshes are
     **/
    static Target components(const Meshes& meshes, SelectMask kind, const Evaluation& evaluation);

    /**
     * The target the selection makes under a select mask: the selected items of the kinds the
     * mask takes when it selects whole items, and the selected components of the selected
     * meshes otherwise. An item beneath another item the mask takes is left out, because it
     * moves with that item already.
     **/
    static Target selected(const Scene& scene, SelectMask mask);

    /**
     * @return whether there is nothing to move
     **/
    bool empty() const noexcept;

    /**
     * @return whether the target is vertices rather than whole objects
     **/
    bool components() const noexcept;

    /**
     * @return the items the target moves, or the meshes it moves parts of, in scene order
     **/
    const Items& items() const noexcept;

    /**
     * @return for a component target, each mesh's vertices and the faces around them
     **/
    const std::vector<Part>& parts() const noexcept;

    /**
     * @return the pivot in world space. The origin for an empty target
     **/
    glm::vec3 pivot() const;

    /**
     * @return the first object's rotation in the world, which the handles follow in its own
     *         axes
     **/
    glm::quat axes() const;

    /**
     * Move everything the target holds.
     *
     * @param pivot what the motion turns and stretches about, in world space
     * @param frame the axes the stretch is measured along
     **/
    void move(const Motion& motion, const glm::vec3& pivot, const glm::quat& frame);

 private:
    Items items_;
    // for an object target, the world matrix of each object's parent, in the order of items_
    std::vector<glm::mat4> parents_;
    // for an object target, each object's placement, as the evaluation found it and as each
    // move has left it since
    std::vector<v3d::type::Transform> placements_;
    std::vector<Part> parts_;
    bool components_;
};

};  // namespace v3d::editor
