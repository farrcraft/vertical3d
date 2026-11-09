/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

namespace v3d::render::realtime::resource {
class MeshRegistry;
};  // namespace v3d::render::realtime::resource

namespace v3d::render::realtime::lighting {

/**
 * How fitShadow() reads the casters. The margin is added to the radius around them. alpha is
 * engine::Engine::alpha(), the fraction of a step elapsed since the last one. At one, the
 * current step's positions are read.
 **/
struct ShadowFit final {
    float margin = 0.0F;
    float alpha = 1.0F;
    /**
     * Where the casters' meshes resolve, so that the fit covers each caster's whole model rather
     * than its origin. Null fits the origins alone
     **/
    const resource::MeshRegistry* meshes = nullptr;
};

};  // namespace v3d::render::realtime::lighting
