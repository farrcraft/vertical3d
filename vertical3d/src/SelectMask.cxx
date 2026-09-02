/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "SelectMask.h"

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
        case SelectMask::Object:
        default:
            return "object";
        }
    }

};  // namespace v3d::editor
