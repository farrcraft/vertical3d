/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/event/Context.h>
#include <api/event/Event.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/type/geometry/Bound2D.h>
#include <api/ui/Arranger.h>
#include <api/ui/Component.h>
#include <api/ui/Container.h>
#include <api/ui/Length.h>
#include <api/ui/component/Button.h>
#include <api/ui/component/Label.h>
#include <api/ui/component/Panel.h>
#include <api/ui/component/menu/Menu.h>
#include <api/ui/component/menu/Item.h>
#include <api/ui/component/TextBox.h>
#include <api/ui/component/menu/Type.h>
#include <api/ui/paint/ComponentRenderer.h>
#include <api/ui/paint/Dressing.h>
#include <api/ui/paint/Text.h>
#include <api/ui/style/Resolver.h>
#include <api/ui/style/Style.h>
#include <api/ui/style/Theme.h>
#include <api/ui/style/property/Color.h>
#include <api/ui/style/property/Number.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <entt/entt.hpp>

#include <boost/make_shared.hpp>

namespace {

/**
 * What the renderer asked to be written, so a test can check where a label went without
 * a font, an atlas or a device.
 **/
struct Written final {
    std::string text;
    glm::vec2 pen;
    glm::vec4 colour;
};

/**
 * A fixed width per character, so a label's width is predictable.
 **/
const float characterWidth = 10.0F;

/**
 * A menu of action items with the given labels, with its level pointing at itself the way
 * the ui::Interface's loader leaves it.
 **/
boost::shared_ptr<v3d::ui::component::menu::Menu> buildMenu(const std::vector<std::string>& labels) {
    const boost::shared_ptr<entt::dispatcher> dispatcher = boost::make_shared<entt::dispatcher>();
    boost::shared_ptr<v3d::ui::component::menu::Menu> menu = boost::make_shared<v3d::ui::component::menu::Menu>(dispatcher);
    for (const std::string& label : labels) {
        menu->addItem(boost::make_shared<v3d::ui::component::menu::Item>(v3d::ui::component::menu::ItemType::Action, label));
    }
    menu->level(menu);
    menu->active(0);
    return menu;
}

BOOST_AUTO_TEST_SUITE(component_renderer_test)

/**
 * A menu is a panel, a highlight and a ring around the active item, and one label per item.
 * With square corners the panel is five quads: four edges traced around it and the background
 * inside them. The ring is four more, so a three item menu is ten rectangles. All of them are
 * untextured, so they are one batch.
 **/
BOOST_AUTO_TEST_CASE(a_menu_draws_a_panel_a_highlight_and_a_label_per_item) {
    std::vector<Written> written;
    v3d::ui::paint::ComponentRenderer renderer(
        [](std::string_view text) { return static_cast<float>(text.size()) * characterWidth; },
        [&written](std::string_view text, const glm::vec2& pen, const glm::vec4& colour) {
            Written line;
            line.text = text;
            line.pen = pen;
            line.colour = colour;
            written.push_back(line);
        });
    // the shadow under the panel is counted by its own case
    renderer.dressing().shadow = glm::vec4(0.0F);

    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(800, 600));

    renderer.draw(&canvas, buildMenu({"One", "Two", "Three"}));

    BOOST_REQUIRE_EQUAL(written.size(), 3);
    BOOST_CHECK_EQUAL(written[0].text, "One");
    BOOST_CHECK_EQUAL(written[2].text, "Three");
    // four edges, the panel, the highlight behind the active item and the ring's four edges
    BOOST_CHECK_EQUAL(canvas.vertices().size(), 10 * 4);
    BOOST_CHECK_EQUAL(canvas.batches().size(), 1);
}

/**
 * A menu takes the dressing's radius: its panel and its highlight are drawn with rounded
 * corners, which takes more geometry than square ones.
 **/
BOOST_AUTO_TEST_CASE(a_menu_is_drawn_with_the_dressing_radius) {
    v3d::ui::paint::ComponentRenderer renderer(
        [](std::string_view text) { return static_cast<float>(text.size()) * characterWidth; },
        [](std::string_view, const glm::vec2&, const glm::vec4&) {});
    v3d::render::realtime::canvas::Canvas square;
    square.resize(glm::uvec2(800, 600));
    renderer.draw(&square, buildMenu({"One", "Two", "Three"}));

    renderer.dressing().radius = 6.0F;
    v3d::render::realtime::canvas::Canvas rounded;
    rounded.resize(glm::uvec2(800, 600));
    renderer.draw(&rounded, buildMenu({"One", "Two", "Three"}));

    BOOST_CHECK_GT(rounded.vertices().size(), square.vertices().size());
}

/**
 * A menu casts a shadow before its panel is drawn: the panel's box moved down by the offset,
 * in the shadow's colour, with a fade around it.
 **/
BOOST_AUTO_TEST_CASE(a_menu_casts_a_shadow_under_its_panel) {
    v3d::ui::paint::ComponentRenderer renderer(
        [](std::string_view text) { return static_cast<float>(text.size()) * characterWidth; },
        [](std::string_view, const glm::vec2&, const glm::vec4&) {});
    const v3d::ui::paint::Dressing& dress = renderer.dressing();
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(800, 600));

    const boost::shared_ptr<v3d::ui::component::menu::Menu> menu = buildMenu({"One", "Two"});
    renderer.draw(&canvas, menu);

    // with square corners the shadow's box is one rectangle, drawn first
    const glm::vec2 rim(dress.borderWidth, dress.borderWidth);
    const glm::vec2 drop(0.0F, dress.shadowOffset);
    const glm::vec2 min = menu->position() - rim + drop;
    const glm::vec2 max = menu->position() + static_cast<const v3d::ui::Component&>(*menu).size() + rim + drop;
    BOOST_REQUIRE_GE(canvas.vertices().size(), 4U);
    for (std::size_t index = 0; index < 4; index++) {
        const v3d::render::realtime::canvas::Canvas::Vertex& vertex = canvas.vertices()[index];
        BOOST_CHECK(vertex.colour == dress.shadow);
        BOOST_CHECK(vertex.position.x == min.x || vertex.position.x == max.x);
        BOOST_CHECK(vertex.position.y == min.y || vertex.position.y == max.y);
    }
    // and its fade reaches past that box
    const auto reaches = [&](const v3d::render::realtime::canvas::Canvas::Vertex& vertex) {
        return vertex.position.y >= max.y + dress.shadowSize - 0.001F;
    };
    BOOST_CHECK(std::ranges::any_of(canvas.vertices(), reaches));
}

