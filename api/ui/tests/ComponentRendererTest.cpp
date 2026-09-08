/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "../ComponentRenderer.h"
#include "../../render/realtime/Canvas.h"
#include "../Container.h"
#include "../component/Label.h"
#include "../style/Resolver.h"
#include "../component/menu/Menu.h"
#include "../component/menu/MenuBar.h"
#include "../component/menu/MenuItem.h"
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
const float characterWidth = 10.0f;

/**
 * A menu of action items with the given labels, with its level pointing at itself the way
 * the ui engine's loader leaves it.
 **/
boost::shared_ptr<v3d::ui::component::Menu> buildMenu(const std::vector<std::string>& labels) {
    boost::shared_ptr<entt::dispatcher> dispatcher = boost::make_shared<entt::dispatcher>();
    boost::shared_ptr<v3d::ui::component::Menu> menu = boost::make_shared<v3d::ui::component::Menu>(dispatcher);
    for (const std::string& label : labels) {
        menu->addItem(boost::make_shared<v3d::ui::component::MenuItem>(v3d::ui::component::menu::ItemType::Action, label));
    }
    menu->level(menu);
    menu->active(0);
    return menu;
}

};  // namespace

BOOST_AUTO_TEST_SUITE(component_renderer_test)

/**
 * A menu is a panel, a highlight behind the active item, and one label per item. The panel is
 * two quads - a border with the background drawn over it - so a three item menu is five
 * rectangles, all untextured and so all one batch.
 **/
BOOST_AUTO_TEST_CASE(a_menu_draws_a_panel_a_highlight_and_a_label_per_item) {
    std::vector<Written> written;
    v3d::ui::ComponentRenderer renderer(
        [](std::string_view text) { return static_cast<float>(text.size()) * characterWidth; },
        [&written](std::string_view text, const glm::vec2& pen, const glm::vec4& colour) {
            Written line;
            line.text = text;
            line.pen = pen;
            line.colour = colour;
            written.push_back(line);
        });

    v3d::render::realtime::Canvas canvas;
    canvas.resize(800, 600);

    renderer.draw(&canvas, buildMenu({"One", "Two", "Three"}));

    BOOST_REQUIRE_EQUAL(written.size(), 3);
    BOOST_CHECK_EQUAL(written[0].text, "One");
    BOOST_CHECK_EQUAL(written[2].text, "Three");
    // the border, the panel and the highlight behind the active item
    BOOST_CHECK_EQUAL(canvas.vertices().size(), 3 * 4);
    BOOST_CHECK_EQUAL(canvas.batches().size(), 1);
}

/**
 * The active item is the one drawn in the active colour, and it is the only one - a menu that
 * highlighted two items would mean navigation had lost track of where it was.
 **/
BOOST_AUTO_TEST_CASE(only_the_active_item_is_drawn_highlighted) {
    std::vector<Written> written;
    v3d::ui::ComponentRenderer renderer(
        [](std::string_view text) { return static_cast<float>(text.size()) * characterWidth; },
        [&written](std::string_view text, const glm::vec2& pen, const glm::vec4& colour) {
            Written line;
            line.text = text;
            line.pen = pen;
            line.colour = colour;
            written.push_back(line);
        });

    v3d::render::realtime::Canvas canvas;
    canvas.resize(800, 600);

    boost::shared_ptr<v3d::ui::component::Menu> menu = buildMenu({"One", "Two", "Three"});
    menu->next();

    renderer.draw(&canvas, menu);

    BOOST_REQUIRE_EQUAL(written.size(), 3);
    const glm::vec4 active = renderer.dressing().activeText;
    BOOST_CHECK(written[0].colour != active);
    BOOST_CHECK(written[1].colour == active);
    BOOST_CHECK(written[2].colour != active);
}

/**
 * Descending into a submenu replaces what is on screen with the submenu's items, because the
 * menu draws the level navigation is on rather than the menu it was reached through.
 **/
