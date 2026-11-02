/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "SelectMask.h"

#include <vertical3d/src/scene/item/Components.h>
#include <vertical3d/src/scene/item/Item.h>

#include <optional>
#include <string>

namespace v3d::editor::selection {

/**
 **/
bool selectMask(const std::string& name, SelectMask* mask) {
    SelectMask found;
    if (name == "object") {
        found = SelectMask::Object;
    } else if (name == "vertex") {
        found = SelectMask::Vertex;
    } else if (name == "edge") {
        found = SelectMask::Edge;
    } else if (name == "face") {
        found = SelectMask::Face;
    } else if (name == "mesh") {
        found = SelectMask::Mesh;
    } else if (name == "light") {
        found = SelectMask::Light;
    } else if (name == "camera") {
        found = SelectMask::Camera;
    } else if (name == "curve") {
        found = SelectMask::Curve;
    } else if (name == "handle") {
        found = SelectMask::Handle;
    } else {
        return false;
    }
    if (mask != nullptr) {
        *mask = found;
    }
    return true;
}

/**
 **/
const char* selectMaskName(SelectMask mask) {
    switch (mask) {
    case SelectMask::Vertex:
        return "vertex";
    case SelectMask::Edge:
        return "edge";
    case SelectMask::Face:
        return "face";
    case SelectMask::Mesh:
        return "mesh";
    case SelectMask::Light:
        return "light";
    case SelectMask::Camera:
        return "camera";
    case SelectMask::Curve:
        return "curve";
    case SelectMask::Handle:
        return "handle";
    case SelectMask::Object:
        break;
    }
    return "object";
}

/**
 **/
bool selectsItems(SelectMask mask) {
    switch (mask) {
    case SelectMask::Object:
    case SelectMask::Mesh:
    case SelectMask::Light:
    case SelectMask::Camera:
    case SelectMask::Curve:
        return true;
    case SelectMask::Vertex:
    case SelectMask::Edge:
    case SelectMask::Face:
    case SelectMask::Handle:
        break;
    }
    return false;
}

/**
 **/
bool takes(SelectMask mask, scene::item::Item::Kind kind) {
    switch (mask) {
    case SelectMask::Object:
        return true;
    case SelectMask::Mesh:
        return kind == scene::item::Item::Kind::Mesh;
    case SelectMask::Light:
        return kind == scene::item::Item::Kind::Light;
    case SelectMask::Camera:
        return kind == scene::item::Item::Kind::Camera;
    case SelectMask::Curve:
        return kind == scene::item::Item::Kind::Curve;
    case SelectMask::Vertex:
    case SelectMask::Edge:
    case SelectMask::Face:
    case SelectMask::Handle:
        break;
    }
    return false;
}

/**
 **/
std::optional<scene::item::Component> componentOf(SelectMask mask) {
    switch (mask) {
        case SelectMask::Vertex:
            return scene::item::Component::Vertex;
        case SelectMask::Edge:
            return scene::item::Component::Edge;
        case SelectMask::Face:
            return scene::item::Component::Face;
        case SelectMask::Handle:
            return scene::item::Component::Handle;
        case SelectMask::Object:
        case SelectMask::Mesh:
        case SelectMask::Camera:
        case SelectMask::Light:
        case SelectMask::Curve:
            break;
    }
    return std::nullopt;
}

};  // namespace v3d::editor::selection
