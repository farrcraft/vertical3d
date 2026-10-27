/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Target.h"

#include <api/brep/BRep.h>
#include <api/brep/Face.h>
#include <api/brep/HalfEdge.h>
#include <api/brep/Index.h>
#include <api/brep/Topology.h>
#include <api/brep/Vertex.h>
#include <api/type/Transform.h>
#include <vertical3d/src/scene/Components.h>
#include <vertical3d/src/scene/Evaluation.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Mesh.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/selection/SelectMask.h>

#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/common.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor {

namespace {

/**
 * The vertex a half edge starts at, which is the vertex the half edge before it in its
 * loop ends at. A half edge names only its end, so this scans for the one whose next it is.
 **/
bool startOf(const v3d::brep::BRep& mesh, v3d::brep::Index edge, v3d::brep::Index* vertex) {
    const std::size_t count = mesh.edgeCount();
    for (v3d::brep::Index id = 0; id < count; id++) {
        if (mesh.edge(id)->next() == edge) {
            *vertex = mesh.edge(id)->vertex();
            return true;
        }
    }
    return false;
}

/**
 * The vertices the selected components of one kind of a mesh item bring, each once, in index
 * order. A part the geometry does not hold brings nothing.
 **/
std::vector<v3d::brep::Index> selectedVertices(const Mesh& item, SelectMask kind, const Components& components) {
    const v3d::brep::BRep& mesh = *item.geometry();
    std::vector<v3d::brep::Index> vertices;
    if (kind == SelectMask::Vertex) {
        for (const v3d::brep::Index id : components.of(item.id(), Component::Vertex)) {
            if (id < mesh.vertexCount()) {
                vertices.push_back(id);
            }
        }
    } else if (kind == SelectMask::Edge) {
        for (const v3d::brep::Index id : components.of(item.id(), Component::Edge)) {
            if (id >= mesh.edgeCount()) {
                continue;
            }
            vertices.push_back(mesh.edge(id)->vertex());
            v3d::brep::Index start = 0;
            if (startOf(mesh, id, &start)) {
                vertices.push_back(start);
            }
        }
    } else if (kind == SelectMask::Face) {
        for (const v3d::brep::Index id : components.of(item.id(), Component::Face)) {
            if (id >= mesh.faceCount()) {
                continue;
            }
            for (const v3d::brep::Index edge : v3d::brep::faceLoop(mesh, id)) {
                vertices.push_back(mesh.edge(edge)->vertex());
            }
        }
    }
    std::sort(vertices.begin(), vertices.end());
    vertices.erase(std::unique(vertices.begin(), vertices.end()), vertices.end());
    return vertices;
}

/**
 * The faces any of the vertices is a corner of, each once, in index order.
 **/
std::vector<v3d::brep::Index> facesAround(const v3d::brep::BRep& mesh, const std::vector<v3d::brep::Index>& vertices) {
    std::vector<v3d::brep::Index> faces;
    for (const v3d::brep::Index vertex : vertices) {
        const std::vector<v3d::brep::Index> around = v3d::brep::facesAround(mesh, vertex);
        faces.insert(faces.end(), around.begin(), around.end());
    }
    std::sort(faces.begin(), faces.end());
    faces.erase(std::unique(faces.begin(), faces.end()), faces.end());
    return faces;
}

};  // namespace

/**
 **/
Target::Target() :
    components_(false) {
}

/**
 **/
Target Target::objects(const Items& items, const Evaluation& evaluation) {
    Target target;
    for (const boost::shared_ptr<Item>& item : items) {
        if (item) {
            target.items_.push_back(item);
            target.parents_.push_back(evaluation.placed(item->id()).parent);
            target.placements_.push_back(evaluation.pose(*item).placement);
        }
    }
    return target;
}

/**
 **/
Target Target::components(const Meshes& meshes, SelectMask kind, const Components& components,
    const Evaluation& evaluation) {
    Target target;
    target.components_ = true;
    for (const boost::shared_ptr<Mesh>& mesh : meshes) {
        if (!mesh) {
            continue;
        }
        std::vector<v3d::brep::Index> vertices = selectedVertices(*mesh, kind, components);
        if (vertices.empty()) {
            continue;
        }
        std::vector<v3d::brep::Index> faces = facesAround(*mesh->geometry(), vertices);
        target.items_.push_back(mesh);
        target.parts_.push_back({.mesh = mesh, .vertices = std::move(vertices), .faces = std::move(faces),
            .world = evaluation.placed(mesh->id()).world});
    }
    return target;
}