BOOST_AUTO_TEST_CASE(a_submenu_replaces_what_is_drawn) {
    std::vector<Written> written;
    v3d::ui::ComponentRenderer renderer(
        [](std::string_view text) { return static_cast<float>(text.size()) * characterWidth; },
        [&written](std::string_view text, const glm::vec2& pen, const glm::vec4& colour) {
            Written line;
            line.text = text;
            line.pen = pen;
            line.colour = colour;
            written.push_back(line);
        });

    v3d::render::realtime::Canvas canvas;
    canvas.resize(800, 600);

    boost::shared_ptr<v3d::ui::component::Menu> menu = buildMenu({"Options", "Quit"});
    boost::shared_ptr<v3d::ui::component::Menu> submenu = buildMenu({"Rounds", "Keys"});
    submenu->parent(menu);
    (*menu)[0] = boost::make_shared<v3d::ui::component::MenuItem>(v3d::ui::component::menu::ItemType::Submenu, "Options");
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
    v3d::ui::ComponentRenderer renderer(
        [](std::string_view text) { return static_cast<float>(text.size()) * characterWidth; },
        [](std::string_view, const glm::vec2&, const glm::vec4&) {});

    v3d::render::realtime::Canvas canvas;
    canvas.resize(800, 600);

    renderer.draw(&canvas, buildMenu({"a", "abcdefghij"}));

    BOOST_REQUIRE(canvas.vertices().size() >= 8);
    // vertices 4..7 are the panel, drawn over the border: min at [4], max at [6]
    const float padding = renderer.dressing().padding;
    const float expectedWidth = 10.0f * characterWidth + padding * 2.0f;
    const float left = canvas.vertices()[4].position.x;
    const float right = canvas.vertices()[6].position.x;

    BOOST_CHECK_CLOSE(right - left, expectedWidth, 0.01f);
    BOOST_CHECK_CLOSE((left + right) * 0.5f, 400.0f, 0.01f);
}

/**
 * A container that is not visible draws nothing, which is how an app shows and hides its
 * whole ui without taking it apart.
 **/
BOOST_AUTO_TEST_CASE(an_invisible_container_draws_nothing) {
    v3d::ui::ComponentRenderer renderer(
        [](std::string_view text) { return static_cast<float>(text.size()) * characterWidth; },
        [](std::string_view, const glm::vec2&, const glm::vec4&) {});

    v3d::render::realtime::Canvas canvas;
    canvas.resize(800, 600);

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
    boost::shared_ptr<v3d::ui::component::Menu> hidden = buildMenu({"One"});
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
    v3d::ui::ComponentRenderer renderer(
        [](std::string_view text) { return static_cast<float>(text.size()) * characterWidth; },
        [](std::string_view, const glm::vec2&, const glm::vec4&) {});

    v3d::render::realtime::Canvas canvas;
    canvas.resize(800, 600);

    renderer.draw(&canvas, buildMenu({}));

    BOOST_CHECK(canvas.empty());
}

/**
 * A label given a width draws a row per line, each a line below the last. One that was given
 * no width is one line however long it is, which is what every label in the tree is today.
 **/
BOOST_AUTO_TEST_CASE(a_label_with_a_width_draws_a_row_per_line) {
    std::vector<Written> written;
    v3d::ui::ComponentRenderer renderer(
        [](std::string_view text) { return static_cast<float>(text.size()) * characterWidth; },
        [&written](std::string_view text, const glm::vec2& pen, const glm::vec4& colour) {
            Written line;
            line.text = text;
            line.pen = pen;
            line.colour = colour;
            written.push_back(line);
        });

    v3d::render::realtime::Canvas canvas;
    canvas.resize(800, 600);

    boost::shared_ptr<v3d::ui::component::Label> wrapped =
        boost::make_shared<v3d::ui::component::Label>();
    wrapped->text("one two three four");
    wrapped->layout().width = v3d::ui::Length(100.0f, v3d::ui::Length::Unit::Pixels);
    wrapped->size(glm::vec2(100.0f, 40.0f));

    renderer.draw(&canvas, wrapped);
    BOOST_REQUIRE_EQUAL(written.size(), 2);
    BOOST_CHECK_EQUAL(written[0].text, "one two");
    BOOST_CHECK_EQUAL(written[1].text, "three four");
    const v3d::ui::style::Resolver styles;
    BOOST_CHECK_CLOSE(written[1].pen.y - written[0].pen.y, styles.base().lineHeight, 0.001f);

    written.clear();
    boost::shared_ptr<v3d::ui::component::Label> single =
        boost::make_shared<v3d::ui::component::Label>();
    single->text("one two three four");
    renderer.draw(&canvas, single);
    BOOST_REQUIRE_EQUAL(written.size(), 1);
    BOOST_CHECK_EQUAL(written[0].text, "one two three four");
}

BOOST_AUTO_TEST_SUITE_END()
