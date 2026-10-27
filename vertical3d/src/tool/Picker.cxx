/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Picker.h"

#include <api/brep/BRep.h>
#include <api/brep/Index.h>
#include <api/brep/Topology.h>
#include <api/brep/Vertex.h>
#include <api/type/camera/Camera.h>
#include <api/type/geometry/AABBox.h>
#include <api/render/realtime/canvas/Line.h>
#include <vertical3d/src/scene/Camera.h>
#include <vertical3d/src/scene/Evaluation.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Group.h>
#include <vertical3d/src/scene/Light.h>
#include <vertical3d/src/scene/Mesh.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/selection/SelectMask.h>
#include <vertical3d/src/view/Glyphs.h>
#include <vertical3d/src/view/ViewPort.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor {

namespace {

/**
 * Where along a segment the closest point to a target is, as a fraction of it,
 * clamped to the segment's ends. A degenerate segment returns zero, its start.
 **/
float closest(const glm::vec2& from, const glm::vec2& to, const glm::vec2& target) {
    const glm::vec2 along = to - from;
    const float length = glm::dot(along, along);
    if (length <= 0.0F) {
        return 0.0F;
    }
    const float u = glm::dot(target - from, along) / length;
    if (u < 0.0F) {
        return 0.0F;
    }
    if (u > 1.0F) {
        return 1.0F;
    }
    return u;
}

};  // namespace

/**
 **/
Picker::Hit::Hit() :
valid(false),
item(0),
kind(SelectMask::Object),
component(0) {
}

/**
 **/
Picker::Picker(float tolerance) :
    tolerance_(tolerance),
    mask_(SelectMask::Object),
    camera_(nullptr),
    cursor_(0.0F, 0.0F),
    model_(1.0F),
    current_(0),
    nearest_(0.0F),
    band_(false),
    min_(0.0F, 0.0F),
    max_(0.0F, 0.0F) {
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
    cursor_ = cursor;
    nearest_ = 0.0F;
    band_ = false;

    if (!begin(view, mask)) {
        return hit_;
    }
    ray_ = camera_->ray(cursor, viewport_);

    scene.accept(this);

    camera_ = nullptr;
    return hit_;
}

/**
 **/
std::vector<Picker::Hit> Picker::inside(const Scene& scene, const ViewPort& view, const glm::vec2& corner,
    const glm::vec2& opposite, SelectMask mask) {
    inside_.clear();
    min_ = glm::min(corner, opposite);
    max_ = glm::max(corner, opposite);
    band_ = true;

    if (begin(view, mask)) {
        scene.accept(this);
    }

    camera_ = nullptr;
    band_ = false;
    std::vector<Hit> found;
    found.swap(inside_);
    return found;
}

/**
 **/
