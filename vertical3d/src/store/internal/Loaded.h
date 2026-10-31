/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/animation/Curve.h>
#include <vertical3d/src/scene/RenderSettings.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/scene/Timeline.h>

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace v3d::editor {

/**
 * Every item one document holds, read before any of it reaches the scene.
 **/
struct Loaded {
    struct Animated {
        std::int64_t owner = 0;
        std::string path;
        v3d::animation::Curve curve;
    };

    Items items;
    std::vector<Animated> curves;
    Timeline timeline;
    RenderSettings render;
    // each child's parent, both by document id
    std::map<std::int64_t, std::int64_t> parents;
    std::int64_t next = 1;
};

};  // namespace v3d::editor
