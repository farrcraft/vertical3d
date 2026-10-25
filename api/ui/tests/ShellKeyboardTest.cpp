/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/event/Context.h>
#include <api/event/Engine.h>
#include <api/event/Event.h>
#include <api/log/Logger.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/ui/Container.h>
#include <api/ui/Engine.h>
#include <api/ui/Immediate.h>
#include <api/ui/component/Button.h>
#include <api/ui/component/Panel.h>
#include <api/ui/component/TextBox.h>
#include <api/ui/shell/Keyboard.h>

#include <SDL3/SDL.h>

#include <string>
#include <string_view>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <boost/json/parse.hpp>
#include <boost/make_shared.hpp>
#include <entt/entt.hpp>

namespace {

/**
 * The keyboard adapter over a ui holding one container. No window: what the platform composes
 * is not what this tests, and an adapter given none still routes every key.
 **/
struct Fixture final {
    Fixture() :
        dispatcher(boost::make_shared<entt::dispatcher>()),
        context(boost::make_shared<v3d::event::Context>("test")) {
        dispatcher->sink<v3d::event::Event>().connect<&Fixture::receive>(*this);
        ui = boost::make_shared<v3d::ui::Engine>(
            boost::make_shared<v3d::event::Engine>(dispatcher), dispatcher,
            boost::make_shared<v3d::log::Logger>());
        BOOST_REQUIRE(ui->load(boost::json::parse(R"({ "themes": [], "containers": [ { "name": "hud", "visible": true, "components": [] } ] })").as_object()));
        container = ui->container("hud");
        BOOST_REQUIRE(container);
        seam = boost::make_shared<v3d::ui::shell::Keyboard>(ui, dispatcher,
            boost::shared_ptr<v3d::render::realtime::Window>());
    }

    void receive(const v3d::event::Event& event) {
        sent.push_back(event.str());
    }

