/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/asset/kind/Json.h>
#include <api/render/realtime/Canvas.h>
#include <api/ui/Container.h>
#include <api/ui/Engine.h>
#include <api/ui/component/Panel.h>
#include <api/ui/component/TextBox.h>
#include <api/ui/input/Cursor.h>
#include <api/ui/input/Keys.h>
#include <api/ui/paint/ComponentRenderer.h>

#include <cstddef>
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
 * A fixed width per byte, so where a character falls is arithmetic rather than a font. The
 * cursor and the renderer are given the same one, which is what makes a click land where the
 * caret is drawn.
 **/
float measured(std::string_view text) {
    return static_cast<float>(text.size()) * characterWidth;
}

/**
 * A ui engine holding one container, the renderer that places what it holds, and both
 * routers - the cursor that gives the focus and the keys that follow it.
 **/
struct Fixture final {
    Fixture() :
        dispatcher(boost::make_shared<entt::dispatcher>()),
        context(boost::make_shared<v3d::event::Context>("test")),
        renderer(&measured, [](std::string_view, const glm::vec2&, const glm::vec4&) {}) {
        dispatcher->sink<v3d::event::Event>().connect<&Fixture::receive>(*this);
        canvas.resize(800, 600);

        ui = boost::make_shared<v3d::ui::Engine>(
            boost::make_shared<v3d::event::Engine>(dispatcher), dispatcher,
            boost::make_shared<v3d::log::Logger>());
        BOOST_REQUIRE(ui->load(boost::make_shared<v3d::asset::kind::Json>("vgui", v3d::asset::Type::JsonDocument,
            boost::json::parse(R"({ "themes": [], "containers": [ { "name": "hud", "visible": true, "components": [] } ] })").as_object())));
        container = ui->container("hud");
        BOOST_REQUIRE(container);
        cursor = boost::make_shared<v3d::ui::input::Cursor>(ui, dispatcher, &measured);

        v3d::ui::input::Keys::Clipboard board;
        board.read = [this]() { return clipboard; };
        board.write = [this](std::string_view text) { clipboard = std::string(text); };
        keys = boost::make_shared<v3d::ui::input::Keys>(ui, dispatcher, board);
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
    std::string clipboard;  /**< what a cut or a copy handed over, and what a paste reads **/
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
    BOOST_REQUIRE(fixture.ui->load(boost::make_shared<v3d::asset::kind::Json>("vgui", v3d::asset::Type::JsonDocument,
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

/**
 * A selection is the run between the anchor and the caret, and nothing is selected exactly
 * when the two are in the same place - ADR-0057. There is no third piece of state, so a
 * selection cannot point into text that has been retyped.
 **/
BOOST_AUTO_TEST_CASE(a_selection_is_the_run_between_the_anchor_and_the_caret) {
    const boost::shared_ptr<v3d::ui::component::TextBox> field = box("abcde");
    BOOST_CHECK(!field->selected());
    BOOST_CHECK_EQUAL(field->anchor(), field->caret());
    BOOST_CHECK(field->selection().empty());

    field->select(1, 4);
    BOOST_CHECK(field->selected());
    BOOST_CHECK_EQUAL(field->selection(), "bcd");
    // the caret is left at the end a shift and an arrow moves
    BOOST_CHECK_EQUAL(field->caret(), 4U);
    BOOST_CHECK_EQUAL(field->anchor(), 1U);

    field->deselect();
    BOOST_CHECK(!field->selected());
    BOOST_CHECK_EQUAL(field->caret(), 4U);

    field->selectAll();
    BOOST_CHECK_EQUAL(field->selection(), "abcde");
}

/**
 * An arrow with nothing held lands on an end of the selection rather than a character past
 * it, which is what a first arrow out of a selected run means everywhere else.
 **/
BOOST_AUTO_TEST_CASE(an_arrow_out_of_a_selection_lands_on_its_edge) {
    const boost::shared_ptr<v3d::ui::component::TextBox> field = box("abcde");

    field->select(1, 4);
    BOOST_CHECK(field->left());
    BOOST_CHECK_EQUAL(field->caret(), 1U);
    BOOST_CHECK(!field->selected());

    field->select(1, 4);
    BOOST_CHECK(field->right());
    BOOST_CHECK_EQUAL(field->caret(), 4U);
    BOOST_CHECK(!field->selected());

    // and the anchor stays put when the arrow is shifted, which is what selects
    field->caret(2);
    BOOST_CHECK(field->right(true));
    BOOST_CHECK(field->right(true));
    BOOST_CHECK_EQUAL(field->selection(), "cd");
    BOOST_CHECK_EQUAL(field->anchor(), 2U);
}

/**
 * Typing over a selected run replaces it, and so does a backspace, a delete and a paste -
 * every one of them through removeSelection(), so there is one answer rather than four.
 **/
BOOST_AUTO_TEST_CASE(an_edit_over_a_selection_replaces_the_run) {
    const boost::shared_ptr<v3d::ui::component::TextBox> typed = box("abcde");
    typed->select(1, 4);
    BOOST_CHECK(typed->insert("X"));
    BOOST_CHECK_EQUAL(typed->text(), "aXe");
    BOOST_CHECK_EQUAL(typed->caret(), 2U);
    BOOST_CHECK(!typed->selected());

    const boost::shared_ptr<v3d::ui::component::TextBox> erased = box("abcde");
    erased->select(1, 4);
    BOOST_CHECK(erased->backspace());
    BOOST_CHECK_EQUAL(erased->text(), "ae");
    BOOST_CHECK_EQUAL(erased->caret(), 1U);

    const boost::shared_ptr<v3d::ui::component::TextBox> deleted = box("abcde");
    deleted->select(1, 4);
    BOOST_CHECK(deleted->erase());
    BOOST_CHECK_EQUAL(deleted->text(), "ae");
    BOOST_CHECK_EQUAL(deleted->caret(), 1U);
}

/**
 * The limit is measured against what the text would become, so a paste may be as long as the
 * run it replaces plus whatever room was left - and one byte longer than that is refused
 * whole, the way it always was.
 **/
BOOST_AUTO_TEST_CASE(a_selection_makes_room_for_what_replaces_it) {
    const boost::shared_ptr<v3d::ui::component::TextBox> field = box("abcde");
    field->limit(5);
    BOOST_CHECK(!field->insert("f"));

    field->select(1, 4);
    BOOST_CHECK(field->insert("XYZ"));
    BOOST_CHECK_EQUAL(field->text(), "aXYZe");

    field->select(1, 4);
    BOOST_CHECK(!field->insert("WXYZ"));
    BOOST_CHECK_EQUAL(field->text(), "aXYZe");
}

/**
 * A selection is moved to a character boundary like everything else, so a multi-byte
 * character is selected, copied and replaced whole.
 **/
BOOST_AUTO_TEST_CASE(a_selection_lands_on_character_boundaries) {
    // "aeb", with a two byte e acute in the middle
    const boost::shared_ptr<v3d::ui::component::TextBox> field = box("aéb");
    BOOST_CHECK_EQUAL(field->text().size(), 4U);

    // an offset inside the character is not expressible, so the run starts before it
    field->select(2, 3);
    BOOST_CHECK_EQUAL(field->anchor(), 1U);
    BOOST_CHECK_EQUAL(field->selection(), "é");

    BOOST_CHECK(field->insert("e"));
    BOOST_CHECK_EQUAL(field->text(), "aeb");
}

/**
 * A press puts the caret where it landed, which is what ADR-0057 gave ui::Cursor a Measure
 * for. It is answered against the pen the last draw left on the box, so a ui routed before it
 * is drawn places nothing - the same rule as picking one, per ADR-0019.
 **/
BOOST_AUTO_TEST_CASE(a_press_puts_the_caret_where_it_landed) {
    Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::TextBox> field = box("abcde");
    fixture.place(field, glm::vec2(0.0f, 0.0f), glm::vec2(200.0f, 24.0f));
    fixture.draw();

    const float pen = field->pen();
    BOOST_CHECK(fixture.cursor->press(glm::vec2(pen + 22.0f, 12.0f)));
    BOOST_CHECK_EQUAL(field->caret(), 2U);
    BOOST_CHECK(!field->selected());
    BOOST_CHECK(fixture.cursor->release(glm::vec2(pen + 22.0f, 12.0f)));

    // left of the first character is the start of the text, and past the last is the end
    BOOST_CHECK(fixture.cursor->press(glm::vec2(pen - 5.0f, 12.0f)));
    BOOST_CHECK_EQUAL(field->caret(), 0U);
    fixture.cursor->release(glm::vec2(pen - 5.0f, 12.0f));

    BOOST_CHECK(fixture.cursor->press(glm::vec2(pen + 100.0f, 12.0f)));
    BOOST_CHECK_EQUAL(field->caret(), 5U);
}

/**
 * A press leaves the anchor where it landed, so following the cursor selects the run between
 * the two - which is what makes a drag a selection rather than a caret being dragged.
 **/
BOOST_AUTO_TEST_CASE(a_drag_selects_the_run_it_crosses) {
    Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::TextBox> field = box("abcde");
    fixture.place(field, glm::vec2(0.0f, 0.0f), glm::vec2(200.0f, 24.0f));
    fixture.draw();

    const float pen = field->pen();
    BOOST_CHECK(fixture.cursor->press(glm::vec2(pen + 2.0f, 12.0f)));
    BOOST_CHECK_EQUAL(field->caret(), 0U);

    BOOST_CHECK(fixture.cursor->motion(glm::vec2(pen + 32.0f, 12.0f)));
    BOOST_CHECK_EQUAL(field->selection(), "abc");
    BOOST_CHECK_EQUAL(field->anchor(), 0U);

    // and the release settles it where the cursor ended rather than dropping the run
    BOOST_CHECK(fixture.cursor->release(glm::vec2(pen + 22.0f, 12.0f)));
    BOOST_CHECK_EQUAL(field->selection(), "ab");
}

/**
 * A cursor given no Measure names no text, so a press focuses the box and leaves the caret
 * where it was - which is every caller's behaviour before one could be given.
 **/
BOOST_AUTO_TEST_CASE(a_cursor_with_no_measure_leaves_the_caret_alone) {
    Fixture fixture;
    const boost::shared_ptr<v3d::ui::input::Cursor> blind =
        boost::make_shared<v3d::ui::input::Cursor>(fixture.ui, fixture.dispatcher);
    const boost::shared_ptr<v3d::ui::component::TextBox> field = box("abcde");
    fixture.place(field, glm::vec2(0.0f, 0.0f), glm::vec2(200.0f, 24.0f));
    fixture.draw();

    field->caret(1);
    BOOST_CHECK(blind->press(glm::vec2(field->pen() + 42.0f, 12.0f)));
    BOOST_CHECK_EQUAL(fixture.ui->focused(), field);
    BOOST_CHECK_EQUAL(field->caret(), 1U);
}

/**
 * Cut, copy, paste and select all, over the clipboard the app hands in - api/ui names no SDL
 * type, so the two calls come from outside it, per ADR-0057.
 **/
BOOST_AUTO_TEST_CASE(the_chords_cut_copy_and_paste_the_selection) {
    Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::TextBox> field = box("abcde");
    fixture.container->add(field);
    fixture.ui->focus(field);

    field->select(1, 4);
    BOOST_CHECK(fixture.keys->press("c", false, true));
    BOOST_CHECK_EQUAL(fixture.clipboard, "bcd");
    BOOST_CHECK_EQUAL(field->text(), "abcde");

    BOOST_CHECK(fixture.keys->press("x", false, true));
    BOOST_CHECK_EQUAL(fixture.clipboard, "bcd");
    BOOST_CHECK_EQUAL(field->text(), "ae");
    BOOST_CHECK_EQUAL(field->caret(), 1U);

    BOOST_CHECK(fixture.keys->press("v", false, true));
    BOOST_CHECK_EQUAL(field->text(), "abcde");
    BOOST_CHECK_EQUAL(field->caret(), 4U);

    BOOST_CHECK(fixture.keys->press("a", false, true));
    BOOST_CHECK_EQUAL(field->selection(), "abcde");
    // and a paste over the whole of it replaces it
    BOOST_CHECK(fixture.keys->press("v", false, true));
    BOOST_CHECK_EQUAL(field->text(), "bcd");
}

/**
 * A cut with nowhere to hand the run does not take it out, because a cut that loses the text
 * is worse than one that did not happen - and a paste with nothing to read puts nothing in.
 **/
BOOST_AUTO_TEST_CASE(a_router_with_no_clipboard_still_edits_everything_else) {
    Fixture fixture;
    const boost::shared_ptr<v3d::ui::input::Keys> alone =
        boost::make_shared<v3d::ui::input::Keys>(fixture.ui, fixture.dispatcher);
    const boost::shared_ptr<v3d::ui::component::TextBox> field = box("abcde");
    fixture.container->add(field);
    fixture.ui->focus(field);

    field->select(1, 4);
    BOOST_CHECK(alone->press("x", false, true));
    BOOST_CHECK_EQUAL(field->text(), "abcde");
    BOOST_CHECK(alone->press("v", false, true));
    BOOST_CHECK_EQUAL(field->text(), "abcde");

    // select all needs no clipboard, so it still answers
    BOOST_CHECK(alone->press("a", false, true));
    BOOST_CHECK_EQUAL(field->selection(), "abcde");
}

/**
 * A chord the box does not answer goes on to the app, so a ctrl-s still saves while somebody
 * is typing. Only a control that eats every key can stop an app being driven.
 **/
BOOST_AUTO_TEST_CASE(a_chord_the_box_does_not_answer_reaches_the_app) {
    Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::TextBox> field = box("abcde");
    fixture.container->add(field);
    fixture.ui->focus(field);

    BOOST_CHECK(!fixture.keys->press("s", false, true));
    BOOST_CHECK(!fixture.keys->press("z", false, true));
    // and the letter is taken when no chord is held, because it is being typed
    BOOST_CHECK(fixture.keys->press("s"));
}

/**
 * Shift and a caret key selects the run it travelled, which is the keyboard's half of a drag.
 **/
BOOST_AUTO_TEST_CASE(shift_and_a_caret_key_selects) {
    Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::TextBox> field = box("abcde");
    fixture.container->add(field);
    fixture.ui->focus(field);

    field->home();
    BOOST_CHECK(fixture.keys->press("arrow_right", true));
    BOOST_CHECK(fixture.keys->press("arrow_right", true));
    BOOST_CHECK_EQUAL(field->selection(), "ab");

    BOOST_CHECK(fixture.keys->press("end", true));
    BOOST_CHECK_EQUAL(field->selection(), "abcde");

    // unshifted, the same key drops the selection
    BOOST_CHECK(fixture.keys->press("home"));
    BOOST_CHECK(!field->selected());
}

/**
 * The selected run is drawn behind the line, so it can be seen: one more quad than the same
 * box with nothing selected, and no batch of its own - the box's own clip is what splits the
 * batches, and the highlight is drawn inside the one the text is.
 **/
BOOST_AUTO_TEST_CASE(the_selected_run_is_drawn_behind_the_text) {
    Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::TextBox> field = box("abcde");
    field->focused(true);
    fixture.place(field, glm::vec2(0.0f, 0.0f), glm::vec2(200.0f, 24.0f));
    fixture.draw();
    const std::size_t plain = fixture.canvas.vertices().size();
    const std::size_t batches = fixture.canvas.batches().size();

    field->select(1, 4);
    fixture.draw();
    BOOST_CHECK_EQUAL(fixture.canvas.vertices().size(), plain + 4U);
    BOOST_CHECK_EQUAL(fixture.canvas.batches().size(), batches);
}

BOOST_AUTO_TEST_SUITE_END()
