/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/event/Context.h>
#include <api/event/Engine.h>
#include <api/event/Event.h>
#include <api/log/Logger.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/type/geometry/Bound2D.h>
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
#include <cstddef>
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
            [](std::string_view text) { return static_cast<float>(text.size()) * 10.0F; },
            [](std::string_view, const glm::vec2&, const glm::vec4&) {}) {
        dispatcher->sink<v3d::event::Event>().connect<&Fixture::receive>(*this);
        canvas.resize(glm::uvec2(800, 600));
        ui = boost::make_shared<v3d::ui::Engine>(
            boost::make_shared<v3d::event::Engine>(dispatcher), dispatcher,
            boost::make_shared<v3d::log::Logger>());
        BOOST_REQUIRE(ui->load(boost::json::parse(document).as_object()));
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
        slider->range({.minimum = 0.0F, .maximum = 10.0F, .step = 1.0F});
        slider->event(v3d::event::Event("volume", context));
        slider->layout().x = v3d::ui::Length(100.0F, v3d::ui::Length::Unit::Pixels);
        slider->layout().y = v3d::ui::Length(50.0F, v3d::ui::Length::Unit::Pixels);
        slider->layout().width = v3d::ui::Length(200.0F, v3d::ui::Length::Unit::Pixels);
        slider->layout().height = v3d::ui::Length(20.0F, v3d::ui::Length::Unit::Pixels);
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
    v3d::render::realtime::canvas::Canvas canvas;
    std::vector<std::string> sent;
    v3d::ui::paint::ComponentRenderer renderer;
    boost::shared_ptr<v3d::ui::Engine> ui;
    boost::shared_ptr<v3d::ui::Container> container;
    boost::shared_ptr<v3d::ui::input::Cursor> cursor;
    boost::shared_ptr<v3d::ui::input::Keys> keys;
};

BOOST_AUTO_TEST_SUITE(slider_test)

/**
 * A value is clamped to the range and snapped to a whole number of steps from the minimum,
 * and a new range does the same to the value already held.
 **/
BOOST_AUTO_TEST_CASE(a_value_is_clamped_and_snapped) {
    v3d::ui::component::Slider slider;
    slider.range({.minimum = 10.0F, .maximum = 20.0F, .step = 2.0F});
    BOOST_CHECK_EQUAL(slider.value(), 10.0F);

    BOOST_CHECK(slider.value(13.2F));
    BOOST_CHECK_EQUAL(slider.value(), 14.0F);
    BOOST_CHECK(slider.value(99.0F));
    BOOST_CHECK_EQUAL(slider.value(), 20.0F);
    BOOST_CHECK(!slider.value(21.0F));
    BOOST_CHECK(slider.value(-5.0F));
    BOOST_CHECK_EQUAL(slider.value(), 10.0F);

    slider.value(18.0F);
    slider.range({.minimum = 0.0F, .maximum = 12.0F, .step = 5.0F});
    // 12 is past 10 and short of 15, and a step that does not divide the range stops at the end
    BOOST_CHECK_EQUAL(slider.value(), 12.0F);
}

/**
 * A continuous slider keeps any value in its range, and its fraction is where that stands.
 **/
BOOST_AUTO_TEST_CASE(a_continuous_slider_keeps_any_value) {
    v3d::ui::component::Slider slider;
    slider.range({.minimum = -1.0F, .maximum = 1.0F, .step = 0.0F});
    slider.value(0.37F);
    BOOST_CHECK_CLOSE(slider.value(), 0.37F, 0.001F);
    BOOST_CHECK_CLOSE(slider.fraction(), 0.685F, 0.001F);
    slider.value(-1.0F);
    BOOST_CHECK_SMALL(slider.fraction(), 0.0001F);
    slider.value(1.0F);
    BOOST_CHECK_CLOSE(slider.fraction(), 1.0F, 0.001F);
}

/**
 * A press on the track sets the value under it and sends the command once; a drag past the
 * end holds the maximum, a drag within one step sends nothing more, and the release follows.
 **/
