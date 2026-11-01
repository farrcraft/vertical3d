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
#include <api/ui/component/menu/Bar.h>
#include <api/ui/component/menu/Item.h>
#include <api/ui/paint/ComponentRenderer.h>
#include <vertical3d/src/control/AreaChrome.h>
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

using v3d::editor::control::AreaChrome;
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
        renderer([](std::string_view text) { return static_cast<float>(text.size()) * 10.0F; },
            [](std::string_view, const glm::vec2&, const glm::vec4&) {}) {
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
     * @return an area's header
     **/
    boost::shared_ptr<v3d::ui::component::menu::Bar> header(std::size_t area) const {
        return boost::dynamic_pointer_cast<v3d::ui::component::menu::Bar>(container(area)->get("header"));
    }

    boost::shared_ptr<v3d::ui::Container> container(std::size_t area) const {
        return ui->container(AreaChrome::name(layout.areas()[area].id));
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
    AreaChrome chrome;
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
    BOOST_CHECK_EQUAL(ui->containers()[0]->name(), AreaChrome::name(layout.areas()[0].id));
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

    // the header names the camera shown, so choosing another view builds the area again
    BOOST_REQUIRE(layout.choose(0, 0));
    sync();
    const std::vector<boost::shared_ptr<v3d::ui::Component>>& chosen = container(0)->get("tabs")->children();
    BOOST_REQUIRE_EQUAL(chosen.size(), 2U);
    BOOST_CHECK(boost::dynamic_pointer_cast<v3d::ui::component::Button>(chosen[0])->checked());
    BOOST_CHECK(!boost::dynamic_pointer_cast<v3d::ui::component::Button>(chosen[1])->checked());
    BOOST_CHECK_EQUAL(header(0)->label(1), "Front");
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
    BOOST_CHECK_CLOSE(bar->opacity(), static_cast<float>(AreaChrome::HEADER_OPACITY), 0.001F);
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
    BOOST_CHECK_EQUAL(AreaChrome::shading(display), "Flat");
    display.shading = v3d::editor::view::Shading::Smooth;
    BOOST_CHECK_EQUAL(AreaChrome::shading(display), "Smooth");
    display.front = ViewPort::Fill::Lines;
    BOOST_CHECK_EQUAL(AreaChrome::shading(display), "Wireframe");
    display.front = ViewPort::Fill::Points;
    BOOST_CHECK_EQUAL(AreaChrome::shading(display), "Points");
}

/**
 * Another kind of editor has only the area's menu.
 **/
BOOST_FIXTURE_TEST_CASE(another_editor_has_no_view_menus, Chrome) {
    BOOST_REQUIRE(layout.add(0, boost::json::parse(R"({"kind": "outliner"})").as_object()));
    sync();
    BOOST_REQUIRE_EQUAL(header(0)->count(), 1U);
    BOOST_CHECK_EQUAL(header(0)->label(0), "Outliner");
}

/**
 * The area's menu offers each kind of editor, sending its command with the area's id, and marks
 * the kind the area shows.
 **/
BOOST_FIXTURE_TEST_CASE(the_area_menu_marks_the_kind_shown, Chrome) {
    BOOST_REQUIRE(layout.add(1, boost::json::parse(R"({"kind": "outliner"})").as_object()));
    sync();
    const boost::shared_ptr<v3d::ui::component::menu::Item> viewport = header(0)->find("area::editor::viewport");
    const boost::shared_ptr<v3d::ui::component::menu::Item> outliner = header(0)->find("area::editor::outliner");
    BOOST_REQUIRE(viewport && outliner);
    BOOST_CHECK(viewport->checked());
    BOOST_CHECK(!outliner->checked());
    BOOST_CHECK(number(viewport->event()) == static_cast<int>(layout.areas()[0].id));

    BOOST_CHECK(!header(1)->find("area::editor::viewport")->checked());
    BOOST_CHECK(header(1)->find("area::editor::outliner")->checked());

    // a timeline is one of the kinds offered
    BOOST_CHECK(header(0)->find("area::editor::timeline"));
    BOOST_CHECK(header(0)->find("area::editor::graph"));
}

/**
 * A tab names a viewport's camera, and any other editor's kind with a capital.
 **/
BOOST_AUTO_TEST_CASE(a_tab_names_its_editor) {
    BOOST_CHECK_EQUAL(AreaChrome::label({ .id = 1, .kind = "viewport", .state = boost::json::parse(R"({"camera": "Top"})").as_object() }), "Top");
    BOOST_CHECK_EQUAL(AreaChrome::label({ .id = 2, .kind = "outliner", .state = {} }), "Outliner");
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
