/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/event/Event.h>
#include <api/event/Router.h>
#include <api/log/Logger.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/ui/Component.h>
#include <api/ui/Container.h>
#include <api/ui/Interface.h>
#include <api/ui/component/Button.h>
#include <api/ui/component/menu/Bar.h>
#include <api/ui/component/menu/Item.h>
#include <api/ui/paint/ComponentRenderer.h>
#include <vertical3d/src/control/AreaChrome.h>
#include <vertical3d/src/view/AreaLayout.h>

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

#include <boost/json/parse.hpp>
#include <boost/make_shared.hpp>
#include <boost/optional.hpp>
#include <boost/test/unit_test.hpp>
#include <entt/entt.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace {

using v3d::editor::AreaChrome;
using v3d::editor::AreaLayout;

/**
 * Two areas side by side over an 800 by 600 window, under a ui whose own container is "editor".
 * The first area runs from 3 to 398 across and the second from 401 to 797, both from 3 to 597
 * down. Headers and tabs are 28 pixels tall.
 **/
struct Chrome {
    Chrome() :
        dispatcher(boost::make_shared<entt::dispatcher>()),
        ui(boost::make_shared<v3d::ui::Interface>(boost::make_shared<v3d::event::Router>(dispatcher), dispatcher,
            boost::make_shared<v3d::log::Logger>())),
        layout(boost::make_shared<v3d::log::Logger>()),
        chrome(ui, "editor", {}),
        renderer([](std::string_view text) { return static_cast<float>(text.size()) * 10.0F; },
            [](std::string_view, const glm::vec2&, const glm::vec4&) {}) {
        BOOST_REQUIRE(ui->load(boost::json::parse(
            R"({ "themes": [], "containers": [ { "name": "editor", "visible": true, "components": [] } ] })").as_object()));
        BOOST_REQUIRE(layout.load(boost::json::parse(R"({"root": {"split": "horizontal", "children": [)"
            R"({"editors": [{"kind": "viewport", "camera": "Front"}]}, {"editors": [{"kind": "viewport", "camera": "Top"}]}]}})").as_object()));
        layout.metrics({ .strip = 28.0F });
        layout.resize(glm::vec4(0.0F, 0.0F, 800.0F, 600.0F));
        canvas.resize(glm::uvec2(800, 600));
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
    AreaLayout layout;
    AreaChrome chrome;
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
    chrome.sync(layout, 28.0F);
    BOOST_REQUIRE_EQUAL(ui->containers().size(), 3U);
    BOOST_CHECK_EQUAL(ui->containers()[0]->name(), AreaChrome::name(layout.areas()[0].id));
    BOOST_CHECK_EQUAL(ui->containers()[2]->name(), "editor");

    const int id = static_cast<int>(layout.areas()[1].id);
    const boost::shared_ptr<v3d::ui::component::menu::Bar> header =
        boost::dynamic_pointer_cast<v3d::ui::component::menu::Bar>(container(1)->get("header"));
    BOOST_REQUIRE(header);
    BOOST_CHECK(!header->docked());
    BOOST_REQUIRE_EQUAL(header->count(), 1U);
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
    chrome.sync(layout, 28.0F);
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
    chrome.sync(layout, 28.0F);
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
    chrome.sync(layout, 28.0F);
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

    BOOST_REQUIRE(layout.choose(0, 0));
    chrome.sync(layout, 28.0F);
    BOOST_CHECK(front->checked());
    BOOST_CHECK(!second->checked());
}

/**
 * A change to the areas builds the containers again, and an area with no room hides its own.
 **/
BOOST_FIXTURE_TEST_CASE(a_change_to_the_areas_builds_again, Chrome) {
    chrome.sync(layout, 28.0F);
    const boost::shared_ptr<v3d::ui::Container> before = container(0);
    BOOST_REQUIRE(layout.split(0, AreaLayout::Edge::Bottom) < layout.areas().size());
    chrome.sync(layout, 28.0F);
    BOOST_CHECK_EQUAL(ui->containers().size(), 4U);
    BOOST_CHECK(container(0) != before);
    BOOST_CHECK(!ui->container(before->name()) || ui->container(before->name()) != before);

    layout.maximise(2);
    chrome.sync(layout, 28.0F);
    BOOST_CHECK(!container(0)->visible());
    BOOST_CHECK(container(2)->visible());
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