/**
 * A container that asks for a backdrop lays the backdrop over the whole canvas before its own
 * components, and one that does not draws none.
 **/
BOOST_AUTO_TEST_CASE(a_container_with_a_backdrop_dims_the_canvas_first) {
    v3d::ui::paint::ComponentRenderer renderer(
        [](std::string_view text) { return static_cast<float>(text.size()) * characterWidth; },
        [](std::string_view, const glm::vec2&, const glm::vec4&) {});

    v3d::ui::Container plain("plain", true);
    plain.add(buildMenu({"One"}));
    v3d::render::realtime::canvas::Canvas without;
    without.resize(glm::uvec2(800, 600));
    renderer.draw(&without, plain);

    v3d::ui::Container dimmed("dimmed", true);
    dimmed.backdrop(true);
    dimmed.add(buildMenu({"One"}));
    v3d::render::realtime::canvas::Canvas with;
    with.resize(glm::uvec2(800, 600));
    renderer.draw(&with, dimmed);

    BOOST_REQUIRE_EQUAL(with.vertices().size(), without.vertices().size() + 4);
    for (std::size_t index = 0; index < 4; index++) {
        const v3d::render::realtime::canvas::Canvas::Vertex& vertex = with.vertices()[index];
        BOOST_CHECK(vertex.colour == renderer.dressing().backdrop);
        BOOST_CHECK(vertex.position.x == 0.0F || vertex.position.x == 800.0F);
        BOOST_CHECK(vertex.position.y == 0.0F || vertex.position.y == 600.0F);
    }
}

/**
 * A button's fill eases toward its hover colour over the hover duration once a clock is given,
 * from nothing, and eases out again in its own colour.
 **/
BOOST_AUTO_TEST_CASE(a_hovered_button_eases_its_fill) {
    v3d::ui::paint::ComponentRenderer renderer(
        [](std::string_view text) { return static_cast<float>(text.size()) * characterWidth; },
        [](std::string_view, const glm::vec2&, const glm::vec4&) {});
    renderer.dressing().hoverDuration = 0.1F;
    const glm::vec4 hover = renderer.dressing().hover;

    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(800, 600));
    const boost::shared_ptr<v3d::ui::component::Button> button = boost::make_shared<v3d::ui::component::Button>();
    button->label("Go");
    button->size(glm::vec2(120.0F, 30.0F));

    const auto drawAt = [&](float now, std::uint64_t frame) {
        canvas.clear();
        renderer.clock({.now = now, .frame = frame});
        renderer.draw(&canvas, button);
    };

    // at rest there is no fill at all
    drawAt(0.0F, 1);
    BOOST_CHECK(canvas.empty());

    button->state(v3d::ui::component::Button::STATE_HOVER);
    drawAt(1.0F, 2);
    drawAt(1.05F, 3);
    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 4U);
    BOOST_CHECK_CLOSE(canvas.vertices()[0].colour.a, hover.a * 0.5F, 0.1F);
    BOOST_CHECK_CLOSE(canvas.vertices()[0].colour.r, hover.r, 0.1F);

    drawAt(1.2F, 4);
    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 4U);
    BOOST_CHECK(canvas.vertices()[0].colour == hover);

    button->state(v3d::ui::component::Button::STATE_NORMAL);
    drawAt(2.0F, 5);
    drawAt(2.05F, 6);
    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 4U);
    BOOST_CHECK_CLOSE(canvas.vertices()[0].colour.a, hover.a * 0.5F, 0.1F);
    drawAt(2.2F, 7);
    BOOST_CHECK(canvas.empty());
}

/**
 * A menu opens faded out and lifted by the open distance, and lands whole and in place once
 * the open duration has passed. Its boxes, which the cursor picks against, never move.
 **/
BOOST_AUTO_TEST_CASE(a_menu_opens_faded_and_lifted) {
    v3d::ui::paint::ComponentRenderer renderer(
        [](std::string_view text) { return static_cast<float>(text.size()) * characterWidth; },
        [](std::string_view, const glm::vec2&, const glm::vec4&) {});
    renderer.dressing().shadow = glm::vec4(0.0F);
    renderer.dressing().openDuration = 0.2F;
    renderer.dressing().openDistance = 8.0F;
    const glm::vec4 panel = renderer.dressing().panel;

    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(800, 600));
    const boost::shared_ptr<v3d::ui::component::menu::Menu> menu = buildMenu({"One", "Two"});

    // the panel's fill is the fifth rectangle, inside its four edges
    const auto fill = [&canvas]() { return canvas.vertices()[16]; };

    renderer.clock({.now = 1.0F, .frame = 1});
    renderer.draw(&canvas, menu);
    const glm::vec2 placed = menu->position();
    BOOST_REQUIRE_GE(canvas.vertices().size(), 20U);
    BOOST_CHECK_SMALL(fill().colour.a, 0.001F);
    BOOST_CHECK_CLOSE(fill().position.y, placed.y - 8.0F, 0.01F);

    canvas.clear();
    renderer.clock({.now = 1.3F, .frame = 2});
    renderer.draw(&canvas, menu);
    BOOST_CHECK(fill().colour == panel);
    BOOST_CHECK_CLOSE(fill().position.y, placed.y, 0.01F);
    BOOST_CHECK(menu->position() == placed);

    // closed for a frame, and so opening again
    canvas.clear();
    renderer.clock({.now = 2.0F, .frame = 4});
    renderer.draw(&canvas, menu);
    BOOST_CHECK_SMALL(fill().colour.a, 0.001F);
}

