/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/asset/Json.h>
#include <api/render/realtime/Canvas.h>
#include <api/ui/ComponentRenderer.h>
#include <api/ui/Container.h>
#include <api/ui/Engine.h>
#include <api/ui/component/Label.h>
#include <api/ui/component/TabBar.h>

#include <string>
#include <string_view>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <boost/json/parse.hpp>
#include <boost/make_shared.hpp>

namespace {

const float characterWidth = 10.0f;

struct Written final {
    std::string text;
    glm::vec2 pen;
    glm::vec4 colour;
};

v3d::ui::ComponentRenderer build(std::vector<Written>* written) {
    return v3d::ui::ComponentRenderer(
        [](std::string_view text) { return static_cast<float>(text.size()) * characterWidth; },
        [written](std::string_view text, const glm::vec2& pen, const glm::vec4& colour) {
            if (written != nullptr) {
                written->push_back(Written{std::string(text), pen, colour});
            }
        });
}

/**
 * A page with one label in it, so that a test can tell whether the page was walked.
 **/
boost::shared_ptr<v3d::ui::component::TabPage> page(const std::string& label, const std::string& holds) {
    boost::shared_ptr<v3d::ui::component::TabPage> component =
        boost::make_shared<v3d::ui::component::TabPage>();
    component->name(label);
    component->label(label);

    boost::shared_ptr<v3d::ui::component::Label> text = boost::make_shared<v3d::ui::component::Label>();
    text->name(holds);
    text->text(holds);
    component->add(text);
    return component;
}

/**
 * A bar of two pages in a box, laid out by a draw.
 **/
boost::shared_ptr<v3d::ui::component::TabBar> bar() {
    boost::shared_ptr<v3d::ui::component::TabBar> component =
        boost::make_shared<v3d::ui::component::TabBar>();
    component->name("panels");
    component->layout().width = v3d::ui::Length(200.0f, v3d::ui::Length::Unit::Pixels);
    component->layout().height = v3d::ui::Length(150.0f, v3d::ui::Length::Unit::Pixels);
    component->add(page("Turn", "turn 3"));
    component->add(page("Entities", "42 entities"));
    return component;
}

};  // namespace

BOOST_AUTO_TEST_SUITE(tab_bar_test)

/**
 * Only the chosen page is drawn, so what the other holds is neither written nor laid out and
 * nothing in it can be picked.
 **/
BOOST_AUTO_TEST_CASE(only_the_chosen_page_is_drawn) {
    std::vector<Written> written;
    v3d::ui::ComponentRenderer renderer = build(&written);
    v3d::render::realtime::Canvas canvas;
    canvas.resize(400, 300);

    const boost::shared_ptr<v3d::ui::component::TabBar> panels = bar();
    v3d::ui::Container container("hud", true);
    container.add(panels);
    renderer.draw(&canvas, container);

    // both tabs, and the contents of the first page alone
    BOOST_REQUIRE_EQUAL(written.size(), 3U);
    BOOST_CHECK_EQUAL(written[0].text, "Turn");
    BOOST_CHECK_EQUAL(written[1].text, "Entities");
    BOOST_CHECK_EQUAL(written[2].text, "turn 3");

    written.clear();
    panels->selected(1);
    renderer.draw(&canvas, container);

    BOOST_REQUIRE_EQUAL(written.size(), 3U);
    BOOST_CHECK_EQUAL(written[2].text, "42 entities");
}

/**
 * The page is laid out in what the strip left, so what it holds starts under the tabs rather
 * than behind them.
 **/
BOOST_AUTO_TEST_CASE(the_page_is_laid_out_under_the_strip) {
    v3d::ui::ComponentRenderer renderer = build(nullptr);
    v3d::render::realtime::Canvas canvas;
    canvas.resize(400, 300);

    const boost::shared_ptr<v3d::ui::component::TabBar> panels = bar();
    panels->layout().x = v3d::ui::Length(30.0f, v3d::ui::Length::Unit::Pixels);
    panels->layout().y = v3d::ui::Length(40.0f, v3d::ui::Length::Unit::Pixels);
    v3d::ui::Container container("hud", true);
    container.add(panels);
    renderer.draw(&canvas, container);

    const boost::shared_ptr<v3d::ui::component::TabPage> up = panels->page();
    BOOST_REQUIRE(up);
    BOOST_CHECK(up->position().y > panels->position().y + renderer.dressing().barHeight);
    BOOST_CHECK_CLOSE(up->position().x, panels->position().x, 0.001f);
    BOOST_CHECK_CLOSE(up->size().x, 200.0f, 0.001f);

    // and the page that is not up was never placed
    const std::vector<boost::shared_ptr<v3d::ui::component::TabPage>> pages = panels->pages();
    BOOST_REQUIRE_EQUAL(pages.size(), 2U);
    BOOST_CHECK_CLOSE(pages[1]->size().x, 0.0f, 0.001f);
}

