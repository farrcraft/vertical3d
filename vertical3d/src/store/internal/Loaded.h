/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/animation/Curve.h>
#include <vertical3d/src/scene/RenderSettings.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/scene/Timeline.h>
#include <vertical3d/src/scene/Units.h>

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace v3d::editor::store::internal {

/**
 * Every item one document holds, read before any of it reaches the scene.
 **/
struct Loaded {
    struct Animated {
        std::int64_t owner = 0;
        std::string path;
        v3d::animation::Curve curve;
    };

    scene::Items items;
    std::vector<Animated> curves;
    scene::Timeline timeline;
    scene::RenderSettings render;
    scene::Length length = scene::Length::Metre;
    // each child's parent, both by document id
    std::map<std::int64_t, std::int64_t> parents;
    std::int64_t next = 1;
};

};  // namespace v3d::editor::store::internal
