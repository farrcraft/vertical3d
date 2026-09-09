/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Picker.h"

#include <vertical3d/src/scene/MeshTopology.h>

#include <cstddef>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor {

namespace {

/**
 * Where along a segment the closest point to a target is, as a fraction of it,
 * clamped to the segment's ends. A degenerate segment answers with its start.
 **/
float closest(const glm::vec2& from, const glm::vec2& to, const glm::vec2& target) {
    const glm::vec2 along = to - from;
    const float length = glm::dot(along, along);
    if (length <= 0.0f) {
        return 0.0f;
    }
    const float u = glm::dot(target - from, along) / length;
    if (u < 0.0f) {
        return 0.0f;
    }
    if (u > 1.0f) {
        return 1.0f;
    }
    return u;
}

};  // namespace

/**
 **/
Picker::Hit::Hit() :
valid(false),
mesh(0),
kind(SelectMask::Object),
component(0) {
}

/**
 **/
Picker::Picker(float tolerance) :
    tolerance_(tolerance),
    mask_(SelectMask::Object),
    camera_(nullptr),
    cursor_(0.0f, 0.0f),
    model_(1.0f),
    nearest_(0.0f) {
    viewport_[0] = 0;
    viewport_[1] = 0;
    viewport_[2] = 0;
    viewport_[3] = 0;
}

/**
 **/
Picker::Hit Picker::pick(const Scene& scene, const ViewPort& view, const glm::vec2& cursor, SelectMask mask) {
    hit_ = Hit();
    hit_.kind = mask;
    mask_ = mask;
    cursor_ = cursor;
    nearest_ = 0.0f;

    boost::shared_ptr<v3d::type::camera::Camera> camera = view.camera();
    const glm::vec4& region = view.region();
    if (!camera || region.z <= 0.0f || region.w <= 0.0f) {
        return hit_;
    }

    viewport_[0] = static_cast<int>(region.x);
    viewport_[1] = static_cast<int>(region.y);
    viewport_[2] = static_cast<int>(region.z);
    viewport_[3] = static_cast<int>(region.w);

    // the camera as it stands, not as it was last drawn: a click can arrive after a
    // camera move and before the next frame
    camera->createProjection();
    camera->createView();
    camera_ = camera.get();
    ray_ = camera_->ray(cursor, viewport_);

    scene.accept(this);

    camera_ = nullptr;
    return hit_;
}

/**
 **/
void Picker::visit(const boost::shared_ptr<v3d::brep::BRep>& mesh) {
    if (!mesh || camera_ == nullptr) {
        return;
    }

    // an object has to be selected before any of its components may be, which is what
    // keeps a component click inside the mesh the user is working on
    if (mask_ != SelectMask::Object && !mesh->selected()) {
        return;
    }

    model_ = mesh->matrix();

    switch (mask_) {
    case SelectMask::Vertex:
        vertices(mesh);
        break;
    case SelectMask::Edge:
        edges(mesh);
        break;
    case SelectMask::Object:
    case SelectMask::Face:
    default:
        surface(mesh);
        break;
    }
}

/**
 **/
void Picker::surface(const boost::shared_ptr<v3d::brep::BRep>& mesh) {
    // transformed() does not renormalise, so this distance is the same number it would
    // be in world space and hits on differently scaled meshes stay comparable
    const v3d::type::geometry::Ray local = ray_.transformed(glm::inverse(model_));

    float distance = 0.0f;
    if (!local.intersects(mesh->bound(), &distance)) {
        return;
    }

    const std::size_t faces = mesh->faceCount();
    for (std::size_t face = 0; face < faces; face++) {
        const std::vector<unsigned int> loop = faceLoop(mesh, static_cast<unsigned int>(face));
        if (loop.size() < 3) {
            continue;
        }

        // a fan from the loop's first vertex is correct only for a planar convex
        // face, which is all the primitives and the modelling operations produce
        glm::vec3 first;
        glm::vec3 second;
        glm::vec3 third;
        if (!loopSegment(mesh, loop, 0, &first, nullptr)) {
            continue;
        }
        for (std::size_t index = 1; index + 1 < loop.size(); index++) {
            if (!loopSegment(mesh, loop, index, &second, &third)) {
                continue;
            }
            if (local.intersects(first, second, third, &distance)) {
                offer(mesh->id(), static_cast<unsigned int>(face), distance);
            }
        }
    }
}

/**
 **/
void Picker::vertices(const boost::shared_ptr<v3d::brep::BRep>& mesh) {
    const std::size_t count = mesh->vertexCount();
    for (std::size_t index = 0; index < count; index++) {
        v3d::brep::Vertex* vertex = mesh->vertex(static_cast<unsigned int>(index));
        if (vertex == nullptr) {
            continue;
        }
        glm::vec2 position;
        float depth = 0.0f;
        if (!screen(vertex->point(), &position, &depth)) {
            continue;
        }
        if (glm::distance(position, cursor_) > tolerance_) {
            continue;
        }
        offer(mesh->id(), static_cast<unsigned int>(index), depth);
    }
}

/**
 **/
void Picker::edges(const boost::shared_ptr<v3d::brep::BRep>& mesh) {
    const std::size_t faces = mesh->faceCount();
    for (std::size_t face = 0; face < faces; face++) {
        const std::vector<unsigned int> loop = faceLoop(mesh, static_cast<unsigned int>(face));
        for (std::size_t index = 0; index < loop.size(); index++) {
            // a half edge and its pair are one edge to a click, so only the half that
            // draws it is offered
            if (!ownsEdge(mesh, loop[index])) {
                continue;
            }

            glm::vec3 from;
            glm::vec3 to;
            if (!loopSegment(mesh, loop, index, &from, &to)) {
                continue;
            }

            glm::vec2 start;
            glm::vec2 end;
            float startDepth = 0.0f;
            float endDepth = 0.0f;
            if (!screen(from, &start, &startDepth) || !screen(to, &end, &endDepth)) {
                continue;
            }

            const float along = closest(start, end, cursor_);
            if (glm::distance(start + (end - start) * along, cursor_) > tolerance_) {
                continue;
            }
            offer(mesh->id(), loop[index], startDepth + (endDepth - startDepth) * along);
        }
    }
}

/**
 **/
bool Picker::screen(const glm::vec3& point, glm::vec2* position, float* depth) const {
    const glm::vec3 world(model_ * glm::vec4(point, 1.0f));
    const glm::vec3 projected = camera_->project(world, const_cast<int*>(viewport_));

    // outside the depth range is behind the near plane or beyond the far one. A point
    // behind a perspective camera divides by a negative w and lands beyond one, so it
    // is rejected here rather than looking like a hit in front
    if (projected[2] < 0.0f || projected[2] > 1.0f) {
        return false;
    }

    if (position != nullptr) {
        *position = glm::vec2(projected[0], projected[1]);
    }
    if (depth != nullptr) {
        *depth = projected[2];
    }
    return true;
}

/**
 **/
void Picker::offer(unsigned int mesh, unsigned int component, float distance) {
    if (hit_.valid && distance >= nearest_) {
        return;
    }
    nearest_ = distance;
    hit_.valid = true;
    hit_.mesh = mesh;
    hit_.component = component;
}

};  // namespace v3d::editor