BOOST_AUTO_TEST_CASE(a_press_and_a_drag_set_the_value_and_send_on_a_change) {
    Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::Slider> slider = fixture.placed();

    BOOST_CHECK(fixture.cursor->press(glm::vec2(200.0F, 60.0F)));
    BOOST_CHECK_EQUAL(slider->value(), 5.0F);
    BOOST_REQUIRE_EQUAL(fixture.sent.size(), 1U);
    BOOST_CHECK_EQUAL(fixture.sent[0], "test::volume");

    // within the same step, so the value and what was sent stay as they were
    fixture.cursor->motion(glm::vec2(204.0F, 60.0F));
    BOOST_CHECK_EQUAL(slider->value(), 5.0F);
    BOOST_CHECK_EQUAL(fixture.sent.size(), 1U);

    fixture.cursor->motion(glm::vec2(900.0F, 300.0F));
    BOOST_CHECK_EQUAL(slider->value(), 10.0F);
    BOOST_CHECK_EQUAL(fixture.sent.size(), 2U);

    fixture.cursor->release(glm::vec2(120.0F, 60.0F));
    BOOST_CHECK_EQUAL(slider->value(), 1.0F);
    BOOST_CHECK_EQUAL(fixture.sent.size(), 3U);
}

/**
 * The arrows move a step, the page keys a tenth of the range, home and end to the ends, and
 * each sends the command. A key that would move nothing is not taken.
 **/
BOOST_AUTO_TEST_CASE(the_keys_move_a_slider) {
    Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::Slider> slider = fixture.placed();
    // a step of one over a range of a hundred, so a page of ten differs from a line
    slider->range({.minimum = 0.0F, .maximum = 100.0F, .step = 1.0F});
    fixture.ui->focus(slider);

    BOOST_CHECK(!fixture.keys->press("arrow_left"));
    BOOST_CHECK(!fixture.keys->press("home"));
    BOOST_CHECK(fixture.sent.empty());

    BOOST_CHECK(fixture.keys->press("arrow_right"));
    BOOST_CHECK_EQUAL(slider->value(), 1.0F);
    BOOST_CHECK(fixture.keys->press("pageup"));
    BOOST_CHECK_EQUAL(slider->value(), 11.0F);
    BOOST_CHECK(fixture.keys->press("arrow_left"));
    BOOST_CHECK_EQUAL(slider->value(), 10.0F);
    BOOST_CHECK(fixture.keys->press("end"));
    BOOST_CHECK_EQUAL(slider->value(), 100.0F);
    BOOST_CHECK(!fixture.keys->press("arrow_right"));
    BOOST_CHECK(!fixture.keys->press("pageup"));
    BOOST_CHECK(fixture.keys->press("pagedown"));
    BOOST_CHECK_EQUAL(slider->value(), 90.0F);
    BOOST_CHECK_EQUAL(fixture.sent.size(), 5U);

    // and a key a slider has nothing to do with goes on past it
    BOOST_CHECK(!fixture.keys->press("w"));
}

/**
 * A document names a slider's range, its step, its value and its command.
 **/
BOOST_AUTO_TEST_CASE(a_document_names_a_slider) {
    const Fixture fixture(R"({ "themes": [], "containers": [ { "name": "hud", "visible": true, "components": [
        { "type": "slider", "name": "music", "minimum": 0, "maximum": 100, "step": 5, "value": 62,
          "context": "settings", "command": "music" }
    ] } ] })");
    const boost::shared_ptr<v3d::ui::component::Slider> slider =
        boost::dynamic_pointer_cast<v3d::ui::component::Slider>(fixture.container->get("music"));
    BOOST_REQUIRE(slider);
    BOOST_CHECK_EQUAL(slider->maximum(), 100.0F);
    BOOST_CHECK_EQUAL(slider->step(), 5.0F);
    BOOST_CHECK_EQUAL(slider->value(), 60.0F);
    BOOST_CHECK_EQUAL(slider->event().str(), "settings::music");
    BOOST_CHECK(slider->pickable());
    BOOST_CHECK(slider->focusable());
}

