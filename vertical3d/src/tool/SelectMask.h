/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

namespace v3d::editor {

    /**
     * What a click selects.
     *
     * Exactly one is in force at a time. Selection state lives on the mesh and on each of
     * its vertices, half edges and faces, and the mask is the only thing stopping two kinds
     * being selected at once.
     **/
    enum class SelectMask {
        Object,
        Vertex,
        Edge,
        Face
    };

    /**
     * @param name one of "object", "vertex", "edge" or "face"
     * @param mask where the answer goes - untouched if the name is not one of them
     * @return whether the name named a mask
     **/
    bool selectMask(const std::string& name, SelectMask* mask);

    /**
     * @return the name selectMask() accepts for this mask
     **/
    const char* selectMaskName(SelectMask mask);

};  // namespace v3d::editor
