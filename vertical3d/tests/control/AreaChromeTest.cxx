/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/event/Event.h>
#include <api/event/Router.h>
#include <api/log/Logger.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/type/camera/Profile.h>
#include <api/ui/Component.h>
#include <api/ui/Container.h>
#include <api/ui/Interface.h>
#include <api/ui/component/Button.h>
#include <api/ui/component/Panel.h>
#include <api/ui/component/menu/Bar.h>
#include <api/ui/component/menu/Item.h>
#include <api/ui/component/menu/Menu.h>
#include <api/ui/paint/ComponentRenderer.h>
#include <api/ui/tests/Typeset.h>
#include <vertical3d/src/control/chrome/Area.h>
#include <vertical3d/src/selection/SelectMask.h>
#include <vertical3d/src/view/AreaLayout.h>
#include <vertical3d/src/view/AreaViews.h>
#include <vertical3d/src/view/Shading.h>
#include <vertical3d/src/view/ViewPort.h>

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <boost/json/parse.hpp>
#include <boost/make_shared.hpp>
#include <boost/optional.hpp>
#include <boost/test/unit_test.hpp>
#include <entt/entt.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace {

using v3d::editor::control::chrome::Area;
using v3d::editor::view::AreaLayout;
using v3d::editor::view::AreaViews;
using v3d::editor::view::ViewPort;

/**
 * Two areas side by side over an 800 by 600 window, under a ui whose own container is "editor".
 * The first area runs from 3 to 398 across and the second from 401 to 797, both from 3 to 597
 * down. Headers and tabs are 28 pixels tall. Each viewport's view starts from a profile named
 * for its camera.
 **/
struct Chrome {
    Chrome() :
        dispatcher(boost::make_shared<entt::dispatcher>()),
        ui(boost::make_shared<v3d::ui::Interface>(boost::make_shared<v3d::event::Router>(dispatcher), dispatcher,
            boost::make_shared<v3d::log::Logger>())),
        held(boost::make_shared<AreaLayout>(boost::make_shared<v3d::log::Logger>())),
        layout(*held),
        views(boost::make_shared<AreaViews>(held, [](const AreaLayout::Editor& editor) {
            const std::string camera(editor.state.at("camera").as_string());
            return boost::make_shared<ViewPort>(camera + " " + std::to_string(editor.id), v3d::type::camera::Profile(camera));
        })),
        chrome(ui, "editor", {}),
        renderer(v3d::ui::typeset::body()) {
        BOOST_REQUIRE(ui->load(boost::json::parse(
            R"({ "themes": [], "containers": [ { "name": "editor", "visible": true, "components": [] } ] })").as_object()));
        BOOST_REQUIRE(layout.load(boost::json::parse(R"({"root": {"split": "horizontal", "children": [)"
            R"({"editors": [{"kind": "viewport", "camera": "Front"}]}, {"editors": [{"kind": "viewport", "camera": "Top"}]}]}})").as_object()));
        layout.metrics({ .strip = 28.0F });
        layout.resize(glm::vec4(0.0F, 0.0F, 800.0F, 600.0F));
        BOOST_REQUIRE(views->sync());
        canvas.resize(glm::uvec2(800, 600));
    }

    /**
     * Bring the views up to the layout, and the chrome up to both.
     **/
    void sync() {
        BOOST_REQUIRE(views->sync());
        chrome.sync(*views, mask, 28.0F, active);
    }

    /**
     * @return an area's header, or null for a tabbed area showing a panel
     **/
    boost::shared_ptr<v3d::ui::component::menu::Bar> header(std::size_t area) const {
        return boost::dynamic_pointer_cast<v3d::ui::component::menu::Bar>(container(area)->get("header"));
    }

    /**
     * @return the menu bar holding an area's own menu: its header, or its strip once it has tabs
     **/
    boost::shared_ptr<v3d::ui::component::menu::Bar> areaMenu(std::size_t area) const {
        const boost::shared_ptr<v3d::ui::Component> strip = container(area)->get("strip-menu");
        return boost::dynamic_pointer_cast<v3d::ui::component::menu::Bar>(strip ? strip : container(area)->get("header"));
    }

