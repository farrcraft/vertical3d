/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/asset/kind/Json.h>
#include <api/render/realtime/Canvas.h>
#include <api/ui/Arranger.h>
#include <api/ui/Container.h>
#include <api/ui/Engine.h>
#include <api/ui/Length.h>
#include <api/ui/component/Slider.h>
#include <api/ui/input/Cursor.h>
#include <api/ui/input/Keys.h>
#include <api/ui/paint/ComponentRenderer.h>
#include <api/ui/style/Resolver.h>

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

#include <boost/json/parse.hpp>
#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>
#include <entt/entt.hpp>

namespace {

/**
 * A ui engine holding one container, the renderer that places what it holds, and both
 * routers over it, so a slider can be pressed, dragged and keyed.
 **/
struct Fixture final {
    explicit Fixture(const char* document = R"({ "themes": [], "containers": [ { "name": "hud", "visible": true, "components": [] } ] })") :
        dispatcher(boost::make_shared<entt::dispatcher>()),
        context(boost::make_shared<v3d::event::Context>("test")),
        renderer(
            [](std::string_view text) { return static_cast<float>(text.size()) * 10.0f; },
            [](std::string_view, const glm::vec2&, const glm::vec4&) {}) {
        dispatcher->sink<v3d::event::Event>().connect<&Fixture::receive>(*this);
        canvas.resize(800, 600);
        ui = boost::make_shared<v3d::ui::Engine>(
            boost::make_shared<v3d::event::Engine>(dispatcher), dispatcher,
            boost::make_shared<v3d::log::Logger>());
        BOOST_REQUIRE(ui->load(boost::make_shared<v3d::asset::kind::Json>("vgui", v3d::asset::Type::JsonDocument,
            boost::json::parse(document).as_object())));
        container = ui->container("hud");
        BOOST_REQUIRE(container);
        cursor = boost::make_shared<v3d::ui::input::Cursor>(ui, dispatcher);
        keys = boost::make_shared<v3d::ui::input::Keys>(ui, dispatcher);
    }

    void receive(const v3d::event::Event& event) {
        sent.push_back(event.str());
    }