/**
 * A tooltip hangs just under the box it describes, its text at the left and the shortcut at
 * the right, and goes above the box where below would leave the canvas.
 **/
BOOST_AUTO_TEST_CASE(a_tooltip_hangs_under_its_box) {
    std::vector<Written> written;
    const v3d::ui::paint::ComponentRenderer renderer(
        [](std::string_view text) { return static_cast<float>(text.size()) * characterWidth; },
        [&written](std::string_view text, const glm::vec2& pen, const glm::vec4& colour) {
            written.push_back({.text = std::string(text), .pen = pen, .colour = colour});
        });
    const v3d::ui::paint::Dressing& dress = renderer.dressing();
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(800, 600));

    const v3d::type::geometry::Bound2D box({.position = glm::vec2(100.0F, 100.0F), .size = glm::vec2(30.0F, 30.0F)});
    renderer.tooltip(&canvas, "Move", "W", box);
    BOOST_REQUIRE_EQUAL(written.size(), 2U);
    BOOST_CHECK_EQUAL(written[0].text, "Move");
    BOOST_CHECK(written[0].colour == dress.text);
    BOOST_CHECK_EQUAL(written[1].text, "W");
    BOOST_CHECK(written[1].colour == dress.secondaryText);
    BOOST_CHECK_GT(written[1].pen.x, written[0].pen.x + (4.0F * characterWidth));
    BOOST_CHECK_GT(written[0].pen.y, 130.0F);
    BOOST_CHECK_LT(written[0].pen.y, 130.0F + dress.spacing + dress.lineHeight);

    written.clear();
    const v3d::type::geometry::Bound2D low({.position = glm::vec2(100.0F, 580.0F), .size = glm::vec2(30.0F, 20.0F)});
    renderer.tooltip(&canvas, "Move", "", low);
    BOOST_REQUIRE_EQUAL(written.size(), 1U);
    BOOST_CHECK_LT(written[0].pen.y, 580.0F);
}

/**
 * Beside its box a tooltip hangs to the right, level with the box, and to the left where the
 * right would leave the canvas.
 **/
BOOST_AUTO_TEST_CASE(a_tooltip_can_hang_beside_its_box) {
    std::vector<Written> written;
    const v3d::ui::paint::ComponentRenderer renderer(
        [](std::string_view text) { return static_cast<float>(text.size()) * characterWidth; },
        [&written](std::string_view text, const glm::vec2& pen, const glm::vec4& colour) {
            written.push_back({.text = std::string(text), .pen = pen, .colour = colour});
        });
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(800, 600));

    const v3d::type::geometry::Bound2D box({.position = glm::vec2(100.0F, 100.0F), .size = glm::vec2(30.0F, 30.0F)});
    renderer.tooltip(&canvas, "Move", "", box, true);
    BOOST_REQUIRE_EQUAL(written.size(), 1U);
    BOOST_CHECK_GT(written[0].pen.x, 130.0F);
    BOOST_CHECK_GT(written[0].pen.y, 100.0F);
    BOOST_CHECK_LT(written[0].pen.y, 130.0F);

    written.clear();
    const v3d::type::geometry::Bound2D edge({.position = glm::vec2(780.0F, 100.0F), .size = glm::vec2(20.0F, 30.0F)});
    renderer.tooltip(&canvas, "Move", "", edge, true);
    BOOST_REQUIRE_EQUAL(written.size(), 1U);
    BOOST_CHECK_LT(written[0].pen.x, 780.0F);
}

/**
 * A renderer never given a clock draws every change at once, whatever its durations.
 **/
BOOST_AUTO_TEST_CASE(without_a_clock_a_change_shows_at_once) {
    v3d::ui::paint::ComponentRenderer renderer(
        [](std::string_view text) { return static_cast<float>(text.size()) * characterWidth; },
        [](std::string_view, const glm::vec2&, const glm::vec4&) {});
    renderer.dressing().hoverDuration = 10.0F;

    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(800, 600));
    const boost::shared_ptr<v3d::ui::component::Button> button = boost::make_shared<v3d::ui::component::Button>();
    button->size(glm::vec2(120.0F, 30.0F));
    renderer.draw(&canvas, button);

    button->state(v3d::ui::component::Button::STATE_HOVER);
    canvas.clear();
    renderer.draw(&canvas, button);
    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 4U);
    BOOST_CHECK(canvas.vertices()[0].colour == renderer.dressing().hover);
}

/**
 * The active item is the one drawn in the active colour, and it is the only one - a menu that
 * highlighted two items would mean navigation had lost track of where it was.
 **/