    boost::shared_ptr<v3d::ui::Container> container(std::size_t area) const {
        return ui->container(Area::name(layout.areas()[area].id));
    }

    /**
     * Draw every container, which lays the components out.
     **/
    void draw() {
        canvas.clear();
        for (const boost::shared_ptr<v3d::ui::Container>& each : ui->containers()) {
            renderer.draw(&canvas, *each);
        }
    }

    boost::shared_ptr<entt::dispatcher> dispatcher;
    boost::shared_ptr<v3d::ui::Interface> ui;
    boost::shared_ptr<AreaLayout> held;
    AreaLayout& layout;
    boost::shared_ptr<AreaViews> views;
    Area chrome;
    v3d::editor::selection::SelectMask mask = v3d::editor::selection::SelectMask::Object;
    std::size_t active = 0;
    v3d::render::realtime::canvas::Canvas canvas;
    v3d::ui::paint::ComponentRenderer renderer;
};

/**
 * @return the whole number an event carries, or nothing
 **/
std::optional<int> number(const v3d::event::Event& event) {
    const boost::optional<v3d::event::EventData> data = event.data();
    if (!data || !std::holds_alternative<int>(*data)) {
        return std::nullopt;
    }
    return std::get<int>(*data);
}

BOOST_AUTO_TEST_SUITE(area_chrome_test)

/**
 * Each area has a container before the editor's own, holding a header menu named for the kind
 * of editor shown and a maximise button, each sending its command with the area's id.
 **/
BOOST_FIXTURE_TEST_CASE(each_area_has_a_header_before_the_editors_ui, Chrome) {
    sync();
    BOOST_REQUIRE_EQUAL(ui->containers().size(), 3U);
    BOOST_CHECK_EQUAL(ui->containers()[0]->name(), Area::name(layout.areas()[0].id));
    BOOST_CHECK_EQUAL(ui->containers()[2]->name(), "editor");

    const int id = static_cast<int>(layout.areas()[1].id);
    const boost::shared_ptr<v3d::ui::component::menu::Bar> header =
        boost::dynamic_pointer_cast<v3d::ui::component::menu::Bar>(container(1)->get("header"));
    BOOST_REQUIRE(header);
    BOOST_CHECK(!header->docked());
    BOOST_REQUIRE_GE(header->count(), 1U);
    BOOST_CHECK_EQUAL(header->label(0), "Viewport");
    const boost::shared_ptr<v3d::ui::component::menu::Item> split = header->find("area::split::left");
    BOOST_REQUIRE(split);
    BOOST_CHECK(number(split->event()) == id);
    for (const char* command : { "area::join::top", "area::swap::bottom", "area::tab::new", "area::tab::close", "area::maximise" }) {
        const boost::shared_ptr<v3d::ui::component::menu::Item> found = header->find(command);
        BOOST_REQUIRE_MESSAGE(found, command);
        BOOST_CHECK(number(found->event()) == id);
    }

    const boost::shared_ptr<v3d::ui::component::Button> maximise =
        boost::dynamic_pointer_cast<v3d::ui::component::Button>(container(1)->get("maximise"));
    BOOST_REQUIRE(maximise);
    BOOST_CHECK_EQUAL(maximise->event().str(), "area::maximise");
    BOOST_CHECK(number(maximise->event()) == id);
    BOOST_CHECK(!container(1)->get("tabs"));
}

/**
 * The header runs across the top of where the editor draws, less the button at its right end.
 **/
