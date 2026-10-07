/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#include "Movement.h"

#include <api/grid/TileCoord.h>
#include <odyssey/engine/Path.h>

namespace odyssey::system {

namespace {

/**
 * How long an entity spends crossing one tile, in seconds. On the grid a diagonal step
 * costs the same as an orthogonal one, so it takes the same time. A walk therefore keeps a
 * steady pace rather than slowing down around corners.
 **/
constexpr float secondsPerTile = 0.15f;

};  // namespace

/**
 **/
bool Movement::simulate(float step) {
    auto view = registry_->view<v3d::grid::TileCoord, odyssey::engine::Path>();
    for (auto entity : view) {
        odyssey::engine::Path& path = view.get<odyssey::engine::Path>(entity);
        if (path.next >= path.tiles.size()) {
            continue;
        }
        path.elapsed += step;
        // a while rather than an if: a step longer than secondsPerTile covers more than one
        // tile, and dropping the remainder would make a slow frame a slow walk
        while (path.elapsed >= secondsPerTile && path.next < path.tiles.size()) {
            path.elapsed -= secondsPerTile;
            const v3d::grid::TileCoord tile = path.tiles[path.next];
            registry_->replace<v3d::grid::TileCoord>(entity, tile.x, tile.y);
            path.next++;
        }
        if (path.next >= path.tiles.size()) {
            path.tiles.clear();
            path.next = 0;
            path.elapsed = 0.0f;
        }
    }
    return true;
}

};  // namespace odyssey::system