BOOST_AUTO_TEST_CASE(only_the_active_item_is_drawn_highlighted) {
    std::vector<Written> written;
    v3d::ui::paint::ComponentRenderer renderer(
        [](std::string_view text) { return static_cast<float>(text.size()) * characterWidth; },
        [&written](std::string_view text, const glm::vec2& pen, const glm::vec4& colour) {
            Written line;
            line.text = text;
            line.pen = pen;
            line.colour = colour;
            written.push_back(line);
        });

    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(800, 600));

    const boost::shared_ptr<v3d::ui::component::menu::Menu> menu = buildMenu({"One", "Two", "Three"});
    menu->next();

    renderer.draw(&canvas, menu);

    BOOST_REQUIRE_EQUAL(written.size(), 3);
    const glm::vec4 active = renderer.dressing().activeText;
    BOOST_CHECK(written[0].colour != active);
    BOOST_CHECK(written[1].colour == active);
    BOOST_CHECK(written[2].colour != active);
}

/**
 * A disabled item in the centred game menu is written in the disabled colour, as it is in a
 * dropped panel, so it does not look like a choice the menu then refuses.
 **/
BOOST_AUTO_TEST_CASE(a_disabled_item_in_a_centred_menu_is_greyed) {
    std::vector<Written> written;
    v3d::ui::paint::ComponentRenderer renderer(
        [](std::string_view text) { return static_cast<float>(text.size()) * characterWidth; },
        [&written](std::string_view text, const glm::vec2& pen, const glm::vec4& colour) {
            Written line;
            line.text = text;
            line.pen = pen;
            line.colour = colour;
            written.push_back(line);
        });
    const glm::vec4 greyed(0.3F, 0.3F, 0.3F, 1.0F);
    renderer.dressing().disabledText = greyed;

    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(800, 600));

    const boost::shared_ptr<v3d::ui::component::menu::Menu> menu = buildMenu({"Resume", "Load", "Quit"});
    (*menu)[1]->enabled(false);
    renderer.draw(&canvas, menu);

    BOOST_REQUIRE_EQUAL(written.size(), 3U);
    BOOST_CHECK(written[0].colour != greyed);
    BOOST_CHECK(written[1].colour == greyed);
    BOOST_CHECK(written[2].colour != greyed);
}

/**
 * Descending into a submenu replaces what is on screen with the submenu's items, because the
 * menu draws the level navigation is on rather than the menu it was reached through.
 **/
BOOST_AUTO_TEST_CASE(a_submenu_replaces_what_is_drawn) {
    std::vector<Written> written;
    const v3d::ui::paint::ComponentRenderer renderer(
        [](std::string_view text) { return static_cast<float>(text.size()) * characterWidth; },
        [&written](std::string_view text, const glm::vec2& pen, const glm::vec4& colour) {
            Written line;
            line.text = text;
            line.pen = pen;
            line.colour = colour;
            written.push_back(line);
        });

    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(800, 600));

    const boost::shared_ptr<v3d::ui::component::menu::Menu> menu = buildMenu({"Options", "Quit"});
    const boost::shared_ptr<v3d::ui::component::menu::Menu> submenu = buildMenu({"Rounds", "Keys"});
    submenu->parent(menu);
    (*menu)[0] = boost::make_shared<v3d::ui::component::menu::Item>(v3d::ui::component::menu::ItemType::Submenu, "Options");
    (*menu)[0]->submenu(submenu);
    submenu->level(submenu);

    menu->activate();
    renderer.draw(&canvas, menu);

    BOOST_REQUIRE_EQUAL(written.size(), 2);
    BOOST_CHECK_EQUAL(written[0].text, "Rounds");
    BOOST_CHECK_EQUAL(written[1].text, "Keys");
}

/**
 * The panel is as wide as its widest label and no wider, and it is centred on the canvas.
 **/
BOOST_AUTO_TEST_CASE(the_panel_is_sized_to_the_widest_label_and_centred) {
    v3d::ui::paint::ComponentRenderer renderer(
        [](std::string_view text) { return static_cast<float>(text.size()) * characterWidth; },
        [](std::string_view, const glm::vec2&, const glm::vec4&) {});
    // the panel is found by where its vertices fall, after nothing but its edges
    renderer.dressing().shadow = glm::vec4(0.0F);

    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(800, 600));

    renderer.draw(&canvas, buildMenu({"a", "abcdefghij"}));

    BOOST_REQUIRE(canvas.vertices().size() >= 20);
    // vertices 16..19 are the panel, filled inside the four edges: min at [16], max at [18]
    const float padding = renderer.dressing().padding;
    const float expectedWidth = (10.0F * characterWidth) + (padding * 2.0F);
    const float left = canvas.vertices()[16].position.x;
    const float right = canvas.vertices()[18].position.x;

    BOOST_CHECK_CLOSE(right - left, expectedWidth, 0.01F);
    BOOST_CHECK_CLOSE((left + right) * 0.5F, 400.0F, 0.01F);
}

/**
 * A titled menu writes its title in the title's setting above its items, which move down to
 * make room for it, and is wide enough for a title wider than any item.
 **/
