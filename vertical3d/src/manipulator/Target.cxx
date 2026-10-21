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
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/tool/SelectMask.h>

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
 * The vertices the selected components of one kind bring, each once, in index order.
 **/
std::vector<v3d::brep::Index> selectedVertices(const v3d::brep::BRep& mesh, SelectMask kind) {
    std::vector<v3d::brep::Index> vertices;
    if (kind == SelectMask::Vertex) {
        for (v3d::brep::Index id = 0; id < mesh.vertexCount(); id++) {
            if (mesh.vertex(id)->selected()) {
                vertices.push_back(id);
            }
        }
    } else if (kind == SelectMask::Edge) {
        for (v3d::brep::Index id = 0; id < mesh.edgeCount(); id++) {
            if (!mesh.edge(id)->selected()) {
                continue;
            }
            vertices.push_back(mesh.edge(id)->vertex());
            v3d::brep::Index start = 0;
            if (startOf(mesh, id, &start)) {
                vertices.push_back(start);
            }
        }
    } else if (kind == SelectMask::Face) {
        for (v3d::brep::Index id = 0; id < mesh.faceCount(); id++) {
            if (!mesh.face(id)->selected()) {
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
Target Target::objects(const Meshes& meshes) {
    Target target;
    for (const boost::shared_ptr<v3d::brep::BRep>& mesh : meshes) {
        if (mesh) {
            target.meshes_.push_back(mesh);
        }
    }
    return target;
}

/**
 **/
Target Target::components(const Meshes& meshes, SelectMask kind) {
    Target target;
    target.components_ = true;
    for (const boost::shared_ptr<v3d::brep::BRep>& mesh : meshes) {
        if (!mesh) {
            continue;
        }
        std::vector<v3d::brep::Index> vertices = selectedVertices(*mesh, kind);
        if (vertices.empty()) {
            continue;
        }
        std::vector<v3d::brep::Index> faces = facesAround(*mesh, vertices);
        target.meshes_.push_back(mesh);
        target.parts_.push_back({.mesh = mesh, .vertices = std::move(vertices), .faces = std::move(faces)});
    }
    return target;
}

/**
 **/
Target Target::selected(const Scene& scene, SelectMask mask) {
    if (mask == SelectMask::Object) {
        return objects(scene.selection());
    }
    return components(scene.selection(), mask);
}

/**
 **/
bool Target::empty() const noexcept {
    return meshes_.empty();
}

/**
 **/
bool Target::components() const noexcept {
    return components_;
}

/**
 **/
const Meshes& Target::meshes() const noexcept {
    return meshes_;
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
        for (const boost::shared_ptr<v3d::brep::BRep>& mesh : meshes_) {
            sum += mesh->translation();
            count++;
        }
    } else {
        for (const Part& part : parts_) {
            const glm::mat4 matrix = part.mesh->matrix();
            for (const v3d::brep::Index vertex : part.vertices) {
                sum += glm::vec3(matrix * glm::vec4(part.mesh->vertex(vertex)->point(), 1.0F));
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
    if (meshes_.empty()) {
        return glm::quat(1.0F, 0.0F, 0.0F, 0.0F);
    }
    return meshes_.front()->rotation();
}

/**
 **/
void Target::move(const Motion& motion, const glm::vec3& pivot, const glm::quat& frame) const {
    // a distance from the pivot is held above the floor too, or a set or a group of
    // vertices could be turned inside out
    const glm::vec3 spread = glm::max(motion.stretch, glm::vec3(minimum));
    const glm::quat unframe = glm::inverse(frame);
    const auto place = [&](const glm::vec3& point) {
        const glm::vec3 stretched = frame * (spread * (unframe * (point - pivot)));
        return pivot + (motion.turn * stretched) + motion.offset;
    };

    if (!components_) {
        // multiplied on the left, because the turn is measured in world space and applies on
        // top of each object's own rotation. A scale is in the object's own axes
        for (const boost::shared_ptr<v3d::brep::BRep>& mesh : meshes_) {
            mesh->rotation(glm::normalize(motion.turn * mesh->rotation()));
            mesh->scale(glm::max(mesh->scale() * motion.stretch, glm::vec3(minimum)));
            mesh->translation(place(mesh->translation()));
        }
        return;
    }

    for (const Part& part : parts_) {
        const glm::mat4 matrix = part.mesh->matrix();
        const glm::mat4 inverse = glm::inverse(matrix);
        for (const v3d::brep::Index index : part.vertices) {
            v3d::brep::Vertex* vertex = part.mesh->vertex(index);
            const glm::vec3 world(matrix * glm::vec4(vertex->point(), 1.0F));
            vertex->point(glm::vec3(inverse * glm::vec4(place(world), 1.0F)));
        }
        // a face stores its normal, so every face with a moved corner is brought up to date.
        // A face squashed flat has no normal, and keeps the one it had
        for (const v3d::brep::Index face : part.faces) {
            const glm::vec3 normal = v3d::brep::newellNormal(*part.mesh, face);
            if (normal != glm::vec3(0.0F)) {
                part.mesh->face(face)->normal(normal);
            }
        }
    }
}

};  // namespace v3d::editor
