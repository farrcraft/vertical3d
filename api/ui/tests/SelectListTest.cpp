/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "../ComponentRenderer.h"
#include "../../render/realtime/Canvas.h"
#include "../Container.h"
#include "../Engine.h"
#include "../component/SelectList.h"

#include "../../asset/Json.h"

#include <boost/json/parse.hpp>
#include <boost/make_shared.hpp>

namespace {

const float characterWidth = 10.0f;

/**
 * What the renderer asked to be written, so a test can read the rows back without a font.
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
 * A list of numbered rows in a box of the given size, laid out by a draw.
 **/
boost::shared_ptr<v3d::ui::component::SelectList> list(std::size_t rows, const glm::vec2& size) {
    boost::shared_ptr<v3d::ui::component::SelectList> component =
        boost::make_shared<v3d::ui::component::SelectList>();
    component->name("saves");
    std::vector<std::string> items;
    items.reserve(rows);
    for (std::size_t index = 0; index < rows; index++) {
        items.push_back("row " + std::to_string(index));
    }
    component->items(items);
    component->layout().width = v3d::ui::Length(size.x, v3d::ui::Length::Unit::Pixels);
    component->layout().height = v3d::ui::Length(size.y, v3d::ui::Length::Unit::Pixels);
    return component;
}

};  // namespace

BOOST_AUTO_TEST_SUITE(select_list_test)

/**
 * The rows go under the plate's clip, so a list holding more than it shows draws inside its
 * own box - ADR-0037 - and the row height the style resolved to is left on the list.
 **/
BOOST_AUTO_TEST_CASE(the_rows_are_cut_off_at_the_list) {
    std::vector<Written> written;
    v3d::ui::ComponentRenderer renderer = build(&written);
    v3d::render::realtime::Canvas canvas;
    canvas.resize(400, 300);

    const boost::shared_ptr<v3d::ui::component::SelectList> saves = list(40, glm::vec2(120.0f, 100.0f));
    // a row is chosen so that something is drawn under the clip: the rest of a row is text,
    // and text is the app's to draw
    saves->selected(0);
    v3d::ui::Container container("panel", true);
    container.add(saves);
    renderer.draw(&canvas, container);

    BOOST_CHECK_CLOSE(saves->rowHeight(), renderer.dressing().lineHeight, 0.001f);
    BOOST_CHECK_CLOSE(saves->content(), renderer.dressing().lineHeight * 40.0f, 0.001f);

    const v3d::render::realtime::Canvas::Batch& rows = canvas.batches().back();
    BOOST_REQUIRE(rows.clipped);
    BOOST_CHECK_CLOSE(rows.clip.w, saves->position().y + 100.0f - renderer.dressing().borderWidth, 0.001f);

    // and only the rows the box has room for are written, not all forty
    BOOST_CHECK(!written.empty());
    BOOST_CHECK(written.size() < 40U);
}

/**
 * Scrolling moves the rows up and starts the drawing further down the list, so the first row
 * written is the one the offset reaches rather than the first of the list.
 **/
BOOST_AUTO_TEST_CASE(an_offset_starts_the_rows_further_down) {
    std::vector<Written> written;
    v3d::ui::ComponentRenderer renderer = build(&written);
    v3d::render::realtime::Canvas canvas;
    canvas.resize(400, 300);

    const boost::shared_ptr<v3d::ui::component::SelectList> saves = list(40, glm::vec2(120.0f, 100.0f));
    v3d::ui::Container container("panel", true);
    container.add(saves);
    renderer.draw(&canvas, container);
    BOOST_REQUIRE(!written.empty());
    BOOST_CHECK_EQUAL(written.front().text, "row 0");

    written.clear();
    saves->offset(renderer.dressing().lineHeight * 5.0f);
    renderer.draw(&canvas, container);

    BOOST_REQUIRE(!written.empty());
    BOOST_CHECK_EQUAL(written.front().text, "row 5");
    BOOST_CHECK_CLOSE(written.front().pen.y, saves->position().y + renderer.dressing().lineHeight * 0.7f, 0.001f);
}

/**
 * A list cannot be scrolled past what it holds, and a list showing everything cannot be
 * scrolled at all.
 **/
BOOST_AUTO_TEST_CASE(the_offset_is_clamped_to_what_the_box_does_not_show) {
    v3d::ui::ComponentRenderer renderer = build(nullptr);
    v3d::render::realtime::Canvas canvas;
    canvas.resize(400, 300);

    const boost::shared_ptr<v3d::ui::component::SelectList> saves = list(10, glm::vec2(120.0f, 100.0f));
    v3d::ui::Container container("panel", true);
    container.add(saves);
    renderer.draw(&canvas, container);

    saves->offset(10000.0f);
    BOOST_CHECK_CLOSE(saves->offset(), saves->content() - 100.0f, 0.001f);

    const boost::shared_ptr<v3d::ui::component::SelectList> few = list(2, glm::vec2(120.0f, 100.0f));
    v3d::ui::Container roomy("roomy", true);
    roomy.add(few);
    renderer.draw(&canvas, roomy);
    few->offset(50.0f);
    BOOST_CHECK_CLOSE(few->offset(), 0.0f, 0.001f);
}

/**
 * Which row a point is on is answered against the box the list was drawn in and the scroll it
 * was drawn at, so the cursor lands on what is under it rather than on what would be there
 * unscrolled. A list that has never been drawn answers nothing, per ADR-0019.
 **/
