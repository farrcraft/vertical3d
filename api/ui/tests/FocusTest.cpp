/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/asset/kind/Json.h>
#include <api/ui/Container.h>
#include <api/ui/Engine.h>
#include <api/ui/component/Panel.h>
#include <api/ui/component/TextBox.h>
#include <api/ui/component/VerticalBox.h>
#include <api/ui/input/Keys.h>

#include <string>
#include <string_view>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <boost/json/parse.hpp>
#include <boost/make_shared.hpp>
#include <entt/entt.hpp>

namespace {

/**
 * A ui engine with as many containers as a case wants, and nothing that draws: the
 * traversal reads what the tree holds rather than the boxes it was drawn in.
 **/
struct Fixture final {
    explicit Fixture(const std::string& document) :
        dispatcher(boost::make_shared<entt::dispatcher>()) {
        ui = boost::make_shared<v3d::ui::Engine>(
            boost::make_shared<v3d::event::Engine>(dispatcher), dispatcher,
            boost::make_shared<v3d::log::Logger>());
        BOOST_REQUIRE(ui->load(boost::make_shared<v3d::asset::kind::Json>("vgui",
            v3d::asset::Type::JsonDocument, boost::json::parse(document).as_object())));
        keys = boost::make_shared<v3d::ui::input::Keys>(ui, dispatcher);
    }

    boost::shared_ptr<entt::dispatcher> dispatcher;
    boost::shared_ptr<v3d::ui::Engine> ui;
    boost::shared_ptr<v3d::ui::input::Keys> keys;
};

const char* const ONE_CONTAINER =
R"({ "themes": [], "containers": [ { "name": "hud", "visible": true, "components": [] } ] })";

boost::shared_ptr<v3d::ui::component::TextBox> box(const std::string& name) {
    boost::shared_ptr<v3d::ui::component::TextBox> made =
        boost::make_shared<v3d::ui::component::TextBox>();
    made->name(name);
    return made;
}

boost::shared_ptr<v3d::ui::component::Panel> panel(const std::string& name) {
    boost::shared_ptr<v3d::ui::component::Panel> made =
        boost::make_shared<v3d::ui::component::Panel>();
    made->name(name);
    return made;
}

std::string focusedName(const boost::shared_ptr<v3d::ui::Engine>& ui) {
    const boost::shared_ptr<v3d::ui::Component> component = ui->focused();
    return component ? std::string(component->name()) : std::string();
}

};  // namespace

BOOST_AUTO_TEST_SUITE(focus_test)

/**
 * The focus walks the tree in document order and wraps at each end, skipping the components
 * that did not ask to be focusable. A TextBox is focusable and a Panel is not, which is the
 * mixture every real screen is.
 **/
BOOST_AUTO_TEST_CASE(the_focus_moves_forward_and_back_and_wraps) {
    Fixture fixture(ONE_CONTAINER);
    const boost::shared_ptr<v3d::ui::Container> hud = fixture.ui->container("hud");
    hud->add(box("first"));
    hud->add(panel("plate"));
    hud->add(box("second"));
    hud->add(box("third"));

    fixture.ui->focus(hud->get("first"));
    BOOST_REQUIRE_EQUAL(focusedName(fixture.ui), "first");

    // the panel is passed over rather than focused
    BOOST_CHECK(fixture.ui->focusNext(true));
    BOOST_CHECK_EQUAL(focusedName(fixture.ui), "second");
    BOOST_CHECK(fixture.ui->focusNext(true));
    BOOST_CHECK_EQUAL(focusedName(fixture.ui), "third");

    // and the end wraps round to the start
    BOOST_CHECK(fixture.ui->focusNext(true));
    BOOST_CHECK_EQUAL(focusedName(fixture.ui), "first");

    // backwards is the same walk the other way, and wraps at the other end
    BOOST_CHECK(fixture.ui->focusNext(false));
    BOOST_CHECK_EQUAL(focusedName(fixture.ui), "third");
    BOOST_CHECK(fixture.ui->focusNext(false));
    BOOST_CHECK_EQUAL(focusedName(fixture.ui), "second");
}

/**
 * A hidden subtree is skipped whole: a form on a tab page nobody is looking at must not be
 * reachable by tabbing through the page in front of it.
 **/
BOOST_AUTO_TEST_CASE(a_hidden_subtree_is_skipped) {
    Fixture fixture(ONE_CONTAINER);
    const boost::shared_ptr<v3d::ui::Container> hud = fixture.ui->container("hud");
    hud->add(box("first"));

    const boost::shared_ptr<v3d::ui::component::Panel> hidden = panel("hidden");
    hidden->visible(false);
    hidden->add(box("buried"));
    hud->add(hidden);
    hud->add(box("last"));

    fixture.ui->focus(hud->get("first"));
    BOOST_CHECK(fixture.ui->focusNext(true));
    BOOST_CHECK_EQUAL(focusedName(fixture.ui), "last");

    // and a component of its own that is hidden is not reachable either
    const boost::shared_ptr<v3d::ui::component::TextBox> unseen = box("unseen");
    unseen->visible(false);
    hud->add(unseen);
    BOOST_CHECK(fixture.ui->focusNext(true));
    BOOST_CHECK_EQUAL(focusedName(fixture.ui), "first");
}

