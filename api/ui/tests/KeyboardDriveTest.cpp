/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <string>
#include <string_view>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "../Container.h"
#include "../Engine.h"
#include "../Keys.h"

#include "../component/Button.h"
#include "../component/CheckBox.h"
#include "../component/Panel.h"
#include "../component/RadioButton.h"
#include "../component/SelectList.h"
#include "../component/TabBar.h"
#include "../component/TabPage.h"
#include "../component/TextBox.h"

#include "../../asset/Json.h"

#include <boost/json/parse.hpp>
#include <boost/make_shared.hpp>
#include <entt/entt.hpp>

namespace {

/**
 * A ui engine holding one container, and the key router over it. Nothing draws: which
 * component a key reaches is the focus's to say, and the focus is given by the engine
 * rather than by a box anything was drawn in.
 **/
struct Fixture final {
    Fixture() :
        dispatcher(boost::make_shared<entt::dispatcher>()),
        context(boost::make_shared<v3d::event::Context>("test")) {
        dispatcher->sink<v3d::event::Event>().connect<&Fixture::receive>(*this);
        ui = boost::make_shared<v3d::ui::Engine>(
            boost::make_shared<v3d::event::Engine>(dispatcher), dispatcher,
            boost::make_shared<v3d::log::Logger>());
        BOOST_REQUIRE(ui->load(boost::make_shared<v3d::asset::Json>("vgui",
            v3d::asset::Type::JsonDocument,
            boost::json::parse(R"({ "themes": [], "containers": [ { "name": "hud", "visible": true, "components": [] } ] })").as_object())));
        container = ui->container("hud");
        BOOST_REQUIRE(container);
        keys = boost::make_shared<v3d::ui::Keys>(ui, dispatcher);
    }

    void receive(const v3d::event::Event& event) {
        sent.push_back(event.str());
    }

    boost::shared_ptr<entt::dispatcher> dispatcher;
    boost::shared_ptr<v3d::event::Context> context;
    std::vector<std::string> sent;
    boost::shared_ptr<v3d::ui::Engine> ui;
    boost::shared_ptr<v3d::ui::Container> container;
    boost::shared_ptr<v3d::ui::Keys> keys;
};

boost::shared_ptr<v3d::ui::component::Button> button(Fixture* fixture, const std::string& name) {
    boost::shared_ptr<v3d::ui::component::Button> made =
        boost::make_shared<v3d::ui::component::Button>();
    made->name(name);
    made->event(v3d::event::Event(name, fixture->context));
    return made;
}

};  // namespace

BOOST_AUTO_TEST_SUITE(keyboard_drive_test)

/**
 * A control asks for the press and the focus in its own constructor, the way a text box does.
 *
 * A screen holding nothing focusable is a screen a tab cannot move through, so a component
 * that answers a click and does not ask for the focus is one that needs a mouse.
 **/
BOOST_AUTO_TEST_CASE(a_control_is_focusable_and_pickable_without_being_asked) {
    BOOST_CHECK(boost::make_shared<v3d::ui::component::Button>()->focusable());
    BOOST_CHECK(boost::make_shared<v3d::ui::component::CheckBox>()->focusable());
    BOOST_CHECK(boost::make_shared<v3d::ui::component::RadioButton>()->focusable());
    BOOST_CHECK(boost::make_shared<v3d::ui::component::SelectList>()->focusable());
    BOOST_CHECK(boost::make_shared<v3d::ui::component::TabBar>()->focusable());
    BOOST_CHECK(boost::make_shared<v3d::ui::component::TextBox>()->focusable());

    BOOST_CHECK(boost::make_shared<v3d::ui::component::Button>()->pickable());
    BOOST_CHECK(boost::make_shared<v3d::ui::component::CheckBox>()->pickable());

    // a panel is what a control is not: laid over a scene, and taking neither
    BOOST_CHECK(!boost::make_shared<v3d::ui::component::Panel>()->focusable());
    BOOST_CHECK(!boost::make_shared<v3d::ui::component::Panel>()->pickable());
}