BOOST_FIXTURE_TEST_CASE(the_header_sits_at_the_top_of_its_area, Chrome) {
    sync();
    draw();
    const boost::shared_ptr<v3d::ui::Component> header = container(1)->get("header");
    const boost::shared_ptr<v3d::ui::Component> maximise = container(1)->get("maximise");
    BOOST_CHECK(header->position() == glm::vec2(401.0F, 3.0F));
    BOOST_CHECK(header->size() == glm::vec2(368.0F, 28.0F));
    BOOST_CHECK(maximise->position() == glm::vec2(769.0F, 3.0F));
    BOOST_CHECK(maximise->size() == glm::vec2(28.0F, 28.0F));

    // a dragged divider moves the header without building it again
    layout.grab(399.0F, 300.0F);
    layout.drag(500.0F, 300.0F);
    layout.release();
    sync();
    draw();
    BOOST_CHECK(container(1)->get("header") == header);
    BOOST_CHECK_GT(header->position().x, 401.0F);
}

/**
 * An area with two editors has a tab for each, above its editor, and the chosen one is checked.
 * tabAt() finds a tab's editor.
 **/
BOOST_FIXTURE_TEST_CASE(two_editors_have_tabs, Chrome) {
    const std::optional<std::size_t> left = layout.add(0, boost::json::parse(R"({"kind": "viewport", "camera": "Left"})").as_object());
    BOOST_REQUIRE(left);
    sync();
    draw();

    const boost::shared_ptr<v3d::ui::Component> tabs = container(0)->get("tabs");
    BOOST_REQUIRE(tabs);
    BOOST_REQUIRE_EQUAL(tabs->children().size(), 2U);
    const boost::shared_ptr<v3d::ui::component::Button> front = boost::dynamic_pointer_cast<v3d::ui::component::Button>(tabs->children()[0]);
    const boost::shared_ptr<v3d::ui::component::Button> second = boost::dynamic_pointer_cast<v3d::ui::component::Button>(tabs->children()[1]);
    BOOST_REQUIRE(front && second);
    BOOST_CHECK_EQUAL(front->label(), "Front");
    BOOST_CHECK_EQUAL(second->label(), "Left");
    BOOST_CHECK(!front->checked());
    BOOST_CHECK(second->checked());
    BOOST_CHECK_EQUAL(second->event().str(), "area::tab::choose");
    BOOST_CHECK(number(second->event()) == static_cast<int>(*left));

    // the tabs take the top strip, and the header sits under them
    BOOST_CHECK_EQUAL(tabs->position().y, 3.0F);
    BOOST_CHECK_EQUAL(container(0)->get("header")->position().y, 31.0F);

    const glm::vec2 middle = second->position() + (second->size() * 0.5F);
    BOOST_CHECK(chrome.tabAt(middle) == *left);
    BOOST_CHECK(!chrome.tabAt(glm::vec2(600.0F, 300.0F)));

    // the header names the camera shown, so choosing another view builds the area again. The
    // area's own menu is on the strip, so the camera's is the header's first
    BOOST_REQUIRE(layout.choose(0, 0));
    sync();
    const std::vector<boost::shared_ptr<v3d::ui::Component>>& chosen = container(0)->get("tabs")->children();
    BOOST_REQUIRE_EQUAL(chosen.size(), 2U);
    BOOST_CHECK(boost::dynamic_pointer_cast<v3d::ui::component::Button>(chosen[0])->checked());
    BOOST_CHECK(!boost::dynamic_pointer_cast<v3d::ui::component::Button>(chosen[1])->checked());
    BOOST_CHECK_EQUAL(header(0)->label(0), "Front");
}

/**
 * A tabbed area's strip holds what acts on the area: a recess, the area's menu, the tabs with a
 * strip's look, a + whose menu adds a tab of each kind, and maximise. The header under it holds
 * only the view's menus.
 **/