/**
 * A key that is not a number, or is beyond the range of a float, takes its default rather
 * than failing the load.
 **/
BOOST_AUTO_TEST_CASE(a_slider_key_that_is_not_a_number_takes_its_default) {
    const Fixture fixture(R"({ "themes": [], "containers": [ { "name": "hud", "visible": true, "components": [
        { "type": "slider", "name": "music", "minimum": "low", "maximum": 1e300, "step": [ 5 ], "value": true }
    ] } ] })");
    const boost::shared_ptr<v3d::ui::component::Slider> slider =
        boost::dynamic_pointer_cast<v3d::ui::component::Slider>(fixture.container->get("music"));
    BOOST_REQUIRE(slider);
    BOOST_CHECK_EQUAL(slider->minimum(), 0.0F);
    BOOST_CHECK_EQUAL(slider->maximum(), 1.0F);
    BOOST_CHECK_EQUAL(slider->step(), 0.0F);
    BOOST_CHECK_EQUAL(slider->value(), 0.0F);
}

/**
 * A slider runs the width it is offered and is as tall as its class's mark.
 **/
BOOST_AUTO_TEST_CASE(a_slider_is_as_tall_as_its_mark) {
    v3d::ui::style::Resolver styles;
    const v3d::ui::Arranger arranger([](std::string_view text) { return static_cast<float>(text.size()); }, styles);
    v3d::ui::component::Slider slider;
    const glm::vec2 natural = arranger.natural(slider,
        v3d::type::geometry::Bound2D({.position = glm::vec2(0.0F, 0.0F), .size = glm::vec2(320.0F, 200.0F)}));
    BOOST_CHECK_CLOSE(natural.x, 320.0F, 0.001F);
    BOOST_CHECK_CLOSE(natural.y, styles.base().markSize, 0.001F);
}

/**
 * The thumb is a square as tall as the track, drawn last, and stands at the value's fraction
 * of the track less its own width.
 **/
BOOST_AUTO_TEST_CASE(the_thumb_is_drawn_at_the_fraction) {
    Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::Slider> slider = fixture.placed();
    slider->value(5.0F);
    fixture.draw();

    const std::vector<v3d::render::realtime::canvas::Canvas::Vertex>& vertices = fixture.canvas.vertices();
    BOOST_REQUIRE(vertices.size() >= 4);
    float left = vertices[vertices.size() - 4].position.x;
    float right = left;
    for (std::size_t index = vertices.size() - 4; index < vertices.size(); ++index) {
        left = std::min(left, vertices[index].position.x);
        right = std::max(right, vertices[index].position.x);
    }
    // 100 in, plus half of the 180 the 20 wide thumb travels
    BOOST_CHECK_CLOSE(left, 190.0F, 0.001F);
    BOOST_CHECK_CLOSE(right, 210.0F, 0.001F);
}

/**
 * A press on the middle of the thumb leaves the value where it was, so grabbing the thumb
 * does not make it jump. The thumb's centre runs half a thumb in from each end of the track.
 **/
BOOST_AUTO_TEST_CASE(a_press_on_the_thumb_keeps_the_value) {
    v3d::ui::component::Slider slider;
    slider.range({.minimum = 0.0F, .maximum = 1.0F, .step = 0.0F});
    slider.position(glm::vec2(100.0F, 50.0F));
    slider.size(glm::vec2(200.0F, 20.0F));
    for (const float value : {0.0F, 0.2F, 0.9F, 1.0F}) {
        slider.value(value);
        const float centre = 100.0F + 10.0F + (180.0F * slider.fraction());
        slider.drag(glm::vec2(centre, 60.0F));
        BOOST_CHECK_CLOSE(slider.value() + 1.0F, value + 1.0F, 0.001F);
    }
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