/**
 * Depth is what orders the walk, and a flow box is the exception: it holds its children in
 * the order it places them, so a z index inside one changes nothing. This mirrors what
 * Arranger::walk does, and the two have to agree or the tab order is not the reading order.
 **/
BOOST_AUTO_TEST_CASE(the_order_is_the_order_things_are_drawn_in) {
    Fixture fixture(ONE_CONTAINER);
    const boost::shared_ptr<v3d::ui::Container> hud = fixture.ui->container("hud");

    const boost::shared_ptr<v3d::ui::component::TextBox> front = box("front");
    front->depth(5);
    hud->add(front);
    hud->add(box("back"));

    fixture.ui->focus(hud->get("back"));
    BOOST_CHECK(fixture.ui->focusNext(true));
    BOOST_CHECK_EQUAL(focusedName(fixture.ui), "front");

    // inside a flow box the order is the order it was given them, whatever the depths are
    const boost::shared_ptr<v3d::ui::component::VerticalBox> column =
        boost::make_shared<v3d::ui::component::VerticalBox>();
    column->name("column");
    const boost::shared_ptr<v3d::ui::component::TextBox> top = box("top");
    top->depth(9);
    column->add(top);
    column->add(box("bottom"));
    hud->add(column);

    fixture.ui->focus(hud->get("top"));
    BOOST_CHECK(fixture.ui->focusNext(true));
    BOOST_CHECK_EQUAL(focusedName(fixture.ui), "bottom");
}

/**
 * A ui with nothing focused is left alone. That is what keeps a game's movement keys
 * working: tab must not take the focus onto the first widget of a hud nobody is using.
 **/
BOOST_AUTO_TEST_CASE(nothing_focused_stays_nothing_focused) {
    Fixture fixture(ONE_CONTAINER);
    const boost::shared_ptr<v3d::ui::Container> hud = fixture.ui->container("hud");
    hud->add(box("first"));
    hud->add(box("second"));

    BOOST_CHECK(!fixture.ui->focused());
    BOOST_CHECK(!fixture.ui->focusNext(true));
    BOOST_CHECK(!fixture.ui->focused());

    // and a tab reaches the app's bindings rather than being swallowed
    BOOST_CHECK(!fixture.keys->press("tab"));

    // once something is focused the tab is the ui's, so it does not also reach a binding
    fixture.ui->focus(hud->get("first"));
    BOOST_CHECK(fixture.keys->press("tab"));
}

/**
 * Tab moves the focus and shift and tab moves it back, and both are taken so that neither
 * also reaches whatever the app bound to tab.
 **/
BOOST_AUTO_TEST_CASE(tab_moves_the_focus_and_is_taken) {
    Fixture fixture(ONE_CONTAINER);
    const boost::shared_ptr<v3d::ui::Container> hud = fixture.ui->container("hud");
    hud->add(box("first"));
    hud->add(box("second"));
    hud->add(box("third"));

    fixture.ui->focus(hud->get("first"));
    BOOST_CHECK(fixture.keys->press("tab"));
    BOOST_CHECK_EQUAL(focusedName(fixture.ui), "second");

    BOOST_CHECK(fixture.keys->press("tab", true));
    BOOST_CHECK_EQUAL(focusedName(fixture.ui), "first");

    // and a caller that never says whether shift is held gets forward only
    BOOST_CHECK(fixture.keys->press("tab", false));
    BOOST_CHECK_EQUAL(focusedName(fixture.ui), "second");
}

/**
 * A container that is not visible holds nothing the focus can reach, the same way a hidden
 * component does.
 **/
BOOST_AUTO_TEST_CASE(a_hidden_container_is_skipped) {
    Fixture fixture(R"({ "themes": [], "containers": [
        { "name": "hud", "visible": true, "components": [] },
        { "name": "menu", "visible": false, "components": [] } ] })");

    const boost::shared_ptr<v3d::ui::Container> hud = fixture.ui->container("hud");
    hud->add(box("first"));
    hud->add(box("second"));
    fixture.ui->container("menu")->add(box("buried"));

    fixture.ui->focus(hud->get("second"));
    BOOST_CHECK(fixture.ui->focusNext(true));
    BOOST_CHECK_EQUAL(focusedName(fixture.ui), "first");
}

BOOST_AUTO_TEST_SUITE_END()
