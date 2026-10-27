/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "SelectMask.h"

#include <vertical3d/src/scene/Item.h>

#include <string>

namespace v3d::editor {

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
        return true;
    case SelectMask::Vertex:
    case SelectMask::Edge:
    case SelectMask::Face:
        break;
    }
    return false;
}

/**
 **/
bool takes(SelectMask mask, Item::Kind kind) {
    switch (mask) {
    case SelectMask::Object:
        return true;
    case SelectMask::Mesh:
        return kind == Item::Kind::Mesh;
    case SelectMask::Light:
        return kind == Item::Kind::Light;
    case SelectMask::Camera:
        return kind == Item::Kind::Camera;
    case SelectMask::Vertex:
    case SelectMask::Edge:
    case SelectMask::Face:
        break;
    }
    return false;
}

};  // namespace v3d::editor