BOOST_AUTO_TEST_CASE(a_centred_menu_writes_its_title_above_its_items) {
    std::vector<Written> body;
    std::vector<Written> titles;
    const auto record = [](std::vector<Written>* into) {
        return [into](std::string_view text, const glm::vec2& pen, const glm::vec4& colour) {
            into->push_back({.text = std::string(text), .pen = pen, .colour = colour});
        };
    };
    v3d::ui::paint::ComponentRenderer renderer(
        [](std::string_view text) { return static_cast<float>(text.size()) * characterWidth; }, record(&body));
    renderer.dressing().shadow = glm::vec4(0.0F);

    v3d::render::realtime::canvas::Canvas untitled;
    untitled.resize(glm::uvec2(800, 600));
    const boost::shared_ptr<v3d::ui::component::menu::Menu> plain = buildMenu({"One", "Two"});
    renderer.draw(&untitled, plain);
    const float plainTop = plain->position().y;
    BOOST_REQUIRE_EQUAL(body.size(), 2U);
    const float firstPen = body[0].pen.y;
    body.clear();

    renderer.emphasis({.title = {
        .measure = [](std::string_view text) { return static_cast<float>(text.size()) * characterWidth * 2.0F; },
        .write = record(&titles), .scale = 2.0F}});
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(800, 600));
    const boost::shared_ptr<v3d::ui::component::menu::Menu> menu = buildMenu({"One", "Two"});
    menu->title("Paused");
    renderer.draw(&canvas, menu);

    BOOST_REQUIRE_EQUAL(titles.size(), 1U);
    BOOST_CHECK_EQUAL(titles[0].text, "Paused");
    BOOST_REQUIRE_EQUAL(body.size(), 2U);
    BOOST_CHECK_LT(titles[0].pen.y, body[0].pen.y);
    const v3d::ui::paint::Dressing& dress = renderer.dressing();
    const float band = (dress.lineHeight * 2.0F) + (dress.padding * 0.5F);
    // the menu grows by the band and stays centred, so its items move down by half of it
    BOOST_CHECK_CLOSE(menu->position().y, plainTop - (band * 0.5F), 0.01F);
    BOOST_CHECK_CLOSE(body[0].pen.y, firstPen + (band * 0.5F), 0.01F);
    const float width = static_cast<const v3d::ui::Component&>(*menu).size().x;
    BOOST_CHECK_CLOSE(width, (6.0F * characterWidth * 2.0F) + (dress.padding * 2.0F), 0.01F);
}

/**
 * The chosen item's label is written in the chosen item's setting and the others in the body
 * text, and the menu is as wide as the chosen setting makes its widest label.
 **/
BOOST_AUTO_TEST_CASE(the_chosen_item_is_written_in_its_own_setting) {
    std::vector<std::string> body;
    std::vector<std::string> chosen;
    v3d::ui::paint::ComponentRenderer renderer(
        [](std::string_view text) { return static_cast<float>(text.size()) * characterWidth; },
        [&body](std::string_view text, const glm::vec2&, const glm::vec4&) { body.emplace_back(text); });
    renderer.emphasis({.chosen = {
        .measure = [](std::string_view text) { return static_cast<float>(text.size()) * (characterWidth + 1.0F); },
        .write = [&chosen](std::string_view text, const glm::vec2&, const glm::vec4&) { chosen.emplace_back(text); }}});

    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(800, 600));
    const boost::shared_ptr<v3d::ui::component::menu::Menu> menu = buildMenu({"One", "Three"});
    menu->active(1);
    renderer.draw(&canvas, menu);

    BOOST_CHECK(chosen == std::vector<std::string>({"Three"}));
    BOOST_CHECK(body == std::vector<std::string>({"One"}));
    const float width = static_cast<const v3d::ui::Component&>(*menu).size().x;
    BOOST_CHECK_CLOSE(width, (5.0F * (characterWidth + 1.0F)) + (renderer.dressing().padding * 2.0F), 0.01F);
}

/**
 * The focus ring is drawn around the chosen item only, so a menu with nothing chosen draws
 * none.
 **/
BOOST_AUTO_TEST_CASE(the_chosen_item_is_ringed_in_the_focus_colour) {
    v3d::ui::paint::ComponentRenderer renderer(
        [](std::string_view text) { return static_cast<float>(text.size()) * characterWidth; },
        [](std::string_view, const glm::vec2&, const glm::vec4&) {});
    const glm::vec4 focus(0.0F, 1.0F, 0.0F, 1.0F);
    renderer.dressing().focus = focus;
    const auto ringed = [&focus](const v3d::render::realtime::canvas::Canvas& canvas) {
        return std::ranges::count_if(canvas.vertices(),
            [&focus](const v3d::render::realtime::canvas::Canvas::Vertex& vertex) { return vertex.colour == focus; });
    };

    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(800, 600));
    renderer.draw(&canvas, buildMenu({"One", "Two"}));
    BOOST_CHECK_EQUAL(ringed(canvas), 4 * 4);

    v3d::render::realtime::canvas::Canvas none;
    none.resize(glm::uvec2(800, 600));
    const boost::shared_ptr<v3d::ui::component::menu::Menu> unchosen = buildMenu({"One", "Two"});
    unchosen->active(-1);
    renderer.draw(&none, unchosen);
    BOOST_CHECK_EQUAL(ringed(none), 0);
}

/**
 * A menu too tall for the safe area starts at the safe area's top, rather than above it.
 **/
BOOST_AUTO_TEST_CASE(a_menu_too_tall_for_the_safe_area_starts_at_its_top) {
    const v3d::ui::paint::ComponentRenderer renderer(
        [](std::string_view text) { return static_cast<float>(text.size()) * characterWidth; },
        [](std::string_view, const glm::vec2&, const glm::vec4&) {});
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(800, 100));
    const boost::shared_ptr<v3d::ui::component::menu::Menu> menu = buildMenu({"One", "Two", "Three", "Four"});
    renderer.draw(&canvas, menu);
    BOOST_CHECK_CLOSE(menu->position().y, 100.0F * v3d::ui::Arranger::safeEnds, 0.01F);
}

/**
 * A settings row writes its label on the left and its value right-aligned, in secondary text
 * when the row is not the chosen one. The menu makes room for a column as wide as the widest
 * value any row can show, so it keeps its width as a value changes. A slider shows its value
 * between the arrows that step it, and a key input the key bound to the command it binds, or
 * an ellipsis while it waits on a key.
 **/