    boost::shared_ptr<entt::dispatcher> dispatcher;
    boost::shared_ptr<v3d::event::Context> context;
    std::vector<std::string> sent;
    boost::shared_ptr<v3d::ui::Engine> ui;
    boost::shared_ptr<v3d::ui::Container> container;
    boost::shared_ptr<v3d::ui::shell::Keyboard> seam;
};

/**
 * A key going down, as SDL would report it. The modifiers ride on the event rather than
 * being polled, and the adapter reads them from there.
 **/
/** A key, and the modifiers held with it. **/
struct Press {
    SDL_Keycode key = SDLK_UNKNOWN;
    SDL_Keymod mod = SDL_KMOD_NONE;
};

SDL_Event keyDown(const Press& press) {
    SDL_Event event = {};
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.key = press.key;
    event.key.mod = press.mod;
    return event;
}

SDL_Event keyUp(SDL_Keycode key) {
    SDL_Event event = {};
    event.type = SDL_EVENT_KEY_UP;
    event.key.key = key;
    return event;
}

SDL_Event composed(const char* utf8) {
    SDL_Event event = {};
    event.type = SDL_EVENT_TEXT_INPUT;
    event.text.text = utf8;
    return event;
}

boost::shared_ptr<v3d::ui::component::TextBox> box(const std::string& value) {
    boost::shared_ptr<v3d::ui::component::TextBox> made =
        boost::make_shared<v3d::ui::component::TextBox>();
    made->name("field");
    made->text(value);
    return made;
}

BOOST_AUTO_TEST_SUITE(shell_keyboard_test)

/**
 * A ui with nothing focused takes neither kind of input, so a game's movement bindings keep
 * working until something is clicked into.
 **/
BOOST_AUTO_TEST_CASE(nothing_focused_takes_nothing) {
    const Fixture fixture;
    fixture.container->add(box("abc"));  // present, but nothing has the focus

    BOOST_CHECK(!fixture.seam->event(keyDown({.key = SDLK_A})));
    BOOST_CHECK(!fixture.seam->event(composed("a")));
}

/**
 * A key going down reaches the focused box under the name api/input gives it, the same
 * name a binding uses.
 **/
BOOST_AUTO_TEST_CASE(a_key_reaches_the_focused_box) {
    const Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::TextBox> field = box("abc");
    fixture.container->add(field);
    fixture.ui->focus(field);

    BOOST_CHECK(fixture.seam->event(keyDown({.key = SDLK_BACKSPACE})));
    BOOST_CHECK_EQUAL(std::string(field->text()), "ab");
}

/**
 * A composed character arrives as text, not as a key. It goes in at the caret whole, with
 * shift already applied to it.
 **/
BOOST_AUTO_TEST_CASE(a_composed_character_goes_in_at_the_caret) {
    const Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::TextBox> field = box("ab");
    fixture.container->add(field);
    fixture.ui->focus(field);

    BOOST_CHECK(fixture.seam->event(composed("C")));
    BOOST_CHECK_EQUAL(std::string(field->text()), "abC");
}

/**
 * The modifiers come off the event. Shift and a caret key selects the run it travelled, and
 * nothing polled would have said so.
 **/
BOOST_AUTO_TEST_CASE(shift_rides_on_the_event) {
    const Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::TextBox> field = box("abc");
    fixture.container->add(field);
    fixture.ui->focus(field);
    field->caret(3);

    BOOST_CHECK(fixture.seam->event(keyDown({.key = SDLK_LEFT, .mod = SDL_KMOD_LSHIFT})));
    BOOST_CHECK(field->selected());
    BOOST_CHECK_EQUAL(field->caret(), 2U);
    BOOST_CHECK_EQUAL(field->anchor(), 3U);
}

/**
 * Control comes off the event too, and names the four chords a box answers.
 **/
BOOST_AUTO_TEST_CASE(control_rides_on_the_event) {
    const Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::TextBox> field = box("abc");
    fixture.container->add(field);
    fixture.ui->focus(field);
    field->caret(0);

    BOOST_CHECK(fixture.seam->event(keyDown({.key = SDLK_A, .mod = SDL_KMOD_LCTRL})));
    BOOST_CHECK(field->selected());
    BOOST_CHECK_EQUAL(field->caret(), 3U);
    BOOST_CHECK_EQUAL(field->anchor(), 0U);
}

/**
 * A release is never taken. A key held as a box took the focus still has to be seen to come
 * up, or api/input holds it down for the rest of the run.
 **/
BOOST_AUTO_TEST_CASE(a_release_is_never_taken) {
    const Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::TextBox> field = box("abc");
    fixture.container->add(field);
    fixture.ui->focus(field);

    BOOST_CHECK(!fixture.seam->event(keyUp(SDLK_BACKSPACE)));
    BOOST_CHECK_EQUAL(std::string(field->text()), "abc");
}

/**
 * A key api/input has no name for is nothing the ui could have acted on, so it goes on to
 * the bindings rather than being swallowed.
 **/
BOOST_AUTO_TEST_CASE(an_unnamed_key_is_not_taken) {
    const Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::TextBox> field = box("abc");
    fixture.container->add(field);
    fixture.ui->focus(field);

    BOOST_CHECK(!fixture.seam->event(keyDown({.key = SDLK_PAUSE})));
    BOOST_CHECK_EQUAL(std::string(field->text()), "abc");
}

/**
 * A letter reaching a focused button is not taken. A player does not type into a button, so
 * the movement binding on that letter still fires.
 **/
BOOST_AUTO_TEST_CASE(a_letter_at_a_button_reaches_the_bindings) {
    const Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::Button> control =
        boost::make_shared<v3d::ui::component::Button>();
    control->name("go");
    fixture.container->add(control);
    fixture.ui->focus(control);

    BOOST_CHECK(!fixture.seam->event(keyDown({.key = SDLK_W})));
    BOOST_CHECK(!fixture.seam->event(composed("w")));
}

/**
 * An event that is neither a key nor a composed character is not the ui's.
 **/
BOOST_AUTO_TEST_CASE(another_event_is_not_taken) {
    const Fixture fixture;
    SDL_Event motion = {};
    motion.type = SDL_EVENT_MOUSE_MOTION;

    BOOST_CHECK(!fixture.seam->event(motion));
}

/**
 * The focus moving is announced, so text input can follow it. A move under a mouse press is
 * announced the same way, so the adapter does not have to see the press.
 **/
BOOST_AUTO_TEST_CASE(the_focus_move_is_announced) {
    const Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::TextBox> field = box("abc");
    fixture.container->add(field);

    std::vector<std::string> landed;
    fixture.ui->onFocus([&landed](const boost::shared_ptr<v3d::ui::Component>& focused) {
        landed.push_back(focused ? std::string(focused->name()) : std::string("-"));
    });

    fixture.ui->focus(field);
    fixture.ui->focus(field);  // the same component again is not a move
    fixture.ui->focus(boost::shared_ptr<v3d::ui::Component>());

    BOOST_REQUIRE_EQUAL(landed.size(), 2U);
    BOOST_CHECK_EQUAL(landed[0], "field");
    BOOST_CHECK_EQUAL(landed[1], "-");
}

/**
 * A component that did not ask to be focusable is nothing to focus, so what is announced is
 * what focused() would answer rather than what focus() was asked for.
 **/
BOOST_AUTO_TEST_CASE(what_is_announced_is_what_is_focused) {
    const Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::TextBox> field = box("abc");
    fixture.container->add(field);
    fixture.ui->focus(field);

    std::vector<boost::shared_ptr<v3d::ui::Component>> landed;
    fixture.ui->onFocus([&landed](const boost::shared_ptr<v3d::ui::Component>& focused) {
        landed.push_back(focused);
    });

    const boost::shared_ptr<v3d::ui::component::Panel> plate =
        boost::make_shared<v3d::ui::component::Panel>();
    plate->name("plate");
    fixture.container->add(plate);
    fixture.ui->focus(plate);

    BOOST_REQUIRE_EQUAL(landed.size(), 1U);
    BOOST_CHECK(!landed.front());
    BOOST_CHECK(!fixture.ui->focused());
}

/**
 * A game that draws only through an immediate layer builds the adapter with no retained ui. A
 * key and a character then reach the layer's field while it holds the keyboard, and are kept
 * from the bindings, and reach the bindings once it has given the keyboard up.
 **/
BOOST_AUTO_TEST_CASE(keys_reach_an_immediate_field_with_no_retained_ui) {
    // the layer is made first so that it outlives the adapter holding it
    v3d::ui::Immediate layer(
        [](std::string_view text) { return static_cast<float>(text.size()) * 10.0F; },
        [](std::string_view, const glm::vec2&, const glm::vec4&) {});
    const boost::shared_ptr<entt::dispatcher> dispatcher = boost::make_shared<entt::dispatcher>();
    v3d::ui::shell::Keyboard seam(boost::shared_ptr<v3d::ui::Engine>(), dispatcher,
        boost::shared_ptr<v3d::render::realtime::Window>());
    seam.immediate(&layer);
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    std::string name;
    const auto draw = [&](bool pressed) {
        v3d::ui::Immediate::Input input;
        input.cursor = glm::vec2(100.0F, 10.0F);
        input.down = pressed;
        input.pressed = pressed;
        layer.begin(&canvas, input);
        layer.textField("Name", &name);
        layer.end();
    };

    BOOST_CHECK(!seam.event(keyDown({.key = SDLK_E})));

    draw(false);
    draw(true);
    BOOST_REQUIRE(layer.typing());
    BOOST_CHECK(seam.event(keyDown({.key = SDLK_E})));
    BOOST_CHECK(seam.event(composed("e")));
    BOOST_CHECK(seam.event(keyDown({.key = SDLK_RETURN})));
    draw(false);

    BOOST_CHECK_EQUAL(name, "e");
    BOOST_CHECK(!layer.typing());
    BOOST_CHECK(!seam.event(keyDown({.key = SDLK_E})));
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
