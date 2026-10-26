/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/event/Router.h>
#include <api/log/Logger.h>
#include <api/ui/Container.h>
#include <api/ui/Interface.h>
#include <api/ui/Image.h>
#include <api/ui/component/Icon.h>
#include <api/ui/component/Label.h>
#include <api/ui/component/SelectList.h>
#include <api/ui/component/TextBox.h>
#include <vertical3d/src/render/RenderPanels.h>
#include <vertical3d/src/scene/RenderSettings.h>

#include <fstream>
#include <iterator>
#include <optional>
#include <string>

#include <boost/json/parse.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>
#include <entt/entt.hpp>
#include <glm/vec2.hpp>

namespace {

/**
 * The editor's own ui, which the suite's data directory holds.
 **/
boost::shared_ptr<v3d::ui::Interface> editorUi() {
    std::ifstream file("data/vgui.json");
    const std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    const boost::shared_ptr<entt::dispatcher> dispatcher = boost::make_shared<entt::dispatcher>();
    const boost::shared_ptr<v3d::ui::Interface> ui = boost::make_shared<v3d::ui::Interface>(
        boost::make_shared<v3d::event::Router>(dispatcher), dispatcher, boost::make_shared<v3d::log::Logger>());
    BOOST_REQUIRE(ui->load(boost::json::parse(text).as_object()));
    return ui;
}

/**
 * One component of a container, as its type.
 **/
template <typename T>
boost::shared_ptr<T> part(const boost::shared_ptr<v3d::ui::Interface>& ui, const char* container, const char* name) {
    const boost::shared_ptr<T> found = boost::dynamic_pointer_cast<T>(ui->container(container)->get(name));
    BOOST_REQUIRE(found);
    return found;
}

BOOST_AUTO_TEST_CASE(render_panels_view_test) {
    const boost::shared_ptr<v3d::ui::Interface> ui = editorUi();
    v3d::editor::RenderPanels panels(ui);

    // the window starts hidden, and says what the render is doing
    BOOST_CHECK(!panels.viewVisible());
    panels.showView();
    BOOST_CHECK(panels.viewVisible());
    panels.status("Rendering - 40%");
    BOOST_CHECK_EQUAL(part<v3d::ui::component::Label>(ui, "render-view", "renderStatus")->text(), "Rendering - 40%");

    // a picture keeps its shape within the room the config gives it
    const boost::shared_ptr<v3d::ui::component::Icon> icon = part<v3d::ui::component::Icon>(ui, "render-view", "renderPicture");
    const glm::vec2 room(icon->layout().width.value(), icon->layout().height.value());
    panels.picture(v3d::ui::Image(), glm::uvec2(1000, 250));
    BOOST_CHECK_CLOSE(icon->layout().width.value(), room.x, 0.001F);
    BOOST_CHECK_CLOSE(icon->layout().height.value(), room.x / 4.0F, 0.001F);
    panels.picture(v3d::ui::Image(), glm::uvec2(100, 400));
    BOOST_CHECK_CLOSE(icon->layout().height.value(), room.y, 0.001F);
    BOOST_CHECK_CLOSE(icon->layout().width.value(), room.y / 4.0F, 0.001F);

    panels.hideView();
    BOOST_CHECK(!panels.viewVisible());
}

BOOST_AUTO_TEST_CASE(render_panels_settings_test) {
    const boost::shared_ptr<v3d::ui::Interface> ui = editorUi();
    v3d::editor::RenderPanels panels(ui);
    v3d::editor::RenderSettings given;
    given.size = glm::uvec2(800, 600);
    given.samples = 3;
    given.hider = v3d::editor::RenderSettings::Hider::RayTrace;

    // the fields are filled from the settings, and read back as the same settings
    panels.openSettings(given);
    BOOST_CHECK(panels.settingsVisible());
    BOOST_CHECK_EQUAL(part<v3d::ui::component::TextBox>(ui, "render-settings", "settingsWidth")->text(), "800");
    BOOST_CHECK_EQUAL(part<v3d::ui::component::SelectList>(ui, "render-settings", "settingsHider")->selected(), 1);
    const std::optional<v3d::editor::RenderSettings> read = panels.settings();
    BOOST_REQUIRE(read);
    BOOST_CHECK(*read == given);

    // a field that is not a whole number, or settings that are not valid, read as nothing
    part<v3d::ui::component::TextBox>(ui, "render-settings", "settingsHeight")->text("600px");
    BOOST_CHECK(!panels.settings());
    part<v3d::ui::component::TextBox>(ui, "render-settings", "settingsHeight")->text("600");
    part<v3d::ui::component::TextBox>(ui, "render-settings", "settingsSamples")->text("0");
    BOOST_CHECK(!panels.settings());

    panels.closeSettings();
    BOOST_CHECK(!panels.settingsVisible());

    // with no ui there is nothing to show or read
    v3d::editor::RenderPanels none((boost::shared_ptr<v3d::ui::Interface>()));
    none.showView();
    BOOST_CHECK(!none.viewVisible());
    BOOST_CHECK(!none.settings());
}

};  // namespace
