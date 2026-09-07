/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <algorithm>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "../ComponentRenderer.h"
#include "../../render/realtime/Canvas.h"
#include "../Container.h"
#include "../Engine.h"
#include "../component/CheckBox.h"
#include "../component/RadioButton.h"

#include "../../asset/Json.h"

#include <boost/json/parse.hpp>
#include <boost/make_shared.hpp>

namespace {

/**
 * A fixed width per character, so a label's natural width is predictable.
 **/
const float characterWidth = 10.0f;

/**
 * What the renderer asked to be written, so a test can read a row back without a font.
 **/
struct Written final {
    std::string text;
    glm::vec2 pen;
    glm::vec4 colour;
};

v3d::ui::ComponentRenderer build(std::vector<Written>* written) {
    return v3d::ui::ComponentRenderer(
        [](const std::string& text) { return static_cast<float>(text.size()) * characterWidth; },
        [written](const std::string& text, const glm::vec2& pen, const glm::vec4& colour) {
            if (written != nullptr) {
                written->push_back(Written{text, pen, colour});
            }
        });
}

/**
 * A ui engine over a document written inline, which is what a config file amounts to by
 * the time it reaches the loader.
 **/
boost::shared_ptr<v3d::ui::Engine> load(const std::string& config) {
    boost::shared_ptr<entt::dispatcher> dispatcher = boost::make_shared<entt::dispatcher>();
    boost::shared_ptr<v3d::ui::Engine> ui = boost::make_shared<v3d::ui::Engine>(
        boost::make_shared<v3d::event::Engine>(dispatcher),
        dispatcher,
        boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(ui->load(boost::make_shared<v3d::asset::Json>(
        "vgui", v3d::asset::Type::JsonDocument, boost::json::parse(config).as_object())));
    return ui;
}

};  // namespace

BOOST_AUTO_TEST_SUITE(check_box_test)

/**
 * A check box asks for the mark, the gap after it and its label, on a row as tall as the
 * taller of the mark and a line of text.
 **/
BOOST_AUTO_TEST_CASE(a_check_box_asks_for_its_mark_and_its_label) {
    std::vector<Written> written;
    v3d::ui::ComponentRenderer renderer = build(&written);
    v3d::render::realtime::Canvas canvas;
    canvas.resize(400, 300);

    const boost::shared_ptr<v3d::ui::component::CheckBox> box =
        boost::make_shared<v3d::ui::component::CheckBox>();
    box->name("shadows");
    box->label("Shadows");

    v3d::ui::Container container("options", true);
    container.add(box);
    renderer.draw(&canvas, container);

    const v3d::ui::ComponentRenderer::Dressing& style = build(nullptr).dressing();
    BOOST_CHECK_CLOSE(box->size().x,
        style.markSize + style.padding * 0.5f + 7.0f * characterWidth, 0.001f);
    BOOST_CHECK_CLOSE(box->size().y, std::max(style.markSize, style.lineHeight), 0.001f);

    // the label is written past the mark rather than over it
    BOOST_REQUIRE_EQUAL(written.size(), 1U);
    BOOST_CHECK_EQUAL(written[0].text, "Shadows");
    BOOST_CHECK(written[0].pen.x >= box->position().x + style.markSize);
}

/**
 * The mark is what checked() adds, so an unchecked box is the plate alone and a checked one
 * is the plate and the mark over it.
 **/
BOOST_AUTO_TEST_CASE(the_mark_is_drawn_only_when_the_box_is_checked) {
    v3d::ui::ComponentRenderer renderer = build(nullptr);
    v3d::render::realtime::Canvas canvas;
    canvas.resize(400, 300);

    const boost::shared_ptr<v3d::ui::component::CheckBox> box =
        boost::make_shared<v3d::ui::component::CheckBox>();
    renderer.draw(&canvas, box);
    const std::size_t plate = canvas.indices().size();
    BOOST_CHECK(plate > 0);

    canvas.clear();
    box->checked(true);
    renderer.draw(&canvas, box);

    BOOST_CHECK(canvas.indices().size() > plate);
}

/**
 * A radio button is drawn round, which is fans rather than quads, and is the same call and
 * the same state as a check box.
 **/
BOOST_AUTO_TEST_CASE(a_radio_button_is_a_check_box_with_a_round_mark) {
    v3d::ui::ComponentRenderer renderer = build(nullptr);
    v3d::render::realtime::Canvas square;
    v3d::render::realtime::Canvas round;
    square.resize(400, 300);
    round.resize(400, 300);

    const boost::shared_ptr<v3d::ui::component::CheckBox> box =
        boost::make_shared<v3d::ui::component::CheckBox>();
    box->checked(true);
    renderer.draw(&square, box);

    const boost::shared_ptr<v3d::ui::component::RadioButton> radio =
        boost::make_shared<v3d::ui::component::RadioButton>();
    radio->checked(true);
    radio->group("difficulty");
    renderer.draw(&round, radio);

    BOOST_CHECK(radio->type() == v3d::ui::component::Type::RadioButton);
    BOOST_CHECK_EQUAL(std::string(radio->group()), "difficulty");
    BOOST_CHECK(round.indices().size() != square.indices().size());
    // both stay in the one batched primitive, fans and quads together
    BOOST_CHECK_EQUAL(round.batches().size(), 1U);
}

/**
 * Neither owns the state it shows: a check box is checked by whatever answered its command,
 * and starts however the config left it. ADR-0019.
 **/
BOOST_AUTO_TEST_CASE(the_loader_reads_a_check_box_a_radio_button_and_a_clip) {
    const std::string document = R"({
  "themes": [ { "name": "default" } ],
  "containers": [
    {
      "name": "options",
      "visible": true,
      "components": [
        {
          "name": "panel", "type": "panel", "clip": true,
          "children": [
            { "name": "shadows", "type": "checkbox", "label": "Shadows", "checked": true,
              "command": "toggleShadows", "context": "ui" },
            { "name": "hard", "type": "radio", "label": "Hard", "group": "difficulty" },
            { "name": "bar", "type": "scrollbar", "direction": "horizontal",
              "content": 400, "page": 100, "offset": 50 }
          ]
        }
      ]
    }
  ]
})";

    const boost::shared_ptr<v3d::ui::Engine> ui = load(document);
    const boost::shared_ptr<v3d::ui::Container> container = ui->container("options");
    BOOST_REQUIRE(container);

    BOOST_CHECK(container->get("panel")->clip());

    const boost::shared_ptr<v3d::ui::component::CheckBox> box =
        boost::dynamic_pointer_cast<v3d::ui::component::CheckBox>(container->get("shadows"));
    BOOST_REQUIRE(box);
    BOOST_CHECK_EQUAL(std::string(box->label()), "Shadows");
    BOOST_CHECK(box->checked());
    BOOST_CHECK(box->event().context());

    const boost::shared_ptr<v3d::ui::component::RadioButton> radio =
        boost::dynamic_pointer_cast<v3d::ui::component::RadioButton>(container->get("hard"));
    BOOST_REQUIRE(radio);
    BOOST_CHECK_EQUAL(std::string(radio->group()), "difficulty");
    BOOST_CHECK(!radio->checked());

    const boost::shared_ptr<v3d::ui::component::Scrollbar> bar =
        boost::dynamic_pointer_cast<v3d::ui::component::Scrollbar>(container->get("bar"));
    BOOST_REQUIRE(bar);
    BOOST_CHECK(bar->direction() == v3d::ui::component::Scrollbar::Direction::Horizontal);
    BOOST_CHECK_CLOSE(bar->maximum(), 300.0f, 0.001f);
    BOOST_CHECK_CLOSE(bar->offset(), 50.0f, 0.001f);
}

BOOST_AUTO_TEST_SUITE_END()
