/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Emitter.h"
#include "State.h"

#include <glm/vec3.hpp>

namespace v3d::ecs::effect {

/**
 * Rain or snow over a region of the world: how dense it is now, how dense it is heading for,
 * and the wind.
 *
 * An Emitter describes the particles and launches them. They are born over the top face of a
 * region, not within the emitter's shape, and the caller moves the region with the view. The
 * game sets target and wind to decide what the weather is and when it changes.
 *
 * +y is up.
 **/
struct Weather final {
    static constexpr float DEFAULT_EASE = 0.25F;

    float density{ 0.0F };       /**< particles a second per square unit of ground, at full intensity **/
    float intensity{ 0.0F };     /**< how much of the density is falling, from 0 to 1 **/
    float target{ 0.0F };        /**< the intensity it is easing towards **/
    float ease{ DEFAULT_EASE };  /**< how far the intensity moves towards its target a second **/
    glm::vec3 wind{ 0.0F };      /**< an acceleration added to the emitter's **/
};

/**
 * Step weather over a region: ease its intensity, move what is falling, and spawn the
 * particles the density makes due over the region's top face.
 *
 * A particle that falls below the region is removed. A particle that leaves it across a side
 * comes back in at the opposite side, and its previous position moves with it. The region
 * therefore keeps its density as it moves, and no particle is drawn streaking across it.
 *
 * @param minimum the region's lowest corner, which is the ground
 * @param maximum its highest, whose height is where the weather is born
 **/
void fall(const Emitter& emitter, Weather* weather, State* state, const glm::vec3& minimum,
    const glm::vec3& maximum, float seconds);

};  // namespace v3d::ecs::effect