BOOST_FIXTURE_TEST_CASE(a_tabbed_area_holds_its_own_commands_on_its_strip, Chrome) {
    BOOST_REQUIRE(layout.add(0, boost::json::parse(R"({"kind": "viewport", "camera": "Left"})").as_object()));
    sync();
    draw();

    const boost::shared_ptr<v3d::ui::component::Panel> recess = boost::dynamic_pointer_cast<v3d::ui::component::Panel>(container(0)->get("strip"));
    BOOST_REQUIRE(recess);
    BOOST_CHECK(recess->recess());
    const boost::shared_ptr<v3d::ui::component::menu::Bar> strip = areaMenu(0);
    BOOST_REQUIRE(strip && strip != header(0));
    BOOST_CHECK(strip->find("area::split::left"));
    BOOST_CHECK(strip->find("area::tab::close"));
    BOOST_CHECK(!header(0)->find("area::split::left"));
    BOOST_CHECK(header(0)->find("view::camera::front"));

    const boost::shared_ptr<v3d::ui::component::menu::Bar> add =
        boost::dynamic_pointer_cast<v3d::ui::component::menu::Bar>(container(0)->get("add"));
    BOOST_REQUIRE(add);
    BOOST_CHECK_EQUAL(add->label(0), "+");
    const boost::shared_ptr<v3d::ui::component::menu::Item> outliner = add->find("area::tab::add::outliner");
    BOOST_REQUIRE(outliner);
    BOOST_CHECK(number(outliner->event()) == static_cast<int>(layout.areas()[0].id));

    const boost::shared_ptr<v3d::ui::component::Button> tab =
        boost::dynamic_pointer_cast<v3d::ui::component::Button>(container(0)->get("tabs")->children()[0]);
    BOOST_REQUIRE(tab);
    BOOST_CHECK(tab->tab() == v3d::ui::component::Button::Tab::Strip);
    // maximise is on the strip, at its right end
    const boost::shared_ptr<v3d::ui::Component> maximise = container(0)->get("maximise");
    BOOST_CHECK_EQUAL(maximise->position().y, tab->position().y);
    BOOST_CHECK_GT(maximise->position().x, tab->position().x);
}

/**
 * A tabbed area showing a panel has no header, so the panel starts under the strip. An area
 * with one editor, and a tabbed area showing a viewport, each have one.
 **/
BOOST_FIXTURE_TEST_CASE(a_tabbed_panel_has_no_header, Chrome) {
    BOOST_CHECK(Area::headed(layout.areas()[0]));
    BOOST_REQUIRE(layout.add(0, boost::json::parse(R"({"kind": "outliner"})").as_object()));
    sync();
    BOOST_CHECK(!container(0)->get("header"));
    BOOST_CHECK(!Area::headed(layout.areas()[0]));
    BOOST_REQUIRE(layout.choose(0, 0));
    sync();
    BOOST_CHECK(container(0)->get("header"));
    BOOST_CHECK(Area::headed(layout.areas()[0]));
}

/**
 * A maximised area's button restores the layout, and a note after its header's menus names the
 * key that does it. Restoring the layout puts the maximise button back.
 **/
BOOST_FIXTURE_TEST_CASE(a_maximised_area_offers_to_restore, Chrome) {
    chrome.restoreKey("Ctrl+Space");
    layout.maximise(0);
    sync();
    draw();
    const boost::shared_ptr<v3d::ui::component::Button> button =
        boost::dynamic_pointer_cast<v3d::ui::component::Button>(container(0)->get("maximise"));
    BOOST_REQUIRE(button);
    BOOST_CHECK_EQUAL(button->icon(), "icon:restore");
    BOOST_CHECK_EQUAL(header(0)->note(), "Ctrl+Space restores the areas");

    layout.maximise(0);
    sync();
    const boost::shared_ptr<v3d::ui::component::Button> restored =
        boost::dynamic_pointer_cast<v3d::ui::component::Button>(container(0)->get("maximise"));
    BOOST_REQUIRE(restored);
    BOOST_CHECK_EQUAL(restored->icon(), "icon:maximise");
    BOOST_CHECK(header(0)->note().empty());
}

/**
 * A change to the areas builds the containers again, and an area with no room hides its own.
 **/
