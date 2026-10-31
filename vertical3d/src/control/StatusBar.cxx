/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "StatusBar.h"

#include <api/brep/BRep.h>
#include <api/brep/HalfEdge.h>
#include <api/brep/Index.h>
#include <api/brep/Topology.h>
#include <vertical3d/src/scene/Components.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Mesh.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/selection/SelectMask.h>
#include <vertical3d/src/tool/CameraControl.h>
#include <vertical3d/src/tool/Select.h>
#include <vertical3d/src/tool/Transform.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <boost/pointer_cast.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::editor::control {

/**
 **/
std::vector<Status::Button> StatusBar::buttons(const tool::CameraControl& camera, const tool::Transform& transform, const tool::Select& select,
    bool perspective) {
    std::vector<Status::Button> made;
    if (const std::optional<std::string> driven = cameraMove(camera.mode())) {
        made.push_back({ .name = "Left", .action = *driven });
    } else {
        std::string action = select.adding() ? "Add or remove" : "Select";
        if (const std::optional<std::string> verb = handles(transform.mode())) {
            action += ", or drag a handle to " + *verb;
        }
        made.push_back({ .name = "Left", .action = action });
    }
    if (perspective) {
        made.push_back({ .name = "Middle", .action = "Orbit" });
    }
    return made;
}

/**
 **/
std::string StatusBar::count(std::size_t number, const std::string& one, const std::string& many) {
    return std::to_string(number) + " " + (number == 1 ? one : many);
}

/**
 **/
std::string StatusBar::selection(const scene::Scene& scene, selection::SelectMask mask) {
    const std::optional<scene::Component> kind = componentOf(mask);
    if (!kind) {
        const std::size_t items = scene.selection().size();
        return items == 0 ? "Nothing selected" : count(items, "item", "items");
    }
    std::size_t number = 0;
    for (const boost::shared_ptr<scene::Item>& item : scene.items()) {
        number += parts(scene, *item, *kind);
    }
    if (number == 0) {
        return "Nothing selected";
    }
    switch (*kind) {
        case scene::Component::Vertex:
            return count(number, "vertex", "vertices");
        case scene::Component::Edge:
            return count(number, "edge", "edges");
        case scene::Component::Face:
            return count(number, "face", "faces");
        case scene::Component::Handle:
            break;
    }
    return count(number, "handle", "handles");
}

/**
 **/
std::string StatusBar::rate(std::uint64_t nanoseconds) {
    if (nanoseconds == 0) {
        return {};
    }
    return std::to_string(static_cast<std::int64_t>(std::lround(SECOND / static_cast<double>(nanoseconds)))) + " fps";
}

/**
 **/
std::optional<std::string> StatusBar::cameraMove(tool::CameraControl::CameraMode mode) {
    switch (mode) {
        case tool::CameraControl::CAMERA_MODE_ZOOM:
            return "Zoom";
        case tool::CameraControl::CAMERA_MODE_TRUCK:
            return "Truck";
        case tool::CameraControl::CAMERA_MODE_PAN:
            return "Orbit";
        case tool::CameraControl::CAMERA_MODE_DOLLY:
            return "Dolly";
        case tool::CameraControl::CAMERA_MODE_PITCH:
            return "Pitch";
        case tool::CameraControl::CAMERA_MODE_NONE:
            break;
    }
    return std::nullopt;
}

/**
 **/
std::optional<std::string> StatusBar::handles(tool::Transform::Mode mode) {
    switch (mode) {
        case tool::Transform::Mode::Translate:
            return "move";
        case tool::Transform::Mode::Rotate:
            return "rotate";
        case tool::Transform::Mode::Scale:
            return "scale";
        case tool::Transform::Mode::None:
            break;
    }
    return std::nullopt;
}

/**
 **/
std::size_t StatusBar::parts(const scene::Scene& scene, const scene::Item& item, scene::Component kind) {
    const std::vector<v3d::brep::Index> chosen = scene.components().of(item.id(), kind);
    const boost::shared_ptr<scene::Mesh> mesh = scene.mesh(item.id());
    if (kind != scene::Component::Edge || !mesh) {
        return chosen.size();
    }
    const v3d::brep::BRep& geometry = *mesh->geometry();
    std::size_t edges = 0;
    for (const v3d::brep::Index half : chosen) {
        // the half that owns its edge stands for it, and the other only when that one is not chosen
        const v3d::brep::HalfEdge* edge = geometry.edge(half);
        const bool alone = edge == nullptr || edge->pair() == v3d::brep::INVALID_ID ||
            !scene.components().selected(item.id(), scene::Component::Edge, edge->pair());
        if (v3d::brep::ownsEdge(geometry, half) || alone) {
            edges++;
        }
    }
    return edges;
}

};  // namespace v3d::editor::control