/**
 * focusFirst() is how a screen says it is keyboard driven.
 *
 * focusNext() leaves a ui with nothing focused alone on purpose, so a press was the only
 * thing that ever gave out a first focus - which is exactly the mouse a keyboard driven
 * screen does not have.
 **/
BOOST_AUTO_TEST_CASE(a_screen_is_started_off_by_focusing_its_first_control) {
    Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::Button> first = button(&fixture, "one");
    const boost::shared_ptr<v3d::ui::component::Button> second = button(&fixture, "two");
    fixture.container->add(first);
    fixture.container->add(second);

    BOOST_CHECK(!fixture.ui->focused());
    // nothing is focused, so tab has nowhere to go from
    BOOST_CHECK(!fixture.keys->press("tab"));

    BOOST_CHECK(fixture.ui->focusFirst());
    BOOST_CHECK_EQUAL(fixture.ui->focused(), first);
    BOOST_CHECK(fixture.keys->press("tab"));
    BOOST_CHECK_EQUAL(fixture.ui->focused(), second);
}

/**
 * A ui holding nothing focusable stays as it was, which is the hud ADR-0040 is protecting.
 **/
BOOST_AUTO_TEST_CASE(a_screen_with_nothing_focusable_is_left_alone) {
    Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::Panel> plate =
        boost::make_shared<v3d::ui::component::Panel>();
    fixture.container->add(plate);

    BOOST_CHECK(!fixture.ui->focusFirst());
    BOOST_CHECK(!fixture.ui->focused());
}

/**
 * A return and a space send a focused button's command - the same command a click sends,
 * because both routers ask ui::command() for it.
 **/
BOOST_AUTO_TEST_CASE(a_return_and_a_space_activate_a_button) {
    Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::Button> start = button(&fixture, "start");
    fixture.container->add(start);
    fixture.ui->focus(start);

    BOOST_CHECK(fixture.keys->press("return"));
    BOOST_CHECK(fixture.keys->press("space"));
    BOOST_REQUIRE_EQUAL(fixture.sent.size(), 2U);
    BOOST_CHECK_EQUAL(fixture.sent[0], "test::start");
    BOOST_CHECK_EQUAL(fixture.sent[1], "test::start");
}

/**
 * A letter reaching a focused button is not being typed, so it goes on to the app's bindings.
 *
 * The opposite of a text box, which takes every key that composes text so that typing "w"
 * does not also walk the player. A button that swallowed the same key would stop a game
 * being played for as long as anything was focused, which is most of a menu screen.
 **/
BOOST_AUTO_TEST_CASE(a_letter_reaching_a_button_still_reaches_the_game) {
    Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::Button> start = button(&fixture, "start");
    fixture.container->add(start);
    fixture.ui->focus(start);

    BOOST_CHECK(!fixture.keys->press("w"));
    BOOST_CHECK(!fixture.keys->press("f3"));
    BOOST_CHECK(fixture.sent.empty());

    // and the same letter reaching a box is taken, which is the distinction
    const boost::shared_ptr<v3d::ui::component::TextBox> field =
        boost::make_shared<v3d::ui::component::TextBox>();
    fixture.container->add(field);
    fixture.ui->focus(field);
    BOOST_CHECK(fixture.keys->press("w"));
}

/**
 * A check box sends its command and marks nothing: whatever answers the command sets
 * checked(), per ADR-0019. A key activating one must not shortcut that, or a box driven by
 * the keyboard would show a state a box driven by the mouse does not.
 **/
BOOST_AUTO_TEST_CASE(a_key_does_not_mark_a_check_box_it_activates) {
    Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::CheckBox> box =
        boost::make_shared<v3d::ui::component::CheckBox>();
    box->event(v3d::event::Event("toggle", fixture.context));
    fixture.container->add(box);
    fixture.ui->focus(box);

    BOOST_CHECK(fixture.keys->press("space"));
    BOOST_REQUIRE_EQUAL(fixture.sent.size(), 1U);
    BOOST_CHECK_EQUAL(fixture.sent.front(), "test::toggle");
    BOOST_CHECK(!box->checked());
}

