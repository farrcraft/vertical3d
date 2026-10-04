/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::render::realtime {

/**
 * How a lit scene looks, as opposed to what is in it: the light, the cel bands, the outline
 * and the shadow's terms. Plain data with no device in it, so a game reading its look from a
 * file or a command line links nothing to do so.
 *
 * The defaults are retcon's, the look the tier was built against.
 **/
struct LitSettings final {
    /**
     * Towards the key light, in world space - it need not be normalised. Keep it well off the
     * camera's axis, or every surface the camera sees lands in the top band and reads flat.
     **/
    glm::vec3 light{-0.45f, 0.62f, 0.64f};

    /**
     * How strongly a fill from above and opposite the key is added before banding. Without it
     * every face turned from the key is one flat shape.
     **/
    float fill = 0.35f;

    /**
     * Where the light, key plus fill, crosses from the shadow band into the mid band, and from
     * the mid into the lit. A surface whose light sits close to one of these crosses it all at
     * once when the light moves a little, so keep the ground well clear of both.
     **/
    float shadowThreshold = 0.26f;
    float litThreshold = 0.70f;

    /**
     * What the albedo is multiplied by in the shadow, mid and lit bands. A shadowed fragment
     * drops one band, so the mid multiplier is also how dark a cast shadow on lit ground is.
     **/
    glm::vec3 bands{0.30f, 0.55f, 1.0f};

    /**
     * The light's colour, which the mid and lit bands are multiplied by, and the shadow band's:
     * an amber light with blue shadows at dusk, a dim blue one at night. White leaves every
     * band as it is.
     **/
    glm::vec3 colour{1.0f};
    glm::vec3 shadowColour{1.0f};

    /**
     * How far the outline hull is pushed out along each normal, in the model's units. Zero
     * draws no outline.
     **/
    float outline = 0.022f;

    /**
     * Depth bias for the shadow pass: a constant in the depth format's smallest steps, and
     * one scaled by the caster's slope. Too little and a surface shadows itself, too much and
     * a contact shadow comes away from what casts it.
     **/
    float constantBias = 1.5f;
    float slopeBias = 2.75f;

    /**
     * How far a receiver's lookup is moved along its normal before the shadow map is read,
     * which removes the acne bias alone leaves on curved surfaces.
     **/
    float normalBias = 0.02f;

    /**
     * How much of a band a shadow takes away. Zero still draws the shadow pass and ignores it,
     * which is the comparison with and without.
     **/
    float shadowStrength = 1.0f;
};

/**
 * Set 2, binding 0, laid out as shaders/lit/lit.glsl declares the Scene block. std140 puts
 * every member here at its natural offset, so the struct maps straight across.
 **/
struct SceneUniforms final {
    glm::vec4 light;                /**< xyz towards the key light, normalised; w the fill **/
    glm::vec4 thresholds;           /**< x shadow to mid, y mid to lit **/
    glm::vec4 bands;                /**< xyz the shadow, mid and lit multipliers **/
    glm::mat4 lightViewProjection;  /**< what the shadow map was drawn through **/
    glm::vec4 shadow;               /**< x a shadow map texel in uv, y strength, z normal bias **/
    glm::vec4 colour;               /**< xyz the light's colour, over the mid and lit bands **/
    glm::vec4 shadowColour;         /**< xyz the shadow band's colour **/
};

/**
 * The scene uniform for one frame.
 *
 * @param lightViewProjection the matrix the shadow pass drew through, or the identity with a
 *        strength of zero when there is no shadow
 * @param texel the size of one shadow map texel in uv, which the shadow lookup steps by
 **/
SceneUniforms pack(const LitSettings& settings, const glm::mat4& lightViewProjection, float texel);

};  // namespace v3d::render::realtime