BOOST_AUTO_TEST_CASE(a_point_names_the_row_under_it) {
    v3d::ui::ComponentRenderer renderer = build(nullptr);
    v3d::render::realtime::Canvas canvas;
    canvas.resize(400, 300);

    const boost::shared_ptr<v3d::ui::component::SelectList> undrawn = list(10, glm::vec2(120.0f, 100.0f));
    BOOST_CHECK_EQUAL(undrawn->at(glm::vec2(10.0f, 10.0f)), v3d::ui::component::SelectList::none);

    const boost::shared_ptr<v3d::ui::component::SelectList> saves = list(40, glm::vec2(120.0f, 100.0f));
    saves->layout().x = v3d::ui::Length(20.0f, v3d::ui::Length::Unit::Pixels);
    saves->layout().y = v3d::ui::Length(30.0f, v3d::ui::Length::Unit::Pixels);
    v3d::ui::Container container("panel", true);
    container.add(saves);
    renderer.draw(&canvas, container);

    const float row = saves->rowHeight();
    BOOST_CHECK_EQUAL(saves->at(glm::vec2(30.0f, 30.0f + row * 0.5f)), 0);
    BOOST_CHECK_EQUAL(saves->at(glm::vec2(30.0f, 30.0f + row * 2.5f)), 2);
    // outside the box, and past the last row that fits
    BOOST_CHECK_EQUAL(saves->at(glm::vec2(300.0f, 40.0f)), v3d::ui::component::SelectList::none);

    saves->offset(row * 5.0f);
    BOOST_CHECK_EQUAL(saves->at(glm::vec2(30.0f, 30.0f + row * 0.5f)), 5);
}

/**
 * The chosen row is a place in the rows, so replacing them keeps a choice that is still
 * there and clears one that is not.
 **/
BOOST_AUTO_TEST_CASE(replacing_the_rows_keeps_a_choice_that_survives) {
    const boost::shared_ptr<v3d::ui::component::SelectList> saves =
        boost::make_shared<v3d::ui::component::SelectList>();
    saves->items({"one", "two", "three"});
    saves->selected(2);
    BOOST_CHECK_EQUAL(std::string(saves->selection()), "three");

    saves->items({"one", "two"});
    BOOST_CHECK_EQUAL(saves->selected(), v3d::ui::component::SelectList::none);
    BOOST_CHECK(saves->selection().empty());

    saves->selected(1);
    saves->items({"first", "second", "third"});
    BOOST_CHECK_EQUAL(saves->selected(), 1);
    BOOST_CHECK_EQUAL(std::string(saves->selection()), "second");

    saves->selected(9);
    BOOST_CHECK_EQUAL(saves->selected(), v3d::ui::component::SelectList::none);
}

/**
 * The chosen row is drawn on a highlight and in the active ink, which is the only thing that
 * tells it from the rows around it.
 **/
BOOST_AUTO_TEST_CASE(the_chosen_row_is_drawn_differently) {
    std::vector<Written> written;
    v3d::ui::ComponentRenderer renderer = build(&written);
    v3d::render::realtime::Canvas canvas;
    canvas.resize(400, 300);

    const boost::shared_ptr<v3d::ui::component::SelectList> saves = list(4, glm::vec2(120.0f, 100.0f));
    v3d::ui::Container container("panel", true);
    container.add(saves);
    renderer.draw(&canvas, container);
    const std::size_t plain = canvas.indices().size();

    canvas.clear();
    written.clear();
    saves->selected(1);
    renderer.draw(&canvas, container);

    BOOST_CHECK(canvas.indices().size() > plain);
    BOOST_REQUIRE_EQUAL(written.size(), 4U);
    BOOST_CHECK(written[1].colour != written[0].colour);
}

/**
 * The loader reads the rows and the row a config chose, for a list whose contents are known
 * before the app runs.
 **/
BOOST_AUTO_TEST_CASE(the_loader_reads_a_list) {
    const std::string document = R"({
  "themes": [ { "name": "default" } ],
  "containers": [
    {
      "name": "panel",
      "visible": true,
      "components": [
        { "name": "saves", "type": "list", "items": ["Autosave", "Slot 1"], "selected": 1,
          "command": "loadGame", "context": "ui" }
      ]
    }
  ]
})";

    boost::shared_ptr<entt::dispatcher> dispatcher = boost::make_shared<entt::dispatcher>();
    boost::shared_ptr<v3d::ui::Engine> ui = boost::make_shared<v3d::ui::Engine>(
        boost::make_shared<v3d::event::Engine>(dispatcher), dispatcher,
        boost::make_shared<v3d::log::Logger>());
    BOOST_REQUIRE(ui->load(boost::make_shared<v3d::asset::Json>(
        "vgui", v3d::asset::Type::JsonDocument, boost::json::parse(document).as_object())));

    const boost::shared_ptr<v3d::ui::component::SelectList> saves =
        boost::dynamic_pointer_cast<v3d::ui::component::SelectList>(ui->container("panel")->get("saves"));
    BOOST_REQUIRE(saves);
    BOOST_REQUIRE_EQUAL(saves->items().size(), 2U);
    BOOST_CHECK_EQUAL(std::string(saves->selection()), "Slot 1");
    BOOST_CHECK(saves->event().context());
}

BOOST_AUTO_TEST_SUITE_END()
