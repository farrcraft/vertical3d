/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <string>
#include <string_view>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "../Cursor.h"

#include "../ComponentRenderer.h"
#include "../Container.h"
#include "../Engine.h"
#include "../../asset/Json.h"
#include "../component/Button.h"
#include "../component/CheckBox.h"
#include "../component/Panel.h"
#include "../component/Scrollbar.h"
#include "../component/SelectList.h"
#include "../component/TabBar.h"
#include "../component/TabPage.h"

#include "../../render/realtime/Canvas.h"

#include <boost/json/parse.hpp>
#include <boost/make_shared.hpp>
#include <entt/entt.hpp>

namespace {

const float characterWidth = 10.0f;

/**
 * A ui engine holding one container, built by hand rather than loaded, plus the renderer
 * that places what it holds and the router that answers a cursor over it.
 *
 * Nothing is picked until something has been drawn, per ADR-0019, so every case here draws
 * before it clicks.
 **/
struct Fixture final {
    Fixture() :
        dispatcher(boost::make_shared<entt::dispatcher>()),
        context(boost::make_shared<v3d::event::Context>("test")),
        renderer(
            [](std::string_view text) { return static_cast<float>(text.size()) * characterWidth; },
            [](std::string_view, const glm::vec2&, const glm::vec4&) {}) {
        dispatcher->sink<v3d::event::Event>().connect<&Fixture::receive>(*this);
        canvas.resize(800, 600);

        ui = boost::make_shared<v3d::ui::Engine>(
            boost::make_shared<v3d::event::Engine>(dispatcher), dispatcher,
            boost::make_shared<v3d::log::Logger>());
        bool loaded = false;
        loaded = ui->load(boost::make_shared<v3d::asset::Json>("vgui", v3d::asset::Type::JsonDocument,
            boost::json::parse(R"({ "themes": [], "containers": [ { "name": "hud", "visible": true, "components": [] } ] })").as_object()));
        BOOST_REQUIRE(loaded);
        container = ui->container("hud");
        BOOST_REQUIRE(container);
        cursor = boost::make_shared<v3d::ui::Cursor>(ui, dispatcher);
    }

    void receive(const v3d::event::Event& event) {
        sent.push_back(event.str());
    }

    /**
     * Place a component at a box and draw the container, which is what leaves it pickable.
     **/
    void place(const boost::shared_ptr<v3d::ui::Component>& component, const glm::vec2& corner,
        const glm::vec2& size) {
        component->pickable(true);
        component->layout().x = v3d::ui::Length(corner.x, v3d::ui::Length::Unit::Pixels);
        component->layout().y = v3d::ui::Length(corner.y, v3d::ui::Length::Unit::Pixels);
        component->layout().width = v3d::ui::Length(size.x, v3d::ui::Length::Unit::Pixels);
        component->layout().height = v3d::ui::Length(size.y, v3d::ui::Length::Unit::Pixels);
        container->add(component);
    }

    void draw() {
        canvas.clear();
        renderer.draw(&canvas, *container);
    }

    boost::shared_ptr<entt::dispatcher> dispatcher;
    boost::shared_ptr<v3d::event::Context> context;
    v3d::render::realtime::Canvas canvas;
    std::vector<std::string> sent;
    v3d::ui::ComponentRenderer renderer;
    boost::shared_ptr<v3d::ui::Engine> ui;
    boost::shared_ptr<v3d::ui::Container> container;
    boost::shared_ptr<v3d::ui::Cursor> cursor;
};

};  // namespace

BOOST_AUTO_TEST_SUITE(cursor_test)

/**
 * A button in a container answers a click by sending its command, which is what nothing did
 * before ADR-0038: a button outside a strip carried a bound event and nothing sent it.
 **/
