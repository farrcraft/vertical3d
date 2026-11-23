/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <glm/mat4x4.hpp>

namespace v3d::moya::output {

/** What an EXR file's header says about its picture and how it was made. **/
struct ExrMetadata {
    /**
     * The Color Interop Forum ID of the space the beauty is in, written as colorInteropID. The
     * film holds ACEScg, lin_ap1_scene. An output converted into the working space names the
     * working space's ID, which colour::Config::workingInteropId() gives. Empty writes none.
     **/
    std::string colourSpace { "lin_ap1_scene" };

    /** World to camera space, written as OpenEXR's worldToCamera. **/
    std::optional<glm::mat4> worldToCamera;

    /** World to screen space, written as OpenEXR's worldToNDC. **/
    std::optional<glm::mat4> worldToScreen;

    /** How long the picture took to render, written as moya/renderTime. **/
    std::optional<float> renderSeconds;

    /** Other string attributes, each a name and its value, as a Cryptomatte manifest is. **/
    std::vector<std::pair<std::string, std::string>> strings;
};

};  // namespace v3d::moya::output