BOOST_AUTO_TEST_CASE(a_settings_row_shows_its_value_on_the_right) {
    std::vector<Written> written;
    v3d::ui::paint::ComponentRenderer renderer(
        [](std::string_view text) { return static_cast<float>(text.size()) * characterWidth; },
        [&written](std::string_view text, const glm::vec2& pen, const glm::vec4& colour) {
            written.push_back({.text = std::string(text), .pen = pen, .colour = colour});
        });
    renderer.shortcuts([](const v3d::event::Event& command) {
        return command.name() == "up" ? std::string("W") : std::string();
    });

    const boost::shared_ptr<entt::dispatcher> dispatcher = boost::make_shared<entt::dispatcher>();
    const boost::shared_ptr<v3d::ui::component::menu::Menu> menu = boost::make_shared<v3d::ui::component::menu::Menu>(dispatcher);
    const boost::shared_ptr<v3d::ui::component::menu::Item> quit =
        boost::make_shared<v3d::ui::component::menu::Item>(v3d::ui::component::menu::ItemType::Action, "Quit");
    const boost::shared_ptr<v3d::ui::component::menu::Item> rounds =
        boost::make_shared<v3d::ui::component::menu::Item>(v3d::ui::component::menu::ItemType::Slider, "Rounds");
    rounds->value(5);
    const boost::shared_ptr<v3d::ui::component::menu::Item> up =
        boost::make_shared<v3d::ui::component::menu::Item>(v3d::ui::component::menu::ItemType::KeyInput, "Up");
    const boost::shared_ptr<v3d::event::Context> ui = boost::make_shared<v3d::event::Context>("ui");
    up->event(v3d::event::Event("setUp", ui));
    up->binds(v3d::event::Event("up", boost::make_shared<v3d::event::Context>("pong")));
    menu->addItem(quit);
    menu->addItem(rounds);
    menu->addItem(up);
    menu->level(menu);
    menu->active(0);

    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(800, 600));
    renderer.draw(&canvas, menu);

    const v3d::ui::paint::Dressing& dress = renderer.dressing();
    const float right = menu->position().x + static_cast<const v3d::ui::Component&>(*menu).size().x - dress.padding;
    const auto find = [&written](const std::string& text) {
        return std::ranges::find_if(written, [&text](const Written& line) { return line.text == text; });
    };
    const auto slider = find("< 5 >");
    BOOST_REQUIRE(slider != written.end());
    BOOST_CHECK_CLOSE(slider->pen.x + (5.0F * characterWidth), right, 0.01F);
    BOOST_CHECK(slider->colour == dress.secondaryText);
    const auto key = find("W");
    BOOST_REQUIRE(key != written.end());
    BOOST_CHECK_CLOSE(key->pen.x + characterWidth, right, 0.01F);
    BOOST_CHECK(find("Rounds") != written.end());
    // the widest label, a column's gap and the widest value any row can show, inside the
    // padding. That value is "< 10 >", the slider's maximum, not the "< 5 >" it shows now
    const float width = static_cast<const v3d::ui::Component&>(*menu).size().x;
    BOOST_CHECK_CLOSE(width, (6.0F * characterWidth) + (dress.padding * 2.0F) + (6.0F * characterWidth) + (dress.padding * 2.0F), 0.01F);

    // stepping the slider to its widest value leaves the width as it was
    rounds->value(10);
    renderer.draw(&canvas, menu);
    BOOST_CHECK_CLOSE(static_cast<const v3d::ui::Component&>(*menu).size().x, width, 0.01F);

    // a capture on the key shows that it waits
    menu->active(2);
    menu->activate();
    BOOST_REQUIRE(menu->capturing());
    written.clear();
    renderer.draw(&canvas, menu);
    BOOST_CHECK(find("...") != written.end());
    BOOST_CHECK(find("W") == written.end());
    BOOST_CHECK_CLOSE(static_cast<const v3d::ui::Component&>(*menu).size().x, width, 0.01F);
}

/**
 * Text over the scene is one plate around its lines, placed by its anchor, in the panel's
 * colour as opaque as the plate opacity, with each line in the text colour inside the
 * padding. The plate's box comes back so the next block can be placed against it.
 **/
BOOST_AUTO_TEST_CASE(hud_text_sits_on_a_plate_placed_by_its_anchor) {
    std::vector<Written> written;
    v3d::ui::paint::ComponentRenderer renderer(
        [](std::string_view text) { return static_cast<float>(text.size()) * characterWidth; },
        [&written](std::string_view text, const glm::vec2& pen, const glm::vec4& colour) {
            written.push_back({.text = std::string(text), .pen = pen, .colour = colour});
        });
    v3d::ui::paint::Dressing& dress = renderer.dressing();
    dress.panel = glm::vec4(0.1F, 0.2F, 0.3F, 1.0F);
    dress.plateOpacity = 0.5F;
    dress.borderWidth = 0.0F;
    dress.radius = 0.0F;
    dress.padding = 4.0F;
    dress.lineHeight = 20.0F;

    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(800, 600));
    const v3d::type::geometry::Bound2D box = renderer.hud(&canvas, {"SCORE", "120"},
        {.at = glm::vec2(400.0F, 100.0F), .anchor = glm::vec2(0.5F, 0.0F)});

    // as wide as the wider line inside the padding, and a line high for each with half a
    // padding above and below
    BOOST_CHECK_CLOSE(box.size().x, (5.0F * characterWidth) + 8.0F, 0.001F);
    BOOST_CHECK_CLOSE(box.size().y, 44.0F, 0.001F);
    BOOST_CHECK_CLOSE(box.position().x, 400.0F - (box.size().x * 0.5F), 0.001F);
    BOOST_CHECK_CLOSE(box.position().y, 100.0F, 0.001F);

    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 4U);
    BOOST_CHECK(canvas.vertices()[0].colour == glm::vec4(0.1F, 0.2F, 0.3F, 0.5F));
    BOOST_REQUIRE_EQUAL(written.size(), 2U);
    BOOST_CHECK_EQUAL(written[1].text, "120");
    BOOST_CHECK(written[0].colour == dress.text);
    BOOST_CHECK_CLOSE(written[0].pen.x, box.position().x + 4.0F, 0.001F);
    BOOST_CHECK_CLOSE(written[1].pen.y - written[0].pen.y, 20.0F, 0.001F);
}