BOOST_FIXTURE_TEST_CASE(a_change_to_the_areas_builds_again, Chrome) {
    sync();
    const boost::shared_ptr<v3d::ui::Container> before = container(0);
    BOOST_REQUIRE(layout.split(0, AreaLayout::Edge::Bottom) < layout.areas().size());
    sync();
    BOOST_CHECK_EQUAL(ui->containers().size(), 4U);
    BOOST_CHECK(container(0) != before);
    BOOST_CHECK(!ui->container(before->name()) || ui->container(before->name()) != before);

    layout.maximise(2);
    sync();
    BOOST_CHECK(!container(0)->visible());
    BOOST_CHECK(container(2)->visible());
}

/**
 * A viewport's header names its camera and its shading as menus. Their items send the view's
 * commands with the area's id, and mark what the view holds.
 **/
BOOST_FIXTURE_TEST_CASE(a_viewports_header_names_its_camera_and_shading, Chrome) {
    views->of(1)->display().shading = v3d::editor::view::Shading::Smooth;
    sync();
    const boost::shared_ptr<v3d::ui::component::menu::Bar> bar = header(1);
    BOOST_REQUIRE_EQUAL(bar->count(), 5U);
    BOOST_CHECK_EQUAL(bar->label(1), "Top");
    BOOST_CHECK_EQUAL(bar->label(2), "Smooth");

    const int id = static_cast<int>(layout.areas()[1].id);
    for (const char* command : { "view::camera::front", "view::camera::perspective", "view::camera::orthographic", "view::shade::flat",
        "view::rendermode::front::lines", "view::rendermode::back::points", "view::cull::both" }) {
        const boost::shared_ptr<v3d::ui::component::menu::Item> found = bar->find(command);
        BOOST_REQUIRE_MESSAGE(found, command);
        BOOST_CHECK(number(found->event()) == id);
    }
    BOOST_CHECK(bar->find("view::camera::top")->checked());
    BOOST_CHECK(!bar->find("view::camera::front")->checked());
    BOOST_CHECK(bar->find("view::shade::smooth")->checked());
    BOOST_CHECK(!bar->find("view::shade::flat")->checked());
    BOOST_CHECK(bar->find("view::cull::back")->checked());
    BOOST_CHECK(bar->find("view::camera::orthographic")->checked() == views->of(1)->camera()->orthographic());

    // a change of what the view holds that its menus do not name is marked without a rebuild
    views->of(1)->display().cull = ViewPort::Cull::None;
    sync();
    BOOST_CHECK(header(1) == bar);
    BOOST_CHECK(bar->find("view::cull::none")->checked());
    BOOST_CHECK(!bar->find("view::cull::back")->checked());
}

/**
 * A viewport's camera menu offers the scene's cameras after the fixed views, each sending its
 * camera's id and the area's. A view through a camera marks it and no fixed view, and offers the
 * lock, which a free view does not. A change to the cameras builds the headers again.
 **/
BOOST_FIXTURE_TEST_CASE(a_camera_menu_offers_the_scenes_cameras, Chrome) {
    sync();
    BOOST_CHECK(!header(0)->find("view::camera::scene"));
    BOOST_CHECK(!header(0)->find("view::camera::lock"));

    chrome.cameras({ { .id = 7, .name = "Wide" }, { .id = 9, .name = "Close" } });
    views->of(0)->through({ .camera = 9, .scene = false, .locked = true });
    views->of(0)->looking(9);
    sync();
    const boost::shared_ptr<v3d::ui::component::menu::Bar> bar = header(0);
    const boost::shared_ptr<v3d::ui::component::menu::Menu> cameras = bar->menu(1);
    const std::string area = std::to_string(layout.areas()[0].id);
    std::vector<std::string> sent;
    for (std::size_t index = 0; index < cameras->count(); index++) {
        const boost::shared_ptr<v3d::ui::component::menu::Item>& item = (*cameras)[index];
        if (item && item->event().name() == "camera::scene") {
            const boost::optional<v3d::event::EventData> data = item->event().data();
            BOOST_REQUIRE(data && std::holds_alternative<std::string>(*data));
            sent.push_back(std::get<std::string>(*data));
            BOOST_CHECK_EQUAL(item->checked(), sent.back().starts_with("9@"));
        }
    }
    BOOST_CHECK(sent == (std::vector<std::string>{ "7@" + area, "9@" + area }));
    BOOST_REQUIRE(bar->find("view::camera::lock"));
    BOOST_CHECK(bar->find("view::camera::lock")->checked());
    BOOST_CHECK(!bar->find("view::camera::front")->checked());
    BOOST_CHECK(!header(1)->find("view::camera::lock"));
}

