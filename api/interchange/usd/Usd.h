/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/interchange/Scene.h>

#include <string>

namespace v3d::interchange {

/**
 * Read a USD stage, .usd, .usda, .usdc or .usdz, as a scene.
 *
 * Every prim that can be placed becomes a node, parents first, and so does a Scope, which
 * groups without placing. A Mesh carries its polygons as the file gives them, a Camera its lens,
 * and a SphereLight or DistantLight its light. A SphereLight shaped to a cone narrower than a
 * half turn is a spot light. Instanced prims are read as copies. Prims that cannot be placed,
 * such as materials and shaders, are left out with everything beneath them.
 *
 * The scene is y up and measured in metres. A stage that is z up, or measured in another unit,
 * is turned and scaled into it at the top of the hierarchy. A prim at the top whose placement
 * does not move takes the turn into its own placement. One that is keyed is put beneath a group
 * named "stage" that holds the turn, so its keys keep their meaning.
 *
 * A prim whose placement is keyed is read at each time the stage samples it, as linear keys of
 * its translation, its rotation and its scale. A part that keeps one value is not keyed. The
 * placement at rest is the one at the stage's start time.
 *
 * A light's power is worked out as Blender writes and reads it. A sphere light gives
 * intensity times two to its exposure times pi watts when it is normalized or has no radius,
 * and that times its surface area otherwise. A distant light's intensity is watts per square
 * metre.
 *
 * Left out, each with a note: a light of any other kind, a mesh whose points are keyed, and a
 * prim that discards its parent's placement, which is placed beneath its parent anyway.
 *
 * @return the scene, or a problem when the stage cannot be opened
 **/
Imported readUsd(const std::string& path);

};  // namespace v3d::interchange