/**
 * A container that is not visible draws nothing. An app shows and hides its whole ui this
 * way, without taking it apart.
 **/
BOOST_AUTO_TEST_CASE(an_invisible_container_draws_nothing) {
    const v3d::ui::paint::ComponentRenderer renderer(
        [](std::string_view text) { return static_cast<float>(text.size()) * characterWidth; },
        [](std::string_view, const glm::vec2&, const glm::vec4&) {});

    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(800, 600));

    v3d::ui::Container container("game-menu", false);
    container.add(buildMenu({"One"}));

    // the container's own visibility is checked by the engine overload, so go through a
    // visible one to prove the component still draws, then an invisible one to prove it does not
    v3d::ui::Container visible("visible", true);
    visible.add(buildMenu({"One"}));
    renderer.draw(&canvas, visible);
    const std::size_t drawn = canvas.vertices().size();
    BOOST_CHECK(drawn > 0);

    canvas.clear();
    const boost::shared_ptr<v3d::ui::component::menu::Menu> hidden = buildMenu({"One"});
    hidden->visible(false);
    v3d::ui::Container withHidden("with-hidden", true);
    withHidden.add(hidden);
    renderer.draw(&canvas, withHidden);

    BOOST_CHECK(canvas.empty());
}

/**
 * A menu with no items draws no panel either.
 **/
BOOST_AUTO_TEST_CASE(an_empty_menu_draws_nothing) {
    const v3d::ui::paint::ComponentRenderer renderer(
        [](std::string_view text) { return static_cast<float>(text.size()) * characterWidth; },
        [](std::string_view, const glm::vec2&, const glm::vec4&) {});

    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(800, 600));

    renderer.draw(&canvas, buildMenu({}));

    BOOST_CHECK(canvas.empty());
}

/**
 * A label given a width draws a row per line, each a line below the last. One that was given
 * no width is one line however long it is.
 **/
BOOST_AUTO_TEST_CASE(a_label_with_a_width_draws_a_row_per_line) {
    std::vector<Written> written;
    const v3d::ui::paint::ComponentRenderer renderer(
        [](std::string_view text) { return static_cast<float>(text.size()) * characterWidth; },
        [&written](std::string_view text, const glm::vec2& pen, const glm::vec4& colour) {
            Written line;
            line.text = text;
            line.pen = pen;
            line.colour = colour;
            written.push_back(line);
        });

    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(800, 600));

    const boost::shared_ptr<v3d::ui::component::Label> wrapped =
        boost::make_shared<v3d::ui::component::Label>();
    wrapped->text("one two three four");
    wrapped->layout().width = v3d::ui::Length(100.0F, v3d::ui::Length::Unit::Pixels);
    wrapped->size(glm::vec2(100.0F, 40.0F));

    renderer.draw(&canvas, wrapped);
    BOOST_REQUIRE_EQUAL(written.size(), 2);
    BOOST_CHECK_EQUAL(written[0].text, "one two");
    BOOST_CHECK_EQUAL(written[1].text, "three four");
    const v3d::ui::style::Resolver styles;
    BOOST_CHECK_CLOSE(written[1].pen.y - written[0].pen.y, styles.base().lineHeight, 0.001F);

    written.clear();
    const boost::shared_ptr<v3d::ui::component::Label> single =
        boost::make_shared<v3d::ui::component::Label>();
    single->text("one two three four");
    renderer.draw(&canvas, single);
    BOOST_REQUIRE_EQUAL(written.size(), 1);
    BOOST_CHECK_EQUAL(written[0].text, "one two three four");
}

/**
 * A focused component is ringed, and an unfocused one is not.
 *
 * Drawn by the renderer rather than by each component's draw, so every control shows the
 * focus the same way. At the default radius of zero the ring is four straight runs, so it
 * is one quad per edge on top of whatever the component drew.
 **/
BOOST_AUTO_TEST_CASE(a_focused_component_is_ringed) {
    const v3d::ui::paint::ComponentRenderer renderer(
        [](std::string_view text) { return static_cast<float>(text.size()) * characterWidth; },
        [](std::string_view, const glm::vec2&, const glm::vec4&) {});

    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(800, 600));

    const boost::shared_ptr<v3d::ui::component::Button> button =
        boost::make_shared<v3d::ui::component::Button>();
    button->label("Start");
    button->layout().width = v3d::ui::Length(120.0F, v3d::ui::Length::Unit::Pixels);
    button->layout().height = v3d::ui::Length(30.0F, v3d::ui::Length::Unit::Pixels);

    v3d::ui::Container container("screen", true);
    container.add(button);

    renderer.draw(&canvas, container);
    const std::size_t plain = canvas.vertices().size();

    canvas.clear();
    button->focused(true);
    renderer.draw(&canvas, container);

    BOOST_CHECK_EQUAL(canvas.vertices().size(), plain + static_cast<std::size_t>(4 * 4));
    // untextured like everything else the ui draws, so the ring costs no batch of its own
    BOOST_CHECK_EQUAL(canvas.batches().size(), 1);
}

/**
 * The ring is read off the class the component is drawn in, so a theme can mark a focused text
 * box differently from every other control.
 *
 * A theme naming nothing still gets the base's ring for every control.
 **/
