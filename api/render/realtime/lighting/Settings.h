/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <glm/vec3.hpp>

namespace v3d::render::realtime::lighting {

/**
 * How a lit scene looks, as opposed to what is in it: the light, the cel bands, the outline
 * and the shadow's terms. Plain data with no device in it, so a game reading its look from a
 * file or a command line links nothing to do so.
 *
 * The defaults give three cel bands under a white key light from above and to one side, with
 * a thin outline and full strength shadows.
 **/
struct Settings final {
    static constexpr glm::vec3 DEFAULT_LIGHT{-0.45F, 0.62F, 0.64F};
    static constexpr float DEFAULT_FILL = 0.35F;
    static constexpr float DEFAULT_SHADOW_THRESHOLD = 0.26F;
    static constexpr float DEFAULT_LIT_THRESHOLD = 0.70F;
    static constexpr glm::vec3 DEFAULT_BANDS{0.30F, 0.55F, 1.0F};
    static constexpr float DEFAULT_OUTLINE = 0.022F;
    static constexpr float DEFAULT_CONSTANT_BIAS = 1.5F;
    static constexpr float DEFAULT_SLOPE_BIAS = 2.75F;
    static constexpr float DEFAULT_NORMAL_BIAS = 0.02F;

    /**
     * Towards the key light, in world space - it need not be normalised. Keep it well off the
     * camera's axis, or every surface the camera sees lands in the top band and reads flat.
     **/
    glm::vec3 light{DEFAULT_LIGHT};

    /**
     * How strongly a fill from above and opposite the key is added before banding. Without it
     * every face turned from the key is one flat shape.
     **/
    float fill = DEFAULT_FILL;

    /**
     * Where the light, key plus fill, crosses from the shadow band into the mid band, and from
     * the mid into the lit. A surface whose light sits close to one of these crosses it all at
     * once when the light moves a little, so keep the ground well clear of both.
     **/
    float shadowThreshold = DEFAULT_SHADOW_THRESHOLD;
    float litThreshold = DEFAULT_LIT_THRESHOLD;

    /**
     * What the albedo is multiplied by in the shadow, mid and lit bands. A shadowed fragment
     * drops one band, so the mid multiplier is also how dark a cast shadow on lit ground is.
     **/
    glm::vec3 bands{DEFAULT_BANDS};

    /**
     * The light's colour, which the mid and lit bands are multiplied by, and the shadow band's:
     * an amber light with blue shadows at dusk, a dim blue one at night. White leaves every
     * band as it is.
     **/
    glm::vec3 colour{1.0F};
    glm::vec3 shadowColour{1.0F};

    /**
     * How far the outline hull is pushed out along each normal, in the model's units. Zero
     * draws no outline.
     **/
    float outline = DEFAULT_OUTLINE;

    /**
     * Depth bias for the shadow pass: a constant in the depth format's smallest steps, and
     * one scaled by the caster's slope. Too little and a surface shadows itself, too much and
     * a contact shadow comes away from what casts it.
     **/
    float constantBias = DEFAULT_CONSTANT_BIAS;
    float slopeBias = DEFAULT_SLOPE_BIAS;

    /**
     * How far a receiver's lookup is moved along its normal before the shadow map is read,
     * which removes the acne bias alone leaves on curved surfaces.
     **/
    float normalBias = DEFAULT_NORMAL_BIAS;

    /**
     * How much of a band a shadow takes away. Zero still draws the shadow pass but ignores its
     * result, for comparing the scene with and without shadows.
     **/
    float shadowStrength = 1.0F;
};

};  // namespace v3d::render::realtime::lighting
