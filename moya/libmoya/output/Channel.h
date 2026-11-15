/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/film/Channel.h>

#include <cstdint>
#include <string>

namespace v3d::moya::output {

/** What a channel collects, as UsdRender's RenderVar names its sourceType. **/
enum class Source : std::uint8_t {
    /** A value the renderer computes, named as the renderer names it. **/
    Raw,
    /** A primvar of the geometry, by its name. **/
    Primvar,
    /** The light of the paths a light path expression matches. **/
    Lpe
};

/**
 * A channel a scene's output asks for: the film channel that holds it, and what it collects. A
 * Cryptomatte layer keeps the ranks of IDs its channel names. A denoised channel is written
 * beside a denoised copy of itself.
 **/
struct Channel {
    film::Channel film;
    Source source { Source::Raw };
    std::string sourceName;
    std::uint32_t ranks { 0 };
    bool denoised { false };
};

};  // namespace v3d::moya::output