bool Picker::begin(const ViewPort& view, SelectMask mask) {
    mask_ = mask;
    camera_ = nullptr;

    const boost::shared_ptr<v3d::type::camera::Camera> camera = view.camera();
    const glm::vec4& region = view.region();
    if (!camera || region.z <= 0.0F || region.w <= 0.0F) {
        return false;
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
    return true;
}

/**
 **/
void Picker::visit(const boost::shared_ptr<Mesh>& item, const Placed& placed) {
    if (!item || camera_ == nullptr) {
        return;
    }

    if (selectsItems(mask_) ? !takes(mask_, Item::Kind::Mesh) : !item->selected()) {
        // an object has to be selected before any of its components may be, so a component
        // click stays inside the mesh the user is working on
        return;
    }

    model_ = placed.world;
    current_ = item->id();
    const boost::shared_ptr<v3d::brep::BRep>& mesh = item->geometry();

    if (band_) {
        enclosed(mesh);
        return;
    }

    switch (mask_) {
    case SelectMask::Vertex:
        vertices(mesh);
        break;
    case SelectMask::Edge:
        edges(mesh);
        break;
    case SelectMask::Object:
    case SelectMask::Mesh:
    case SelectMask::Light:
    case SelectMask::Camera:
    case SelectMask::Face:
        surface(mesh);
        break;
    }
}

/**
 **/
void Picker::visit(const boost::shared_ptr<Camera>& camera, const Placed& placed, const Camera::Lens& lens) {
    if (!camera || camera_ == nullptr || !takes(mask_, Item::Kind::Camera)) {
        return;
    }
    v3d::render::realtime::canvas::Line drawn;
    drawCamera(lens, glm::vec4(1.0F), &drawn);
    glyph(*camera, placed.world, drawn);
}

/**
 **/
void Picker::visit(const boost::shared_ptr<Light>& light, const Placed& placed, const Light::Emission& emission) {
    if (!light || camera_ == nullptr || !takes(mask_, Item::Kind::Light)) {
        return;
    }
    v3d::render::realtime::canvas::Line drawn;
    drawLight(emission, glm::vec4(1.0F), &drawn);
    glyph(*light, placed.world, drawn);
}

/**
 **/
void Picker::visit(const boost::shared_ptr<Group>& group, const Placed& placed) {
    if (!group || camera_ == nullptr || !takes(mask_, Item::Kind::Group)) {
        return;
    }
    v3d::render::realtime::canvas::Line drawn;
    drawGroup(glm::vec4(1.0F), &drawn);
    glyph(*group, placed.world, drawn);
}

/**
 **/
void Picker::glyph(const Item& item, const glm::mat4& world, const v3d::render::realtime::canvas::Line& drawn) {
    model_ = glyphPlacement(world);
    current_ = item.id();
    const std::vector<v3d::render::realtime::canvas::Line::Vertex>& points = drawn.vertices();

    if (band_) {
        const bool whole = !points.empty() && std::all_of(points.begin(), points.end(),
            [this](const v3d::render::realtime::canvas::Line::Vertex& point) { return encloses(point.position); });
        if (whole) {
            Hit hit;
            hit.valid = true;
            hit.item = current_;
            hit.kind = mask_;
            inside_.push_back(hit);
        }
        return;
    }

    for (std::size_t index = 0; index + 1 < points.size(); index += 2) {
        glm::vec2 start;
        glm::vec2 end;
        float startDepth = 0.0F;
        float endDepth = 0.0F;
        if (!screen(points[index].position, &start, &startDepth) || !screen(points[index + 1].position, &end, &endDepth)) {
            continue;
        }
        const float along = closest(start, end, cursor_);
        if (glm::distance(start + (end - start) * along, cursor_) <= tolerance_) {
            offer({.item = current_, .component = 0, .distance = startDepth + ((endDepth - startDepth) * along)});
        }
    }
}

/**
 **/
void Picker::surface(const boost::shared_ptr<v3d::brep::BRep>& mesh) {
    // transformed() does not renormalise, so this distance is the same number it would
    // be in world space and hits on differently scaled meshes stay comparable
    const v3d::type::geometry::Ray local = ray_.transformed(glm::inverse(model_));

    float distance = 0.0F;
    if (!local.intersects(mesh->bound(), &distance)) {
        return;
    }

    const std::size_t faces = mesh->faceCount();
    for (v3d::brep::Index face = 0; face < faces; face++) {
        const std::vector<v3d::brep::Index> loop = v3d::brep::faceLoop(*mesh, face);
        if (loop.size() < 3) {
            continue;
        }

        // a fan from the loop's first vertex is correct only for a planar convex face.
        // Every primitive the editor creates has only such faces
        const std::optional<v3d::brep::Segment> opening = v3d::brep::loopSegment(*mesh, loop, 0);
        if (!opening) {
            continue;
        }
        const glm::vec3& first = opening->from;
        for (std::size_t index = 1; index + 1 < loop.size(); index++) {
            const std::optional<v3d::brep::Segment> edge = v3d::brep::loopSegment(*mesh, loop, index);
            if (!edge) {
                continue;
            }
            if (local.intersects(first, edge->from, edge->to, &distance)) {
                offer({.item = current_, .component = face, .distance = distance});
            }
        }
    }
}

/**
 **/
void Picker::vertices(const boost::shared_ptr<v3d::brep::BRep>& mesh) {
    const std::size_t count = mesh->vertexCount();
    for (v3d::brep::Index index = 0; index < count; index++) {
        const v3d::brep::Vertex* vertex = mesh->vertex(index);
        if (vertex == nullptr) {
            continue;
        }
        glm::vec2 position;
        float depth = 0.0F;
        if (!screen(vertex->point(), &position, &depth)) {
            continue;
        }
        if (glm::distance(position, cursor_) > tolerance_) {
            continue;
        }
        offer({.item = current_, .component = index, .distance = depth});
    }
}

/**
 **/
void Picker::edges(const boost::shared_ptr<v3d::brep::BRep>& mesh) {
    const std::size_t faces = mesh->faceCount();
    for (v3d::brep::Index face = 0; face < faces; face++) {
        const std::vector<v3d::brep::Index> loop = v3d::brep::faceLoop(*mesh, face);
        for (std::size_t index = 0; index < loop.size(); index++) {
            // a half edge and its pair are one edge to a click, so only the half that
            // draws it is offered
            if (!v3d::brep::ownsEdge(*mesh, loop[index])) {
                continue;
            }

            const std::optional<v3d::brep::Segment> segment = v3d::brep::loopSegment(*mesh, loop, index);
            if (!segment) {
                continue;
            }
            const glm::vec3& from = segment->from;
            const glm::vec3& to = segment->to;

            glm::vec2 start;
            glm::vec2 end;
            float startDepth = 0.0F;
            float endDepth = 0.0F;
            if (!screen(from, &start, &startDepth) || !screen(to, &end, &endDepth)) {
                continue;
            }

            const float along = closest(start, end, cursor_);
            if (glm::distance(start + (end - start) * along, cursor_) > tolerance_) {
                continue;
            }
            offer({.item = current_, .component = loop[index], .distance = startDepth + ((endDepth - startDepth) * along)});
        }
    }
}

/**
 **/
void Picker::enclosed(const boost::shared_ptr<v3d::brep::BRep>& mesh) {
    switch (mask_) {
    case SelectMask::Object:
    case SelectMask::Mesh:
    case SelectMask::Light:
    case SelectMask::Camera:
        enclosedObject(mesh);
        break;
    case SelectMask::Vertex:
        enclosedVertices(mesh);
        break;
    case SelectMask::Edge:
    case SelectMask::Face:
        enclosedLoops(mesh);
        break;
    }
}

/**
 **/
void Picker::enclosedObject(const boost::shared_ptr<v3d::brep::BRep>& mesh) {
    // an empty mesh has a bound at the origin and nothing to see there
    if (mesh->vertexCount() == 0) {
        return;
    }
    const v3d::type::geometry::AABBox bound = mesh->bound();
    const glm::vec3 low = bound.min();
    const glm::vec3 high = bound.max();
    const std::array<glm::vec3, 8> corners = {
        glm::vec3(low.x, low.y, low.z), glm::vec3(high.x, low.y, low.z),
        glm::vec3(low.x, high.y, low.z), glm::vec3(high.x, high.y, low.z),
        glm::vec3(low.x, low.y, high.z), glm::vec3(high.x, low.y, high.z),
        glm::vec3(low.x, high.y, high.z), glm::vec3(high.x, high.y, high.z) };
    for (const glm::vec3& corner : corners) {
        if (!encloses(corner)) {
            return;
        }
    }
    Hit hit;
    hit.valid = true;
    hit.item = current_;
    hit.kind = mask_;
    inside_.push_back(hit);
}

/**
 **/
void Picker::enclosedVertices(const boost::shared_ptr<v3d::brep::BRep>& mesh) {
    Hit hit;
    hit.valid = true;
    hit.item = current_;
    hit.kind = SelectMask::Vertex;
    const std::size_t count = mesh->vertexCount();
    for (v3d::brep::Index index = 0; index < count; index++) {
        const v3d::brep::Vertex* vertex = mesh->vertex(index);
        if (vertex != nullptr && encloses(vertex->point())) {
            hit.component = index;
            inside_.push_back(hit);
        }
    }
}

/**
 **/
void Picker::enclosedLoops(const boost::shared_ptr<v3d::brep::BRep>& mesh) {
    Hit hit;
    hit.valid = true;
    hit.item = current_;
    hit.kind = mask_;
    const std::size_t faces = mesh->faceCount();
    for (v3d::brep::Index face = 0; face < faces; face++) {
        const std::vector<v3d::brep::Index> loop = v3d::brep::faceLoop(*mesh, face);
        bool whole = !loop.empty();
        for (std::size_t index = 0; index < loop.size(); index++) {
            const std::optional<v3d::brep::Segment> segment = v3d::brep::loopSegment(*mesh, loop, index);
            const bool held = segment && encloses(segment->from) && encloses(segment->to);
            whole = whole && held;
            // a half edge and its pair are one edge, so only the half that draws it is
            // offered, as a click offers it
            if (mask_ == SelectMask::Edge && held && v3d::brep::ownsEdge(*mesh, loop[index])) {
                hit.component = loop[index];
                inside_.push_back(hit);
            }
        }
        if (mask_ == SelectMask::Face && whole) {
            hit.component = face;
            inside_.push_back(hit);
        }
    }
}

/**
 **/
bool Picker::encloses(const glm::vec3& point) const {
    glm::vec2 position;
    if (!screen(point, &position, nullptr)) {
        return false;
    }
    return position.x >= min_.x && position.x <= max_.x && position.y >= min_.y && position.y <= max_.y;
}

/**
 **/
bool Picker::screen(const glm::vec3& point, glm::vec2* position, float* depth) const {
    const glm::vec3 world(model_ * glm::vec4(point, 1.0F));
    const glm::vec3 projected = camera_->project(world, viewport_);

    // outside the depth range is behind the near plane or beyond the far one. A point
    // behind a perspective camera divides by a negative w and lands beyond one, so it
    // is rejected here rather than looking like a hit in front
    if (projected[2] < 0.0F || projected[2] > 1.0F) {
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
void Picker::offer(const Candidate& candidate) {
    if (hit_.valid && candidate.distance >= nearest_) {
        return;
    }
    nearest_ = candidate.distance;
    hit_.valid = true;
    hit_.item = candidate.item;
    hit_.component = candidate.component;
}

};  // namespace v3d::editor
