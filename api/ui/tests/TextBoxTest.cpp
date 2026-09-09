/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/asset/Json.h>
#include <api/render/realtime/Canvas.h>
#include <api/ui/Container.h>
#include <api/ui/Engine.h>
#include <api/ui/component/Panel.h>
#include <api/ui/component/TextBox.h>
#include <api/ui/input/Cursor.h>
#include <api/ui/input/Keys.h>
#include <api/ui/paint/ComponentRenderer.h>

#include <string>
#include <string_view>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <boost/json/parse.hpp>
#include <boost/make_shared.hpp>
#include <entt/entt.hpp>

namespace {

const float characterWidth = 10.0f;

/**
 * A ui engine holding one container, the renderer that places what it holds, and both
 * routers - the cursor that gives the focus and the keys that follow it.
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
        BOOST_REQUIRE(ui->load(boost::make_shared<v3d::asset::Json>("vgui", v3d::asset::Type::JsonDocument,
            boost::json::parse(R"({ "themes": [], "containers": [ { "name": "hud", "visible": true, "components": [] } ] })").as_object())));
        container = ui->container("hud");
        BOOST_REQUIRE(container);
        cursor = boost::make_shared<v3d::ui::input::Cursor>(ui, dispatcher);
        keys = boost::make_shared<v3d::ui::input::Keys>(ui, dispatcher);
    }

    void receive(const v3d::event::Event& event) {
        sent.push_back(event.str());
    }

    void place(const boost::shared_ptr<v3d::ui::Component>& component, const glm::vec2& corner,
        const glm::vec2& size) {
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
    v3d::ui::paint::ComponentRenderer renderer;
    boost::shared_ptr<v3d::ui::Engine> ui;
    boost::shared_ptr<v3d::ui::Container> container;
    boost::shared_ptr<v3d::ui::input::Cursor> cursor;
    boost::shared_ptr<v3d::ui::input::Keys> keys;
};

boost::shared_ptr<v3d::ui::component::TextBox> box(const std::string& text) {
    boost::shared_ptr<v3d::ui::component::TextBox> made =
        boost::make_shared<v3d::ui::component::TextBox>();
    made->text(text);
    return made;
}

};  // namespace

BOOST_AUTO_TEST_SUITE(textbox_test)

/**
 * Setting the text leaves the caret after it, and typing puts characters in where the caret
 * is rather than at the end.
 **/
BOOST_AUTO_TEST_CASE(typing_goes_in_at_the_caret) {
    const boost::shared_ptr<v3d::ui::component::TextBox> field = box("ac");
    BOOST_CHECK_EQUAL(field->caret(), 2U);

    field->left();
    BOOST_CHECK(field->insert("b"));
    BOOST_CHECK_EQUAL(field->text(), "abc");
    BOOST_CHECK_EQUAL(field->caret(), 2U);
}

/**
 * A backspace takes out what is behind the caret and a delete takes out what is in front of
 * it, and neither runs off the end of the text.
 **/
BOOST_AUTO_TEST_CASE(backspace_and_delete_take_out_one_character_each) {
    const boost::shared_ptr<v3d::ui::component::TextBox> field = box("abc");

    BOOST_CHECK(field->backspace());
    BOOST_CHECK_EQUAL(field->text(), "ab");

    field->home();
    BOOST_CHECK(!field->backspace());
    BOOST_CHECK(field->erase());
    BOOST_CHECK_EQUAL(field->text(), "b");

    field->end();
    BOOST_CHECK(!field->erase());
}

/**
 * The caret is a byte offset into utf-8 and every move lands on a character boundary, so a
 * multi-byte character is stepped over and erased whole rather than cut in half.
 **/
BOOST_AUTO_TEST_CASE(the_caret_moves_by_characters_not_by_bytes) {
    // "e" with an acute accent is two bytes, and the euro sign is three
    const boost::shared_ptr<v3d::ui::component::TextBox> field = box("a\xC3\xA9\xE2\x82\xACz");
    BOOST_CHECK_EQUAL(field->caret(), 7U);

    field->left();
    BOOST_CHECK_EQUAL(field->caret(), 6U);
    field->left();
    BOOST_CHECK_EQUAL(field->caret(), 3U);
    field->left();
    BOOST_CHECK_EQUAL(field->caret(), 1U);

    // and a backspace over the accented character takes both its bytes
    field->end();
    field->left();
    BOOST_CHECK(field->backspace());
    BOOST_CHECK_EQUAL(field->text(), "a\xC3\xA9z");

    // an offset asked for inside a character comes back on the boundary before it
    field->caret(2U);
    BOOST_CHECK_EQUAL(field->caret(), 1U);
}

/**
 * A limit refuses an insertion whole rather than truncating it, because half a pasted path
 * is worse than none of it and cutting utf-8 by bytes can split a character.
 **/
BOOST_AUTO_TEST_CASE(a_limit_refuses_an_insertion_whole) {
    const boost::shared_ptr<v3d::ui::component::TextBox> field = box("abc");
    field->limit(5U);

    BOOST_CHECK(field->insert("de"));
    BOOST_CHECK_EQUAL(field->text(), "abcde");
    BOOST_CHECK(!field->insert("f"));
    BOOST_CHECK_EQUAL(field->text(), "abcde");
}

/**
 * A press moves the keyboard onto what it lands on and takes it off what it does not, which
 * is what makes clicking into a box mean "type here" - ADR-0040.
 **/
BOOST_AUTO_TEST_CASE(a_press_gives_and_takes_the_focus) {
    Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::TextBox> field = box("");
    fixture.place(field, glm::vec2(100.0f, 50.0f), glm::vec2(200.0f, 30.0f));
    const boost::shared_ptr<v3d::ui::component::Panel> plate =
        boost::make_shared<v3d::ui::component::Panel>();
    plate->pickable(true);
    fixture.place(plate, glm::vec2(0.0f, 200.0f), glm::vec2(200.0f, 100.0f));
    fixture.draw();

    BOOST_CHECK(fixture.cursor->press(glm::vec2(150.0f, 60.0f)));
    BOOST_CHECK_EQUAL(fixture.ui->focused(), field);
    BOOST_CHECK(field->focused());

    // a press on something that never asked to be focusable takes the keyboard off rather
    // than moving it onto that component
    BOOST_CHECK(fixture.cursor->press(glm::vec2(50.0f, 250.0f)));
    BOOST_CHECK(!fixture.ui->focused());
    BOOST_CHECK(!field->focused());

    // and so does a press on nothing at all
    fixture.cursor->press(glm::vec2(150.0f, 60.0f));
    BOOST_REQUIRE(fixture.ui->focused());
    fixture.cursor->press(glm::vec2(700.0f, 500.0f));
    BOOST_CHECK(!fixture.ui->focused());
}

/**
 * A key reaches the focused component and nothing else, so a ui with nothing focused leaves
 * every key to the app - which is what keeps a game's movement keys working.
 **/
BOOST_AUTO_TEST_CASE(a_key_goes_to_the_focused_component_or_to_nobody) {
    Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::TextBox> field = box("ab");
    fixture.place(field, glm::vec2(100.0f, 50.0f), glm::vec2(200.0f, 30.0f));
    fixture.draw();

    BOOST_CHECK(!fixture.keys->press("backspace"));
    BOOST_CHECK(!fixture.keys->text("c"));
    BOOST_CHECK_EQUAL(field->text(), "ab");

    fixture.cursor->press(glm::vec2(150.0f, 60.0f));

    BOOST_CHECK(fixture.keys->text("c"));
    BOOST_CHECK_EQUAL(field->text(), "abc");
    BOOST_CHECK(fixture.keys->press("backspace"));
    BOOST_CHECK_EQUAL(field->text(), "ab");
    BOOST_CHECK(fixture.keys->press("arrow_left"));
    BOOST_CHECK_EQUAL(field->caret(), 1U);
}

/**
 * A key that will arrive again as a character is taken while a box has the focus, so typing
 * "w" into one does not also walk the player forward. A key that composes nothing is left
 * for the app.
 **/
BOOST_AUTO_TEST_CASE(a_character_key_is_taken_and_a_command_key_is_not) {
    Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::TextBox> field = box("");
    fixture.place(field, glm::vec2(100.0f, 50.0f), glm::vec2(200.0f, 30.0f));
    fixture.draw();
    fixture.cursor->press(glm::vec2(150.0f, 60.0f));

    BOOST_CHECK(fixture.keys->press("w"));
    BOOST_CHECK(fixture.keys->press("space"));
    // the key itself typed nothing - the character comes separately
    BOOST_CHECK(field->text().empty());

    BOOST_CHECK(!fixture.keys->press("f3"));
    BOOST_CHECK(!fixture.keys->press("left_shift"));
    BOOST_CHECK(!fixture.keys->press("pageup"));
}

/**
 * A return says the user is done: the box sends its command and whatever answers reads the
 * text, per ADR-0038. An escape leaves the box, which is the only way out of one with no
 * other ui to click on.
 **/
BOOST_AUTO_TEST_CASE(a_return_sends_the_command_and_an_escape_leaves_the_box) {
    Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::TextBox> field = box("hello");
    field->event(v3d::event::Event("submit", fixture.context));
    fixture.place(field, glm::vec2(100.0f, 50.0f), glm::vec2(200.0f, 30.0f));
    fixture.draw();
    fixture.cursor->press(glm::vec2(150.0f, 60.0f));

    BOOST_CHECK(fixture.keys->press("return"));
    BOOST_REQUIRE_EQUAL(fixture.sent.size(), 1U);
    BOOST_CHECK_EQUAL(fixture.sent.front(), "test::submit");
    BOOST_CHECK_EQUAL(field->text(), "hello");

    BOOST_CHECK(fixture.keys->press("escape"));
    BOOST_CHECK(!fixture.ui->focused());
    BOOST_CHECK(!fixture.keys->press("backspace"));
}

/**
 * One component has the keyboard at a time, so focusing a second box takes it off the first.
 **/
BOOST_AUTO_TEST_CASE(only_one_component_is_focused) {
    Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::TextBox> first = box("");
    const boost::shared_ptr<v3d::ui::component::TextBox> second = box("");
    fixture.place(first, glm::vec2(0.0f, 0.0f), glm::vec2(200.0f, 30.0f));
    fixture.place(second, glm::vec2(0.0f, 100.0f), glm::vec2(200.0f, 30.0f));
    fixture.draw();

    fixture.cursor->press(glm::vec2(50.0f, 10.0f));
    BOOST_CHECK(first->focused());
    fixture.cursor->press(glm::vec2(50.0f, 110.0f));
    BOOST_CHECK(!first->focused());
    BOOST_CHECK(second->focused());
    BOOST_CHECK_EQUAL(fixture.ui->focused(), second);
}

/**
 * A box is as wide as the room it is in and as tall as the line it holds, so what has been
 * typed into it does not change its size.
 **/
BOOST_AUTO_TEST_CASE(a_box_is_sized_by_its_room_and_not_by_its_text) {
    Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::TextBox> field = box("");
    fixture.container->add(field);
    fixture.draw();
    const glm::vec2 empty = field->size();

    field->text("a very long line of text indeed");
    fixture.draw();

    BOOST_CHECK_CLOSE(field->size().x, empty.x, 0.001f);
    BOOST_CHECK_CLOSE(field->size().y, empty.y, 0.001f);
    BOOST_CHECK_CLOSE(field->size().x, 800.0f, 0.001f);
}

/**
 * A box loaded from a config is pickable and focusable without saying so, because a box
 * exists to be typed into - unlike a panel, which must not take a press off the scene.
 **/
BOOST_AUTO_TEST_CASE(a_loaded_box_asks_for_the_press_and_the_keyboard) {
    Fixture fixture;
    BOOST_REQUIRE(fixture.ui->load(boost::make_shared<v3d::asset::Json>("vgui", v3d::asset::Type::JsonDocument,
        boost::json::parse(R"({ "themes": [], "containers": [ { "name": "form", "visible": true, "components": [
            { "name": "search", "type": "textbox", "text": "abc", "placeholder": "Search", "limit": 8 } ] } ] })").as_object())));

    const boost::shared_ptr<v3d::ui::Container> form = fixture.ui->container("form");
    BOOST_REQUIRE(form);
    const boost::shared_ptr<v3d::ui::component::TextBox> field =
        boost::dynamic_pointer_cast<v3d::ui::component::TextBox>(form->get("search"));
    BOOST_REQUIRE(field);
    BOOST_CHECK_EQUAL(field->text(), "abc");
    BOOST_CHECK_EQUAL(field->placeholder(), "Search");
    BOOST_CHECK_EQUAL(field->limit(), 8U);
    BOOST_CHECK(field->pickable());
    BOOST_CHECK(field->focusable());
}

BOOST_AUTO_TEST_SUITE_END()
