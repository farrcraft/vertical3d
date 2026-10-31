/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Usd.h"

#include <api/interchange/Scene.h>
#include <api/interchange/usd/internal/Reader.h>
#include <pxr/usd/usd/common.h>
#include <pxr/usd/usd/stage.h>

#include <string>

namespace v3d::interchange {

Imported readUsd(const std::string& path) {
    Imported imported;
    const pxr::UsdStageRefPtr stage = pxr::UsdStage::Open(path);
    if (!stage) {
        imported.problem = "could not open " + path + " as USD";
        return imported;
    }
    return usd::internal::Reader(stage).read();
}

};  // namespace v3d::interchange
