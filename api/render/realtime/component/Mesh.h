/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/Handle.h>

namespace v3d::render::realtime::component {

/**
 * What a lit entity looks like: a model registered with a MeshRegistry, and whether it casts
 * a shadow - ADR-0063. Where it stands is its ecs::component::Transform.
 *
 * The material is the registry entry's rather than the entity's, so two entities drawing one
 * model draw it the same way.
 **/
struct Mesh final {
    MeshHandle mesh;
    bool castsShadow = true;
};

};  // namespace v3d::render::realtime::component
