/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Import.h"

#include <api/interchange/Scene.h>
#include <vertical3d/src/scene/Importer.h>

namespace v3d::editor::scene {

Import importScene(const v3d::interchange::Scene& scene, float rate) {
    return Importer(scene, rate).run();
}

};  // namespace v3d::editor::scene
