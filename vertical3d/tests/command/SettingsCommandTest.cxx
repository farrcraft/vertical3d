/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <vertical3d/src/command/SettingsCommand.h>
#include <vertical3d/src/scene/RenderSettings.h>
#include <vertical3d/src/scene/Scene.h>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

#include <glm/vec2.hpp>

namespace {

BOOST_AUTO_TEST_CASE(settings_command_test) {
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    const v3d::editor::RenderSettings before = scene->renderSettings();
    v3d::editor::RenderSettings after;
    after.size = glm::uvec2(320, 200);
    after.samples = 4;
    after.hider = v3d::editor::RenderSettings::Hider::RayTrace;

    // the settings it is made from are the ones it puts back
    v3d::editor::SettingsCommand command(scene, after);
    command.redo();
    BOOST_CHECK(scene->renderSettings() == after);
    command.undo();
    BOOST_CHECK(scene->renderSettings() == before);
    BOOST_CHECK_EQUAL(command.name(), "render settings");
}

BOOST_AUTO_TEST_CASE(render_settings_valid_test) {
    BOOST_CHECK(v3d::editor::RenderSettings().valid());
    v3d::editor::RenderSettings settings;
    settings.size = glm::uvec2(0, 10);
    BOOST_CHECK(!settings.valid());
    settings.size = glm::uvec2(v3d::editor::RenderSettings::LARGEST_SIDE + 1, 10);
    BOOST_CHECK(!settings.valid());
    settings.size = glm::uvec2(10, 10);
    settings.samples = 0;
    BOOST_CHECK(!settings.valid());
    settings.samples = v3d::editor::RenderSettings::MOST_SAMPLES + 1;
    BOOST_CHECK(!settings.valid());
}

};  // namespace