BOOST_AUTO_TEST_CASE(a_theme_rings_a_class_in_its_own_colour) {
    v3d::ui::paint::ComponentRenderer renderer(
        [](std::string_view text) { return static_cast<float>(text.size()) * characterWidth; },
        [](std::string_view, const glm::vec2&, const glm::vec4&) {});

    const glm::vec4 ringColour(0.0F, 1.0F, 0.0F, 1.0F);
    const boost::shared_ptr<v3d::ui::style::Theme> theme =
        boost::make_shared<v3d::ui::style::Theme>("dark");
    const boost::shared_ptr<v3d::ui::style::Style> field =
        boost::make_shared<v3d::ui::style::Style>(v3d::ui::style::Selector{.name = "default", .className = "textbox"});
    field->addProperty(boost::make_shared<v3d::ui::style::property::Color>("focus", ringColour), "color");
    field->addProperty(boost::make_shared<v3d::ui::style::property::Number>("focus-width", 2.0F), "number");
    theme->addStyle(field);
    // the base first: taking it by reference drops whatever has been resolved from it
    renderer.dressing().focus = glm::vec4(1.0F, 0.0F, 0.0F, 1.0F);
    renderer.theme(theme);

    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(800, 600));

    const boost::shared_ptr<v3d::ui::component::TextBox> box =
        boost::make_shared<v3d::ui::component::TextBox>();
    box->layout().width = v3d::ui::Length(120.0F, v3d::ui::Length::Unit::Pixels);
    box->layout().height = v3d::ui::Length(24.0F, v3d::ui::Length::Unit::Pixels);
    box->focused(true);

    v3d::ui::Container container("screen", true);
    container.add(box);
    renderer.draw(&canvas, container);

    // the ring is traced after the component it rings, so the last four quads are its
    const std::size_t ringVertices = static_cast<std::size_t>(4 * 4);
    BOOST_REQUIRE(canvas.vertices().size() >= ringVertices);
    for (std::size_t back = canvas.vertices().size() - ringVertices; back < canvas.vertices().size(); back++) {
        BOOST_CHECK(canvas.vertices()[back].colour == ringColour);
    }
}

/**
 * A control that is there and cannot be used is drawn saying so: its label is written in the
 * theme's disabled colour, and no hover it was left holding lights it.
 **/
BOOST_AUTO_TEST_CASE(a_disabled_button_is_written_in_the_disabled_colour) {
    std::vector<Written> written;
    v3d::ui::paint::ComponentRenderer renderer(
        [](std::string_view text) { return static_cast<float>(text.size()) * characterWidth; },
        [&written](std::string_view text, const glm::vec2& pen, const glm::vec4& colour) {
            Written line;
            line.text = text;
            line.pen = pen;
            line.colour = colour;
            written.push_back(line);
        });

    const glm::vec4 ordinary(0.8F, 0.8F, 0.8F, 1.0F);
    const glm::vec4 greyed(0.3F, 0.3F, 0.3F, 1.0F);
    renderer.dressing().text = ordinary;
    renderer.dressing().disabledText = greyed;

    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(800, 600));

    const boost::shared_ptr<v3d::ui::component::Button> button =
        boost::make_shared<v3d::ui::component::Button>();
    button->label("Continue");
    button->size(glm::vec2(120.0F, 30.0F));

    renderer.draw(&canvas, button);
    BOOST_REQUIRE_EQUAL(written.size(), 1U);
    BOOST_CHECK(written.front().colour == ordinary);

    written.clear();
    canvas.clear();
    button->enabled(false);
    // the state a cursor left on it before it was disabled, which must not light it now
    button->state(v3d::ui::component::Button::STATE_HOVER);
    renderer.draw(&canvas, button);

    BOOST_REQUIRE_EQUAL(written.size(), 1U);
    BOOST_CHECK(written.front().colour == greyed);
    // no highlight under it either: a disabled button is never lit
    BOOST_CHECK(canvas.empty());
}

/**
 * Disabling a box greys what it holds. Routing a point or a tab skips a disabled subtree and
 * never reaches it. Drawing reaches every component on its own, so a label inside a
 * disabled group has to check what is around it.
 **/
BOOST_AUTO_TEST_CASE(a_disabled_box_greys_the_label_it_holds) {
    std::vector<Written> written;
    v3d::ui::paint::ComponentRenderer renderer(
        [](std::string_view text) { return static_cast<float>(text.size()) * characterWidth; },
        [&written](std::string_view text, const glm::vec2& pen, const glm::vec4& colour) {
            Written line;
            line.text = text;
            line.pen = pen;
            line.colour = colour;
            written.push_back(line);
        });

    const glm::vec4 greyed(0.3F, 0.3F, 0.3F, 1.0F);
    renderer.dressing().text = glm::vec4(0.8F, 0.8F, 0.8F, 1.0F);
    renderer.dressing().disabledText = greyed;

    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(800, 600));

    const boost::shared_ptr<v3d::ui::component::Panel> group =
        boost::make_shared<v3d::ui::component::Panel>();
    group->layout().width = v3d::ui::Length(200.0F, v3d::ui::Length::Unit::Pixels);
    group->layout().height = v3d::ui::Length(100.0F, v3d::ui::Length::Unit::Pixels);
    group->enabled(false);

    const boost::shared_ptr<v3d::ui::component::Label> label =
        boost::make_shared<v3d::ui::component::Label>();
    label->text("Sound");
    group->add(label);

    v3d::ui::Container container("screen", true);
    container.add(group);
    renderer.draw(&canvas, container);

    BOOST_REQUIRE_EQUAL(written.size(), 1U);
    BOOST_CHECK(written.front().colour == greyed);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