/**
 * A viewport's header ends with a menu named for the select mask, whose items send each mask's
 * command with the area's id and mark the one in force. A new mask builds the header again.
 **/
BOOST_FIXTURE_TEST_CASE(a_viewports_header_names_the_mask, Chrome) {
    sync();
    BOOST_CHECK_EQUAL(header(0)->label(3), "Object");
    const boost::shared_ptr<v3d::ui::component::menu::Item> face = header(0)->find("select::mask::face");
    BOOST_REQUIRE(face);
    BOOST_CHECK(number(face->event()) == static_cast<int>(layout.areas()[0].id));
    BOOST_CHECK(!face->checked());
    BOOST_CHECK(header(0)->find("select::mask::object")->checked());

    mask = v3d::editor::selection::SelectMask::Face;
    sync();
    BOOST_CHECK_EQUAL(header(0)->label(3), "Face");
    BOOST_CHECK(header(0)->find("select::mask::face")->checked());
    BOOST_CHECK(!header(0)->find("select::mask::object")->checked());
}

/**
 * A viewport's header is drawn partly through over the view. Its editor menu shows the kind's
 * icon, every menu has a drop mark, and it ends with an Overlays menu that marks what the view
 * draws. The active view's camera menu is set apart, and moves with the active area.
 **/
BOOST_FIXTURE_TEST_CASE(a_viewports_header_is_dressed_as_menus, Chrome) {
    views->of(1)->show(ViewPort::SHOW_GRID, false);
    sync();
    const boost::shared_ptr<v3d::ui::component::menu::Bar> bar = header(1);
    BOOST_CHECK_CLOSE(bar->opacity(), static_cast<float>(Area::HEADER_OPACITY), 0.001F);
    BOOST_CHECK_EQUAL(bar->menu(0)->icon(), "icon:editor-viewport");
    for (std::size_t index = 0; index < bar->count(); index++) {
        BOOST_CHECK_MESSAGE(bar->menu(index)->drop(), bar->label(index));
    }
    BOOST_CHECK_EQUAL(bar->label(4), "Overlays");
    BOOST_CHECK_EQUAL(bar->menu(4)->icon(), "icon:overlays");
    const boost::shared_ptr<v3d::ui::component::menu::Item> grid = bar->find("view::show::grid");
    BOOST_REQUIRE(grid);
    BOOST_CHECK(number(grid->event()) == static_cast<int>(layout.areas()[1].id));
    BOOST_CHECK(!grid->checked());
    BOOST_CHECK(bar->find("view::show::mesh")->checked());

    BOOST_CHECK(header(0)->menu(1)->emphasis());
    BOOST_CHECK(!bar->menu(1)->emphasis());
    active = 1;
    sync();
    BOOST_CHECK(!header(0)->menu(1)->emphasis());
    BOOST_CHECK(bar->menu(1)->emphasis());
    BOOST_CHECK(!bar->menu(0)->emphasis());
}

/**
 * A change to what a view's menus are named for builds the header again under the new name.
 **/
BOOST_FIXTURE_TEST_CASE(a_new_camera_renames_the_header, Chrome) {
    sync();
    const boost::shared_ptr<v3d::ui::component::menu::Bar> before = header(0);
    views->of(0)->look(v3d::type::camera::Profile("Bottom"));
    views->of(0)->display().front = ViewPort::Fill::Lines;
    sync();
    BOOST_CHECK(header(0) != before);
    BOOST_CHECK_EQUAL(header(0)->label(1), "Bottom");
    BOOST_CHECK_EQUAL(header(0)->label(2), "Wireframe");
}