/**
 * The arrows step through a list's rows and send its command, the way clicking a row does.
 *
 * A list owns which row is chosen - the exception ADR-0019 names - so the router moves the
 * selection and then sends, rather than sending and waiting to be told.
 **/
BOOST_AUTO_TEST_CASE(the_arrows_step_through_a_list) {
    Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::SelectList> list =
        boost::make_shared<v3d::ui::component::SelectList>();
    list->items({"red", "green", "blue"});
    list->event(v3d::event::Event("chose", fixture.context));
    fixture.container->add(list);
    fixture.ui->focus(list);

    // nothing chosen steps onto the first row, so one arrow reaches a list nobody clicked in
    BOOST_CHECK_EQUAL(list->selected(), v3d::ui::component::SelectList::none);
    BOOST_CHECK(fixture.keys->press("arrow_down"));
    BOOST_CHECK_EQUAL(list->selected(), 0);
    BOOST_CHECK(fixture.keys->press("arrow_down"));
    BOOST_CHECK_EQUAL(list->selected(), 1);
    BOOST_CHECK(fixture.keys->press("arrow_up"));
    BOOST_CHECK_EQUAL(list->selected(), 0);
    BOOST_CHECK_EQUAL(fixture.sent.size(), 3U);
    BOOST_CHECK_EQUAL(fixture.sent.front(), "test::chose");

    BOOST_CHECK(fixture.keys->press("end"));
    BOOST_CHECK_EQUAL(list->selected(), 2);
    BOOST_CHECK(fixture.keys->press("home"));
    BOOST_CHECK_EQUAL(list->selected(), 0);
}

/**
 * A list does not wrap. Running off the end is how a keyboard reaches the last row and stays
 * there; a wrap would take the user back to the top instead, which is not what an arrow means
 * in a list. The key that moved nothing is not taken, so it goes on to the app.
 **/
BOOST_AUTO_TEST_CASE(a_list_does_not_wrap_at_either_end) {
    Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::SelectList> list =
        boost::make_shared<v3d::ui::component::SelectList>();
    list->items({"red", "green"});
    fixture.container->add(list);
    fixture.ui->focus(list);

    list->selected(1);
    BOOST_CHECK(!fixture.keys->press("arrow_down"));
    BOOST_CHECK_EQUAL(list->selected(), 1);

    list->selected(0);
    BOOST_CHECK(!fixture.keys->press("arrow_up"));
    BOOST_CHECK_EQUAL(list->selected(), 0);
}

/**
 * The arrows change which tab page is up. A bar carries no command, so nothing is sent -
 * which is what a click on a tab does too.
 **/
BOOST_AUTO_TEST_CASE(the_arrows_turn_the_pages_of_a_tab_bar) {
    Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::TabBar> bar =
        boost::make_shared<v3d::ui::component::TabBar>();
    bar->add(boost::make_shared<v3d::ui::component::TabPage>());
    bar->add(boost::make_shared<v3d::ui::component::TabPage>());
    fixture.container->add(bar);
    fixture.ui->focus(bar);

    BOOST_CHECK_EQUAL(bar->selected(), 0);
    BOOST_CHECK(fixture.keys->press("arrow_right"));
    BOOST_CHECK_EQUAL(bar->selected(), 1);
    BOOST_CHECK(fixture.keys->press("arrow_left"));
    BOOST_CHECK_EQUAL(bar->selected(), 0);

    // a bar has no command, so a return has nothing to send and is left for the page
    BOOST_CHECK(!fixture.keys->press("return"));
    BOOST_CHECK(fixture.sent.empty());
}

BOOST_AUTO_TEST_SUITE_END()
