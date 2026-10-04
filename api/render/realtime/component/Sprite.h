/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/Handle.h>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::render::realtime::component {

/**
 * An entity drawn as an upright quad facing the camera, its bottom edge centred on the
 * position of its ecs::component::Transform - ADR-0063.
 *
 * The region is held resolved rather than by name, so a game resolves it again when the sheet
 * it came from is reloaded. The transform's rotation does not turn the quad: a game shows which
 * way a sprite faces by choosing its region.
 **/
struct Sprite final {
    TextureHandle texture;               /**< unset for an untextured quad drawn against white **/
    glm::vec2 uv0{0.0f, 0.0f};           /**< the region's top-left, as SpriteSheets::uv hands it out **/
    glm::vec2 uv1{1.0f, 1.0f};           /**< the region's bottom-right **/
    glm::vec2 size{1.0f, 1.0f};          /**< width by height in world units, before the transform's scale **/
    glm::vec4 tint{1.0f, 1.0f, 1.0f, 1.0f};
};

};  // namespace v3d::render::realtime::component
