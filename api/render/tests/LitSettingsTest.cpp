/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/LitSettings.h>

#include <boost/test/unit_test.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

using v3d::render::realtime::LitSettings;
using v3d::render::realtime::SceneUniforms;

BOOST_AUTO_TEST_SUITE(lit_settings_test)

/**
 * Each setting lands in the member and the component lit.glsl reads it from. The offsets are
 * static assertions beside pack(); this is the half they cannot see, which is a value written
 * into the wrong component of the right member.
 **/
BOOST_AUTO_TEST_CASE(pack_puts_each_setting_where_the_shader_reads_it) {
    LitSettings settings;
    settings.light = glm::vec3(0.0f, 2.0f, 0.0f);
    settings.fill = 0.4f;
    settings.shadowThreshold = 0.2f;
    settings.litThreshold = 0.8f;
    settings.bands = glm::vec3(0.1f, 0.5f, 0.9f);
    settings.normalBias = 0.03f;
    settings.shadowStrength = 0.75f;
    const glm::mat4 light = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 2.0f, 3.0f));

    const SceneUniforms packed = v3d::render::realtime::pack(settings, light, 1.0f / 2048.0f);

    // normalised, so a light given at any length shades the same
    BOOST_CHECK_CLOSE(packed.light.y, 1.0f, 0.0001f);
    BOOST_CHECK_SMALL(packed.light.x, 0.0001f);
    BOOST_CHECK_CLOSE(packed.light.w, 0.4f, 0.0001f);
    BOOST_CHECK_CLOSE(packed.thresholds.x, 0.2f, 0.0001f);
    BOOST_CHECK_CLOSE(packed.thresholds.y, 0.8f, 0.0001f);
    BOOST_CHECK_CLOSE(packed.bands.x, 0.1f, 0.0001f);
    BOOST_CHECK_CLOSE(packed.bands.z, 0.9f, 0.0001f);
    BOOST_CHECK(packed.lightViewProjection == light);
    BOOST_CHECK_CLOSE(packed.shadow.x, 1.0f / 2048.0f, 0.0001f);
    BOOST_CHECK_CLOSE(packed.shadow.y, 0.75f, 0.0001f);
    BOOST_CHECK_CLOSE(packed.shadow.z, 0.03f, 0.0001f);
}

BOOST_AUTO_TEST_SUITE_END()