BOOST_AUTO_TEST_CASE(a_button_in_a_container_sends_its_command) {
    Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::Button> button =
        boost::make_shared<v3d::ui::component::Button>();
    button->label("Start");
    button->event(v3d::event::Event("start", fixture.context));
    fixture.place(button, glm::vec2(100.0f, 50.0f), glm::vec2(120.0f, 30.0f));
    fixture.draw();

    BOOST_CHECK(fixture.cursor->press(glm::vec2(160.0f, 65.0f)));
    BOOST_REQUIRE_EQUAL(fixture.sent.size(), 1U);
    BOOST_CHECK_EQUAL(fixture.sent.front(), "test::start");
}

/**
 * A press that lands on nothing pickable is not taken, so a hud of labels over a scene leaves
 * the scene clickable - which is what ADR-0034's pickable() default of false is for.
 **/
BOOST_AUTO_TEST_CASE(a_press_on_nothing_pickable_falls_through) {
    Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::Panel> plate =
        boost::make_shared<v3d::ui::component::Panel>();
    fixture.place(plate, glm::vec2(0.0f, 0.0f), glm::vec2(200.0f, 100.0f));
    plate->pickable(false);
    fixture.draw();

    BOOST_CHECK(!fixture.cursor->press(glm::vec2(50.0f, 50.0f)));
    BOOST_CHECK(fixture.sent.empty());

    // and one that is pickable takes the press even though it sends nothing
    plate->pickable(true);
    BOOST_CHECK(fixture.cursor->press(glm::vec2(50.0f, 50.0f)));
    BOOST_CHECK(fixture.sent.empty());
}

/**
 * Nothing is picked until it has been drawn, per ADR-0019, so a router asked before the first
 * frame answers nothing rather than guessing.
 **/
BOOST_AUTO_TEST_CASE(nothing_is_picked_before_anything_is_drawn) {
    Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::Button> button =
        boost::make_shared<v3d::ui::component::Button>();
    button->event(v3d::event::Event("start", fixture.context));
    fixture.place(button, glm::vec2(100.0f, 50.0f), glm::vec2(120.0f, 30.0f));

    BOOST_CHECK(!fixture.cursor->press(glm::vec2(160.0f, 65.0f)));
    BOOST_CHECK(fixture.sent.empty());
}

/**
 * A check box sends its command and marks nothing: whatever answers the command sets checked(),
 * so the mark cannot disagree with what the item reports - ADR-0019.
 **/
BOOST_AUTO_TEST_CASE(a_check_box_sends_its_command_and_marks_nothing) {
    Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::CheckBox> box =
        boost::make_shared<v3d::ui::component::CheckBox>();
    box->label("Grid");
    box->event(v3d::event::Event("grid", fixture.context));
    fixture.place(box, glm::vec2(10.0f, 10.0f), glm::vec2(140.0f, 24.0f));
    fixture.draw();

    BOOST_CHECK(fixture.cursor->press(glm::vec2(20.0f, 20.0f)));
    BOOST_REQUIRE_EQUAL(fixture.sent.size(), 1U);
    BOOST_CHECK_EQUAL(fixture.sent.front(), "test::grid");
    BOOST_CHECK(!box->checked());
}

/**
 * A list moves its selection to the row that was clicked and then sends its command, so an
 * app that wants the row reads selected() when the command arrives.
 **/
BOOST_AUTO_TEST_CASE(a_list_chooses_the_row_under_the_cursor) {
    Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::SelectList> list =
        boost::make_shared<v3d::ui::component::SelectList>();
    list->items({ "alpha", "beta", "gamma" });
    list->event(v3d::event::Event("load", fixture.context));
    fixture.place(list, glm::vec2(20.0f, 20.0f), glm::vec2(200.0f, 200.0f));
    fixture.draw();

    const float row = list->rowHeight();
    BOOST_REQUIRE_GT(row, 0.0f);
    // the middle of the second row
    BOOST_CHECK(fixture.cursor->press(glm::vec2(60.0f, 20.0f + row * 1.5f)));
    BOOST_CHECK_EQUAL(list->selected(), 1);
    BOOST_REQUIRE_EQUAL(fixture.sent.size(), 1U);
    BOOST_CHECK_EQUAL(fixture.sent.front(), "test::load");
}