    /**
     * A slider from 0 to 10 in steps of 1, 200 wide and 20 tall at (100, 50), drawn so that
     * it can be picked.
     **/
    boost::shared_ptr<v3d::ui::component::Slider> placed() {
        boost::shared_ptr<v3d::ui::component::Slider> slider = boost::make_shared<v3d::ui::component::Slider>();
        slider->range(0.0f, 10.0f, 1.0f);
        slider->event(v3d::event::Event("volume", context));
        slider->layout().x = v3d::ui::Length(100.0f, v3d::ui::Length::Unit::Pixels);
        slider->layout().y = v3d::ui::Length(50.0f, v3d::ui::Length::Unit::Pixels);
        slider->layout().width = v3d::ui::Length(200.0f, v3d::ui::Length::Unit::Pixels);
        slider->layout().height = v3d::ui::Length(20.0f, v3d::ui::Length::Unit::Pixels);
        container->add(slider);
        draw();
        return slider;
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

};  // namespace

BOOST_AUTO_TEST_SUITE(slider_test)

/**
 * A value is clamped to the range and snapped to a whole number of steps from the minimum,
 * and a new range does the same to the value already held.
 **/
BOOST_AUTO_TEST_CASE(a_value_is_clamped_and_snapped) {
    v3d::ui::component::Slider slider;
    slider.range(10.0f, 20.0f, 2.0f);
    BOOST_CHECK_EQUAL(slider.value(), 10.0f);

    BOOST_CHECK(slider.value(13.2f));
    BOOST_CHECK_EQUAL(slider.value(), 14.0f);
    BOOST_CHECK(slider.value(99.0f));
    BOOST_CHECK_EQUAL(slider.value(), 20.0f);
    BOOST_CHECK(!slider.value(21.0f));
    BOOST_CHECK(slider.value(-5.0f));
    BOOST_CHECK_EQUAL(slider.value(), 10.0f);

    slider.value(18.0f);
    slider.range(0.0f, 12.0f, 5.0f);
    // 12 is past 10 and short of 15, and a step that does not divide the range stops at the end
    BOOST_CHECK_EQUAL(slider.value(), 12.0f);
}

/**
 * A continuous slider keeps any value in its range, and its fraction is where that stands.
 **/
BOOST_AUTO_TEST_CASE(a_continuous_slider_keeps_any_value) {
    v3d::ui::component::Slider slider;
    slider.range(-1.0f, 1.0f, 0.0f);
    slider.value(0.37f);
    BOOST_CHECK_CLOSE(slider.value(), 0.37f, 0.001f);
    BOOST_CHECK_CLOSE(slider.fraction(), 0.685f, 0.001f);
    slider.value(-1.0f);
    BOOST_CHECK_SMALL(slider.fraction(), 0.0001f);
    slider.value(1.0f);
    BOOST_CHECK_CLOSE(slider.fraction(), 1.0f, 0.001f);
}

/**
 * A press on the track sets the value under it and sends the command once; a drag past the
 * end holds the maximum, a drag within one step sends nothing more, and the release follows.
 **/
BOOST_AUTO_TEST_CASE(a_press_and_a_drag_set_the_value_and_send_on_a_change) {
    Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::Slider> slider = fixture.placed();

    BOOST_CHECK(fixture.cursor->press(glm::vec2(200.0f, 60.0f)));
    BOOST_CHECK_EQUAL(slider->value(), 5.0f);
    BOOST_REQUIRE_EQUAL(fixture.sent.size(), 1u);
    BOOST_CHECK_EQUAL(fixture.sent[0], "test::volume");

    // within the same step, so the value and what was sent stay as they were
    fixture.cursor->motion(glm::vec2(204.0f, 60.0f));
    BOOST_CHECK_EQUAL(slider->value(), 5.0f);
    BOOST_CHECK_EQUAL(fixture.sent.size(), 1u);

    fixture.cursor->motion(glm::vec2(900.0f, 300.0f));
    BOOST_CHECK_EQUAL(slider->value(), 10.0f);
    BOOST_CHECK_EQUAL(fixture.sent.size(), 2u);

    fixture.cursor->release(glm::vec2(120.0f, 60.0f));
    BOOST_CHECK_EQUAL(slider->value(), 1.0f);
    BOOST_CHECK_EQUAL(fixture.sent.size(), 3u);
}

/**
 * The arrows move a step, the page keys a tenth of the range, home and end to the ends, and
 * each sends the command. A key that would move nothing is not taken.
 **/
BOOST_AUTO_TEST_CASE(the_keys_move_a_slider) {
    Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::Slider> slider = fixture.placed();
    fixture.ui->focus(slider);

    BOOST_CHECK(!fixture.keys->press("arrow_left"));
    BOOST_CHECK(!fixture.keys->press("home"));
    BOOST_CHECK(fixture.sent.empty());

    BOOST_CHECK(fixture.keys->press("arrow_right"));
    BOOST_CHECK_EQUAL(slider->value(), 1.0f);
    BOOST_CHECK(fixture.keys->press("pageup"));
    BOOST_CHECK_EQUAL(slider->value(), 2.0f);
    BOOST_CHECK(fixture.keys->press("end"));
    BOOST_CHECK_EQUAL(slider->value(), 10.0f);
    BOOST_CHECK(!fixture.keys->press("arrow_right"));
    BOOST_CHECK(fixture.keys->press("pagedown"));
    BOOST_CHECK_EQUAL(slider->value(), 9.0f);
    BOOST_CHECK_EQUAL(fixture.sent.size(), 4u);

    // and a key a slider has nothing to do with goes on past it
    BOOST_CHECK(!fixture.keys->press("w"));
}

/**
 * A document names a slider's range, its step, its value and its command.
 **/
BOOST_AUTO_TEST_CASE(a_document_names_a_slider) {
    Fixture fixture(R"({ "themes": [], "containers": [ { "name": "hud", "visible": true, "components": [
        { "type": "slider", "name": "music", "minimum": 0, "maximum": 100, "step": 5, "value": 62,
          "context": "settings", "command": "music" }
    ] } ] })");
    const boost::shared_ptr<v3d::ui::component::Slider> slider =
        boost::dynamic_pointer_cast<v3d::ui::component::Slider>(fixture.container->get("music"));
    BOOST_REQUIRE(slider);
    BOOST_CHECK_EQUAL(slider->maximum(), 100.0f);
    BOOST_CHECK_EQUAL(slider->step(), 5.0f);
    BOOST_CHECK_EQUAL(slider->value(), 60.0f);
    BOOST_CHECK_EQUAL(slider->event().str(), "settings::music");
    BOOST_CHECK(slider->pickable());
    BOOST_CHECK(slider->focusable());
}

/**
 * A slider runs the width it is offered and is as tall as its class's mark.
 **/
BOOST_AUTO_TEST_CASE(a_slider_is_as_tall_as_its_mark) {
    v3d::ui::style::Resolver styles;
    const v3d::ui::Arranger arranger([](std::string_view text) { return static_cast<float>(text.size()); }, styles);
    v3d::ui::component::Slider slider;
    const glm::vec2 natural = arranger.natural(slider,
        v3d::type::geometry::Bound2D(glm::vec2(0.0f, 0.0f), glm::vec2(320.0f, 200.0f)));
    BOOST_CHECK_CLOSE(natural.x, 320.0f, 0.001f);
    BOOST_CHECK_CLOSE(natural.y, styles.base().markSize, 0.001f);
}

/**
 * The thumb is a square as tall as the track, drawn last, and stands at the value's fraction
 * of the track less its own width.
 **/
BOOST_AUTO_TEST_CASE(the_thumb_is_drawn_at_the_fraction) {
    Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::Slider> slider = fixture.placed();
    slider->value(5.0f);
    fixture.draw();

    const std::vector<v3d::render::realtime::Canvas::Vertex>& vertices = fixture.canvas.vertices();
    BOOST_REQUIRE(vertices.size() >= 4);
    float left = vertices[vertices.size() - 4].position.x;
    float right = left;
    for (std::size_t index = vertices.size() - 4; index < vertices.size(); ++index) {
        left = std::min(left, vertices[index].position.x);
        right = std::max(right, vertices[index].position.x);
    }
    // 100 in, plus half of the 180 the 20 wide thumb travels
    BOOST_CHECK_CLOSE(left, 190.0f, 0.001f);
    BOOST_CHECK_CLOSE(right, 210.0f, 0.001f);
}

BOOST_AUTO_TEST_SUITE_END()
