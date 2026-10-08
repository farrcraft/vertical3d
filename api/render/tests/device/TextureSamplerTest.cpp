/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/Textures.h>
#include <api/render/realtime/vulkan/pipeline/Resources.h>
#include <api/render/realtime/vulkan/pipeline/Sampler.h>
#include <api/render/realtime/vulkan/pipeline/Texture.h>

#include <boost/test/unit_test.hpp>

#include "Headless.h"

using v3d::render::realtime::TextureHandle;
using v3d::render::realtime::vulkan::pipeline::Sampler;
using v3d::render::realtime::vulkan::pipeline::Texture;

namespace {

/**
 * @return the sampler a registered texture is read through
 **/
const Sampler* samplerOf(const v3d::test::Headless& headless, const TextureHandle& handle) {
    const Texture* texture = headless.context->resources()->texture(handle);
    BOOST_REQUIRE(texture != nullptr);
    return texture->sampler.get();
}

};  // namespace

BOOST_AUTO_TEST_SUITE(texture_sampler_test)

/**
 * A texture created with the default spec is read through the default sampler, and two created
 * with the same repeating spec share one sampler of their own. Both sides are checked: a factory
 * that ignored the spec would hand all three the default, and one that made a sampler per
 * texture would give the two repeating ones different samplers.
 **/
BOOST_AUTO_TEST_CASE(textures_share_a_sampler_per_spec) {
    v3d::test::Headless headless(VK_FORMAT_R8G8B8A8_UNORM, 4, 4);
    const unsigned char texel[4] = {0xFF, 0xFF, 0xFF, 0xFF};

    Sampler::Spec repeating;
    repeating.address = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    const TextureHandle plain = headless.context->textures()->texture(texel, 1, 1, 4);
    const TextureHandle first = headless.context->textures()->texture(texel, 1, 1, 4, repeating);
    const TextureHandle second = headless.context->textures()->texture(texel, 1, 1, 4, repeating);

    BOOST_TEST(samplerOf(headless, plain) == samplerOf(headless, headless.context->textures()->white()));
    BOOST_TEST(samplerOf(headless, plain) != samplerOf(headless, first));
    BOOST_TEST(samplerOf(headless, first) == samplerOf(headless, second));
    BOOST_TEST(headless.silent());
}

BOOST_AUTO_TEST_SUITE_END()
