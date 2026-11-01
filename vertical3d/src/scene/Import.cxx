/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Import.h"

#include <api/interchange/Rescale.h>
#include <api/interchange/Scene.h>
#include <vertical3d/src/scene/Importer.h>

namespace v3d::editor::scene {

Import importScene(const v3d::interchange::Scene& scene, const Into& into) {
    if (into.metres == 1.0 || !(into.metres > 0.0)) {
        return Importer(scene, into.rate).run();
    }
    v3d::interchange::Scene measured = scene;
    v3d::interchange::rescale(&measured, static_cast<float>(1.0 / into.metres));
    return Importer(measured, into.rate).run();
}

};  // namespace v3d::editor::scene