/**
 **/
Target Target::selected(const Scene& scene, SelectMask mask) {
    const Evaluation evaluation = scene.evaluate();
    if (selectsItems(mask)) {
        // a mask that takes one kind moves only that kind, though others may be selected
        Items taken;
        for (const boost::shared_ptr<Item>& item : scene.selection()) {
            if (takes(mask, item->kind())) {
                taken.push_back(item);
            }
        }
        // an item moved with its parent and again by the motion would move twice
        return objects(scene.uppermost(taken), evaluation);
    }
    return components(scene.selected<Mesh>(), mask, scene.components(), evaluation);
}

/**
 **/
bool Target::empty() const noexcept {
    return items_.empty();
}

/**
 **/
bool Target::components() const noexcept {
    return components_;
}

/**
 **/
const Items& Target::items() const noexcept {
    return items_;
}

/**
 **/
const std::vector<Target::Part>& Target::parts() const noexcept {
    return parts_;
}

/**
 **/
glm::vec3 Target::pivot() const {
    glm::vec3 sum(0.0F, 0.0F, 0.0F);
    std::size_t count = 0;
    if (!components_) {
        for (std::size_t index = 0; index < items_.size(); index++) {
            sum += glm::vec3(parents_[index] * glm::vec4(placements_[index].position, 1.0F));
            count++;
        }
    } else {
        for (const Part& part : parts_) {
            for (const v3d::brep::Index vertex : part.vertices) {
                sum += glm::vec3(part.world * glm::vec4(part.mesh->geometry()->vertex(vertex)->point(), 1.0F));
                count++;
            }
        }
    }
    if (count == 0) {
        return sum;
    }
    return sum / static_cast<float>(count);
}

/**
 **/
glm::quat Target::axes() const {
    if (items_.empty()) {
        return glm::quat(1.0F, 0.0F, 0.0F, 0.0F);
    }
    if (components_) {
        return orientation(parts_.front().world);
    }
    return glm::normalize(orientation(parents_.front()) * placements_.front().rotation);
}

/**
 **/
void Target::move(const Motion& motion, const glm::vec3& pivot, const glm::quat& frame) {
    // a distance from the pivot is held above the floor too, or a set or a group of
    // vertices could be turned inside out
    const glm::vec3 spread = glm::max(motion.stretch, glm::vec3(minimum));
    const glm::quat unframe = glm::inverse(frame);
    const auto place = [&](const glm::vec3& point) {
        const glm::vec3 stretched = frame * (spread * (unframe * (point - pivot)));
        return pivot + (motion.turn * stretched) + motion.offset;
    };

    if (!components_) {
        // the turn is measured in world space and applies on top of the object's rotation in
        // the world, so it is carried into the parent's axes and multiplied on the left. A
        // scale is in the object's own axes
        for (std::size_t index = 0; index < items_.size(); index++) {
            v3d::type::Transform& placement = placements_[index];
            const glm::mat4& parent = parents_[index];
            const glm::quat axes = orientation(parent);
            const glm::quat turn = glm::inverse(axes) * motion.turn * axes;
            placement.rotation = glm::normalize(turn * placement.rotation);
            placement.scale = glm::max(placement.scale * motion.stretch, glm::vec3(minimum));
            const glm::vec3 world(parent * glm::vec4(placement.position, 1.0F));
            placement.position = glm::vec3(glm::inverse(parent) * glm::vec4(place(world), 1.0F));
            items_[index]->placement(placement);
        }
        return;
    }

    for (const Part& part : parts_) {
        v3d::brep::BRep& geometry = *part.mesh->geometry();
        const glm::mat4& matrix = part.world;
        const glm::mat4 inverse = glm::inverse(matrix);
        for (const v3d::brep::Index index : part.vertices) {
            v3d::brep::Vertex* vertex = geometry.vertex(index);
            const glm::vec3 world(matrix * glm::vec4(vertex->point(), 1.0F));
            vertex->point(glm::vec3(inverse * glm::vec4(place(world), 1.0F)));
        }
        // a face stores its normal, so every face with a moved corner is brought up to date.
        // A face squashed flat has no normal, and keeps the one it had
        for (const v3d::brep::Index face : part.faces) {
            const glm::vec3 normal = v3d::brep::newellNormal(geometry, face);
            if (normal != glm::vec3(0.0F)) {
                geometry.face(face)->normal(normal);
            }
        }
        // the parts were changed in place, which the mesh does not see for itself
        geometry.touch();
    }
}

};  // namespace v3d::editor