/**
 * The shading menu is named for the front fill when it is not filled, and for the shading when
 * it is.
 **/
BOOST_AUTO_TEST_CASE(the_shading_menu_names_the_front_fill_or_the_shading) {
    ViewPort::Display display;
    BOOST_CHECK_EQUAL(Area::shading(display), "Flat");
    display.shading = v3d::editor::view::Shading::Smooth;
    BOOST_CHECK_EQUAL(Area::shading(display), "Smooth");
    display.front = ViewPort::Fill::Lines;
    BOOST_CHECK_EQUAL(Area::shading(display), "Wireframe");
    display.front = ViewPort::Fill::Points;
    BOOST_CHECK_EQUAL(Area::shading(display), "Points");
}

/**
 * Another kind of editor has only the area's menu.
 **/
BOOST_FIXTURE_TEST_CASE(another_editor_has_no_view_menus, Chrome) {
    BOOST_REQUIRE(layout.replace(0, boost::json::parse(R"({"kind": "outliner"})").as_object()));
    sync();
    BOOST_REQUIRE_EQUAL(header(0)->count(), 1U);
    BOOST_CHECK_EQUAL(header(0)->label(0), "Outliner");
    // the header names the panel after the menu's icon, and a view's does not
    BOOST_REQUIRE(header(0)->menu(0));
    BOOST_CHECK(header(0)->menu(0)->named());
    BOOST_CHECK(!header(1)->menu(0)->named());
}

/**
 * The area's menu names its editor as the Editor menu does, not by the kind's own word.
 **/
BOOST_AUTO_TEST_CASE(the_area_menu_names_a_kind_as_the_editor_menu_does) {
    BOOST_CHECK_EQUAL(Area::kindLabel("render-view"), "Render View");
    BOOST_CHECK_EQUAL(Area::kindLabel("graph"), "Graph Editor");
    BOOST_CHECK_EQUAL(Area::kindLabel("unknown"), "Unknown");
}

/**
 * The area's menu offers each kind of editor, sending its command with the area's id, and marks
 * the kind the area shows.
 **/
BOOST_FIXTURE_TEST_CASE(the_area_menu_marks_the_kind_shown, Chrome) {
    BOOST_REQUIRE(layout.add(1, boost::json::parse(R"({"kind": "outliner"})").as_object()));
    sync();
    const boost::shared_ptr<v3d::ui::component::menu::Item> viewport = areaMenu(0)->find("area::editor::viewport");
    const boost::shared_ptr<v3d::ui::component::menu::Item> outliner = areaMenu(0)->find("area::editor::outliner");
    BOOST_REQUIRE(viewport && outliner);
    BOOST_CHECK(viewport->checked());
    BOOST_CHECK(!outliner->checked());
    BOOST_CHECK(number(viewport->event()) == static_cast<int>(layout.areas()[0].id));

    BOOST_CHECK(!areaMenu(1)->find("area::editor::viewport")->checked());
    BOOST_CHECK(areaMenu(1)->find("area::editor::outliner")->checked());

    // a timeline is one of the kinds offered
    BOOST_CHECK(areaMenu(0)->find("area::editor::timeline"));
    BOOST_CHECK(areaMenu(0)->find("area::editor::graph"));
}

/**
 * A tab names a viewport's camera, and any other editor's kind with a capital.
 **/
BOOST_AUTO_TEST_CASE(a_tab_names_its_editor) {
    BOOST_CHECK_EQUAL(Area::label({ .id = 1, .kind = "viewport", .state = boost::json::parse(R"({"camera": "Top"})").as_object() }), "Top");
    BOOST_CHECK_EQUAL(Area::label({ .id = 2, .kind = "outliner", .state = {} }), "Outliner");
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
