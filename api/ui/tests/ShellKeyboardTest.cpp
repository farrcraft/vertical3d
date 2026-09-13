/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/asset/kind/Json.h>
#include <api/ui/Container.h>
#include <api/ui/Engine.h>
#include <api/ui/component/Button.h>
#include <api/ui/component/Panel.h>
#include <api/ui/component/TextBox.h>
#include <api/ui/shell/Keyboard.h>

#include <SDL3/SDL.h>

#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <boost/json/parse.hpp>
#include <boost/make_shared.hpp>
#include <entt/entt.hpp>

namespace {

/**
 * The seam over a ui holding one container. No window: what the platform composes is not
 * what this decides, and a seam given none still routes every key.
 **/
struct Fixture final {
    Fixture() :
        dispatcher(boost::make_shared<entt::dispatcher>()),
        context(boost::make_shared<v3d::event::Context>("test")) {
        dispatcher->sink<v3d::event::Event>().connect<&Fixture::receive>(*this);
        ui = boost::make_shared<v3d::ui::Engine>(
            boost::make_shared<v3d::event::Engine>(dispatcher), dispatcher,
            boost::make_shared<v3d::log::Logger>());
        BOOST_REQUIRE(ui->load(boost::make_shared<v3d::asset::kind::Json>("vgui",
            v3d::asset::Type::JsonDocument,
            boost::json::parse(R"({ "themes": [], "containers": [ { "name": "hud", "visible": true, "components": [] } ] })").as_object())));
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
 * being polled, which is what the seam reads them off.
 **/
SDL_Event keyDown(SDL_Keycode key, SDL_Keymod mod = SDL_KMOD_NONE) {
    SDL_Event event = {};
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.key = key;
    event.key.mod = mod;
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

};  // namespace

BOOST_AUTO_TEST_SUITE(shell_keyboard_test)

/**
 * A ui with nothing focused takes neither kind of input, which is what leaves a game's
 * movement bindings working until something is clicked into.
 **/
BOOST_AUTO_TEST_CASE(nothing_focused_takes_nothing) {
    Fixture fixture;
    fixture.container->add(box("abc"));  // present, but nothing has the focus

    BOOST_CHECK(!fixture.seam->event(keyDown(SDLK_A)));
    BOOST_CHECK(!fixture.seam->event(composed("a")));
}

/**
 * A key going down reaches the focused box as the operation api/input names it, which is
 * the whole point of sharing one key name table with the device that binds the same key.
 **/
BOOST_AUTO_TEST_CASE(a_key_reaches_the_focused_box) {
    Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::TextBox> field = box("abc");
    fixture.container->add(field);
    fixture.ui->focus(field);

    BOOST_CHECK(fixture.seam->event(keyDown(SDLK_BACKSPACE)));
    BOOST_CHECK_EQUAL(std::string(field->text()), "ab");
}

/**
 * A character is not a key: what the platform composed goes in at the caret whole, with
 * shift already applied to it.
 **/
BOOST_AUTO_TEST_CASE(a_composed_character_goes_in_at_the_caret) {
    Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::TextBox> field = box("ab");
    fixture.container->add(field);
    fixture.ui->focus(field);

    BOOST_CHECK(fixture.seam->event(composed("C")));
    BOOST_CHECK_EQUAL(std::string(field->text()), "abC");
}

/**
 * The modifiers come off the event. Shift and a caret key selects the run it travelled -
 * ADR-0057 - and nothing polled would have said so.
 **/
BOOST_AUTO_TEST_CASE(shift_rides_on_the_event) {
    Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::TextBox> field = box("abc");
    fixture.container->add(field);
    fixture.ui->focus(field);
    field->caret(3);

    BOOST_CHECK(fixture.seam->event(keyDown(SDLK_LEFT, SDL_KMOD_LSHIFT)));
    BOOST_CHECK(field->selected());
    BOOST_CHECK_EQUAL(field->caret(), 2u);
    BOOST_CHECK_EQUAL(field->anchor(), 3u);
}

/**
 * And so does control, which names the four chords a box answers.
 **/
BOOST_AUTO_TEST_CASE(control_rides_on_the_event) {
    Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::TextBox> field = box("abc");
    fixture.container->add(field);
    fixture.ui->focus(field);
    field->caret(0);

    BOOST_CHECK(fixture.seam->event(keyDown(SDLK_A, SDL_KMOD_LCTRL)));
    BOOST_CHECK(field->selected());
    BOOST_CHECK_EQUAL(field->caret(), 3u);
    BOOST_CHECK_EQUAL(field->anchor(), 0u);
}

/**
 * A release is never taken. A key held as a box took the focus still has to be seen to come
 * up, or api/input holds it down for the rest of the run.
 **/
BOOST_AUTO_TEST_CASE(a_release_is_never_taken) {
    Fixture fixture;
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
    Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::TextBox> field = box("abc");
    fixture.container->add(field);
    fixture.ui->focus(field);

    BOOST_CHECK(!fixture.seam->event(keyDown(SDLK_PAUSE)));
    BOOST_CHECK_EQUAL(std::string(field->text()), "abc");
}

/**
 * A letter reaching a focused button is not taken, because a button is not something a
 * player types into - so the movement binding on that letter still fires.
 **/
BOOST_AUTO_TEST_CASE(a_letter_at_a_button_reaches_the_bindings) {
    Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::Button> control =
        boost::make_shared<v3d::ui::component::Button>();
    control->name("go");
    fixture.container->add(control);
    fixture.ui->focus(control);

    BOOST_CHECK(!fixture.seam->event(keyDown(SDLK_W)));
    BOOST_CHECK(!fixture.seam->event(composed("w")));
}

/**
 * An event that is neither a key nor a composed character is not the ui's.
 **/
BOOST_AUTO_TEST_CASE(another_event_is_not_taken) {
    Fixture fixture;
    SDL_Event motion = {};
    motion.type = SDL_EVENT_MOUSE_MOTION;

    BOOST_CHECK(!fixture.seam->event(motion));
}

/**
 * The focus moving is announced, which is what lets text input follow it. A move under a
 * mouse press is announced the same way, so the seam does not have to see the press.
 **/
BOOST_AUTO_TEST_CASE(the_focus_move_is_announced) {
    Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::TextBox> field = box("abc");
    fixture.container->add(field);

    std::vector<std::string> landed;
    fixture.ui->onFocus([&landed](const boost::shared_ptr<v3d::ui::Component>& focused) {
        landed.push_back(focused ? std::string(focused->name()) : std::string("-"));
    });

    fixture.ui->focus(field);
    fixture.ui->focus(field);  // the same component again is not a move
    fixture.ui->focus(boost::shared_ptr<v3d::ui::Component>());

    BOOST_REQUIRE_EQUAL(landed.size(), 2u);
    BOOST_CHECK_EQUAL(landed[0], "field");
    BOOST_CHECK_EQUAL(landed[1], "-");
}

/**
 * A component that did not ask to be focusable is nothing to focus, so what is announced is
 * what focused() would answer rather than what focus() was asked for.
 **/
BOOST_AUTO_TEST_CASE(what_is_announced_is_what_is_focused) {
    Fixture fixture;
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

    BOOST_REQUIRE_EQUAL(landed.size(), 1u);
    BOOST_CHECK(!landed.front());
    BOOST_CHECK(!fixture.ui->focused());
}

BOOST_AUTO_TEST_SUITE_END()
