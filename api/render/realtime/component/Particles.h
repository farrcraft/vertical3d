/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/Handle.h>
#include <api/animation/SpriteClip.h>

#include <cstdint>

#include <boost/shared_ptr.hpp>

#include <glm/vec2.hpp>

namespace v3d::render::realtime::component {

/**
 * What the particles of an entity's ecs::component::Emitter look like. The emitter owns its
 * particles.
 *
 * A particle is a quad centred on where it stands, sized and coloured by the emitter's tracks.
 * Its region is a frame of the clip, or the uv pair below when there is no clip, held resolved
 * as Sprite holds its own.
 **/
struct Particles final {
    static constexpr float DEFAULT_STRETCH = 0.05F;

    /**
     * Which way a particle's quad lies.
     **/
    enum class Facing : std::uint8_t {
        Camera,   /**< square to the camera, spanned by its right and up **/
        Velocity  /**< stretched along the particle's velocity as the camera sees it: rain, sparks **/
    };

    TextureHandle texture;               /**< unset for an untextured quad drawn against white **/
    boost::shared_ptr<const animation::SpriteClip> clip;  /**< shared, since every particle of every emitter plays it **/
    glm::vec2 uv0{0.0F, 0.0F};           /**< the region drawn when there is no clip **/
    glm::vec2 uv1{1.0F, 1.0F};

    /**
     * Whether the clip runs once over a particle's whole life, however long that is, or plays by
     * the particle's age in seconds. The first suits a puff that thins as it dies, and the
     * second a flame that flickers at its own rate.
     **/
    bool overLife{ false };

    Facing facing{ Facing::Camera };

    /**
     * For a particle stretched along its velocity, how many seconds of its travel the quad
     * spans. The quad is never shorter than it is wide.
     **/
    float stretch{ DEFAULT_STRETCH };
};

};  // namespace v3d::render::realtime::component
