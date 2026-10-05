/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Emitter.h"
#include "State.h"

#include <glm/vec3.hpp>

namespace v3d::type::effect {

/**
 * Rain or snow over a region of the world: how dense it is now, how dense it is heading for,
 * and the wind.
 *
 * The particles are an Emitter's, launched as it launches them, but born over the top of a
 * region the caller moves with the view rather than within the emitter's shape. The game
 * decides what the weather is and when it changes, by setting target and wind.
 *
 * +y is up.
 **/
struct Weather final {
    float density{ 0.0f };    /**< particles a second per square unit of ground, at full intensity **/
    float intensity{ 0.0f };  /**< how much of the density is falling, from 0 to 1 **/
    float target{ 0.0f };     /**< the intensity it is easing towards **/
    float ease{ 0.25f };      /**< how far the intensity moves towards its target a second **/
    glm::vec3 wind{ 0.0f };   /**< an acceleration added to the emitter's **/
};

/**
 * Step weather over a region: ease its intensity, move what is falling, and spawn the
 * particles the density makes due over the region's top face.
 *
 * A particle that falls below the region is removed. One that leaves it across a side comes in
 * at the opposite side, its previous position moved with it, so the region keeps its density as
 * it moves and nothing is drawn sweeping across it.
 *
 * @param minimum the region's lowest corner, which is the ground
 * @param maximum its highest, whose height is where the weather is born
 **/
void fall(const Emitter& emitter, Weather* weather, State* state, const glm::vec3& minimum,
    const glm::vec3& maximum, float seconds);

};  // namespace v3d::type::effect