/**
 * A tab bar owns which page is up, the way a list owns which row is chosen, so a click on a
 * tab changes the page and sends nothing.
 **/
BOOST_AUTO_TEST_CASE(a_tab_bar_changes_its_page_and_sends_nothing) {
    Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::TabBar> bar =
        boost::make_shared<v3d::ui::component::TabBar>();
    const boost::shared_ptr<v3d::ui::component::TabPage> first =
        boost::make_shared<v3d::ui::component::TabPage>();
    first->label("Scene");
    const boost::shared_ptr<v3d::ui::component::TabPage> second =
        boost::make_shared<v3d::ui::component::TabPage>();
    second->label("Render");
    bar->add(first);
    bar->add(second);
    fixture.place(bar, glm::vec2(0.0f, 0.0f), glm::vec2(400.0f, 300.0f));
    fixture.draw();

    BOOST_REQUIRE_EQUAL(bar->tabs().size(), 2U);
    const v3d::type::Bound2D& tab = bar->tabs()[1];
    BOOST_CHECK(fixture.cursor->press(tab.position() + tab.size() * 0.5f));
    BOOST_CHECK_EQUAL(bar->selected(), 1);
    BOOST_CHECK(fixture.sent.empty());
}

/**
 * A scrollbar is the one widget whose input is a drag: the press jumps the thumb to what was
 * clicked, and the cursor goes on being followed until it comes up - even off the bar.
 **/
BOOST_AUTO_TEST_CASE(a_scrollbar_follows_the_cursor_until_the_press_comes_up) {
    Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::Scrollbar> bar =
        boost::make_shared<v3d::ui::component::Scrollbar>();
    bar->range(1000.0f, 200.0f);
    fixture.place(bar, glm::vec2(300.0f, 0.0f), glm::vec2(12.0f, 200.0f));
    fixture.draw();

    BOOST_CHECK(fixture.cursor->press(glm::vec2(306.0f, 20.0f)));
    BOOST_CHECK_EQUAL(fixture.cursor->held(), bar);
    const float near = bar->offset();

    // dragged down, and well off the bar sideways - the press is still its
    BOOST_CHECK(fixture.cursor->motion(glm::vec2(500.0f, 180.0f)));
    BOOST_CHECK_GT(bar->offset(), near);

    BOOST_CHECK(fixture.cursor->release(glm::vec2(500.0f, 180.0f)));
    BOOST_CHECK(!fixture.cursor->held());
    // and a move afterwards is nothing of the bar's
    const float settled = bar->offset();
    fixture.cursor->motion(glm::vec2(500.0f, 20.0f));
    BOOST_CHECK_CLOSE(bar->offset(), settled, 0.001f);
}

/**
 * The topmost thing under the point takes the press, and nothing under it hears about it.
 **/
BOOST_AUTO_TEST_CASE(only_the_topmost_component_takes_a_press) {
    Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::Button> under =
        boost::make_shared<v3d::ui::component::Button>();
    under->event(v3d::event::Event("under", fixture.context));
    fixture.place(under, glm::vec2(0.0f, 0.0f), glm::vec2(200.0f, 200.0f));

    const boost::shared_ptr<v3d::ui::component::Button> over =
        boost::make_shared<v3d::ui::component::Button>();
    over->event(v3d::event::Event("over", fixture.context));
    fixture.place(over, glm::vec2(50.0f, 50.0f), glm::vec2(100.0f, 100.0f));
    fixture.draw();

    BOOST_CHECK(fixture.cursor->press(glm::vec2(100.0f, 100.0f)));
    BOOST_REQUIRE_EQUAL(fixture.sent.size(), 1U);
    BOOST_CHECK_EQUAL(fixture.sent.front(), "test::over");
}

BOOST_AUTO_TEST_SUITE_END()