/**
 * A tab is as wide as its label, and which one a point is on is answered against where the
 * draw put them. A bar that has never been drawn answers nothing, per ADR-0019.
 **/
BOOST_AUTO_TEST_CASE(a_point_names_the_tab_under_it) {
    v3d::ui::ComponentRenderer renderer = build(nullptr);
    v3d::render::realtime::Canvas canvas;
    canvas.resize(400, 300);

    const boost::shared_ptr<v3d::ui::component::TabBar> panels = bar();
    BOOST_CHECK_EQUAL(panels->at(glm::vec2(10.0f, 10.0f)), v3d::ui::component::TabBar::none);

    v3d::ui::Container container("hud", true);
    container.add(panels);
    renderer.draw(&canvas, container);

    BOOST_REQUIRE_EQUAL(panels->tabs().size(), 2U);
    const v3d::type::Bound2D& first = panels->tabs()[0];
    const v3d::type::Bound2D& second = panels->tabs()[1];
    BOOST_CHECK_CLOSE(first.size().x, 4.0f * characterWidth + renderer.dressing().padding, 0.001f);
    BOOST_CHECK(second.position().x > first.position().x + first.size().x - 1.0f);

    BOOST_CHECK_EQUAL(panels->at(first.position() + first.size() * 0.5f), 0);
    BOOST_CHECK_EQUAL(panels->at(second.position() + second.size() * 0.5f), 1);
    // past the last tab, on the empty part of the strip
    BOOST_CHECK_EQUAL(panels->at(glm::vec2(panels->position().x + 190.0f, panels->position().y + 2.0f)),
        v3d::ui::component::TabBar::none);
}

/**
 * Choosing a tab that is not there chooses none, and a bar holding no pages draws its strip
 * and nothing under it.
 **/
BOOST_AUTO_TEST_CASE(a_bar_with_no_pages_shows_nothing) {
    v3d::ui::ComponentRenderer renderer = build(nullptr);
    v3d::render::realtime::Canvas canvas;
    canvas.resize(400, 300);

    const boost::shared_ptr<v3d::ui::component::TabBar> panels =
        boost::make_shared<v3d::ui::component::TabBar>();
    panels->layout().width = v3d::ui::Length(200.0f, v3d::ui::Length::Unit::Pixels);
    panels->layout().height = v3d::ui::Length(150.0f, v3d::ui::Length::Unit::Pixels);
    panels->selected(0);

    BOOST_CHECK_EQUAL(panels->selected(), v3d::ui::component::TabBar::none);
    BOOST_CHECK(!panels->page());

    v3d::ui::Container container("hud", true);
    container.add(panels);
    renderer.draw(&canvas, container);
    // the rule under an empty strip, and nothing else
    BOOST_CHECK(!canvas.empty());
    BOOST_CHECK(panels->tabs().empty());

    const boost::shared_ptr<v3d::ui::component::TabBar> two = bar();
    two->selected(7);
    BOOST_CHECK_EQUAL(two->selected(), v3d::ui::component::TabBar::none);
}

/**
 * The loader builds the strip out of the pages the children array named, and reads which of
 * them is up after they are there.
 **/
BOOST_AUTO_TEST_CASE(the_loader_reads_a_strip_of_pages) {
    const std::string document = R"({
  "themes": [ { "name": "default" } ],
  "containers": [
    {
      "name": "hud",
      "visible": true,
      "components": [
        {
          "name": "panels", "type": "tabs", "selected": 1,
          "children": [
            { "name": "turn", "type": "tab", "label": "Turn",
              "children": [ { "name": "count", "type": "label", "label": "turn 3" } ] },
            { "name": "entities", "type": "tab", "label": "Entities" }
          ]
        }
      ]
    }
  ]
})";

    boost::shared_ptr<entt::dispatcher> dispatcher = boost::make_shared<entt::dispatcher>();
    boost::shared_ptr<v3d::ui::Engine> ui = boost::make_shared<v3d::ui::Engine>(
        boost::make_shared<v3d::event::Engine>(dispatcher), dispatcher,
        boost::make_shared<v3d::log::Logger>());
    BOOST_REQUIRE(ui->load(boost::make_shared<v3d::asset::Json>(
        "vgui", v3d::asset::Type::JsonDocument, boost::json::parse(document).as_object())));

    const boost::shared_ptr<v3d::ui::component::TabBar> panels =
        boost::dynamic_pointer_cast<v3d::ui::component::TabBar>(ui->container("hud")->get("panels"));
    BOOST_REQUIRE(panels);
    BOOST_REQUIRE_EQUAL(panels->pages().size(), 2U);
    BOOST_CHECK_EQUAL(panels->selected(), 1);
    BOOST_CHECK_EQUAL(std::string(panels->page()->label()), "Entities");
    // and a page keeps what it holds, whether or not it is the one that is up
    BOOST_CHECK(ui->container("hud")->get("count"));
}

BOOST_AUTO_TEST_SUITE_END()
