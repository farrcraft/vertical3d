/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/Handle.h>
#include <api/type/animation/SpriteClip.h>

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
    /**
     * Which way a particle's quad lies.
     **/
    enum class Facing {
        Camera,   /**< square to the camera, spanned by its right and up **/
        Velocity  /**< stretched along the particle's velocity as the camera sees it: rain, sparks **/
    };

    TextureHandle texture;               /**< unset for an untextured quad drawn against white **/
    boost::shared_ptr<const type::animation::SpriteClip> clip;  /**< shared, since every particle of every emitter plays it **/
    glm::vec2 uv0{0.0f, 0.0f};           /**< the region drawn when there is no clip **/
    glm::vec2 uv1{1.0f, 1.0f};

    /**
     * Whether the clip runs once over a particle's whole life, however long that is, or plays by
     * the particle's age in seconds - a puff that thins as it dies, or a flame that flickers at
     * its own rate.
     **/
    bool overLife{ false };

    Facing facing{ Facing::Camera };

    /**
     * For a particle stretched along its velocity, how many seconds of its travel the quad
     * spans. The quad is never shorter than it is wide.
     **/
    float stretch{ 0.05f };
};

};  // namespace v3d::render::realtime::component
