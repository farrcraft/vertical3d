/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/lighting/Settings.h>
#include <api/render/realtime/lighting/SceneUniforms.h>

#include <boost/test/unit_test.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

using v3d::render::realtime::lighting::SceneUniforms;

namespace lighting = v3d::render::realtime::lighting;

namespace {

BOOST_AUTO_TEST_SUITE(lit_settings_test)

/**
 * Each setting is written to the member and component that lit.glsl reads it from. Static
 * assertions beside pack() check the offsets. This test checks what they cannot: a value
 * written into the wrong component of the right member.
 **/
BOOST_AUTO_TEST_CASE(pack_puts_each_setting_where_the_shader_reads_it) {
    lighting::Settings settings;
    settings.light = glm::vec3(0.0F, 2.0F, 0.0F);
    settings.fill = 0.4F;
    settings.shadowThreshold = 0.2F;
    settings.litThreshold = 0.8F;
    settings.bands = glm::vec3(0.1F, 0.5F, 0.9F);
    settings.normalBias = 0.03F;
    settings.shadowStrength = 0.75F;
    settings.colour = glm::vec3(1.0F, 0.5F, 0.25F);
    settings.shadowColour = glm::vec3(0.25F, 0.5F, 1.0F);
    const glm::mat4 light = glm::translate(glm::mat4(1.0F), glm::vec3(1.0F, 2.0F, 3.0F));

    const SceneUniforms packed = lighting::pack(settings, light, 1.0F / 2048.0F);

    // normalised, so a light given at any length shades the same
    BOOST_CHECK_CLOSE(packed.light.y, 1.0F, 0.0001F);
    BOOST_CHECK_SMALL(packed.light.x, 0.0001F);
    BOOST_CHECK_CLOSE(packed.light.w, 0.4F, 0.0001F);
    BOOST_CHECK_CLOSE(packed.thresholds.x, 0.2F, 0.0001F);
    BOOST_CHECK_CLOSE(packed.thresholds.y, 0.8F, 0.0001F);
    BOOST_CHECK_CLOSE(packed.bands.x, 0.1F, 0.0001F);
    BOOST_CHECK_CLOSE(packed.bands.z, 0.9F, 0.0001F);
    BOOST_CHECK(packed.lightViewProjection == light);
    BOOST_CHECK_CLOSE(packed.shadow.x, 1.0F / 2048.0F, 0.0001F);
    BOOST_CHECK_CLOSE(packed.shadow.y, 0.75F, 0.0001F);
    BOOST_CHECK_CLOSE(packed.shadow.z, 0.03F, 0.0001F);
    BOOST_CHECK(packed.colour == glm::vec4(1.0F, 0.5F, 0.25F, 0.0F));
    BOOST_CHECK(packed.shadowColour == glm::vec4(0.25F, 0.5F, 1.0F, 0.0F));
}

/**
 * A light left at its default colour is white over every band, so a scene that sets no light
 * colour is lit by plain white.
 **/
BOOST_AUTO_TEST_CASE(the_default_light_is_white) {
    const SceneUniforms packed = lighting::pack(lighting::Settings(), glm::mat4(1.0F), 0.0F);
    BOOST_CHECK(glm::vec3(packed.colour) == glm::vec3(1.0F));
    BOOST_CHECK(glm::vec3(packed.shadowColour) == glm::vec3(1.0F));
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
