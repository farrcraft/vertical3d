/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/event/Context.h>
#include <api/event/Event.h>
#include <api/render/realtime/resource/Handle.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/type/geometry/Bound2D.h>
#include <api/ui/Arranger.h>
#include <api/ui/Component.h>
#include <api/ui/Image.h>
#include <api/ui/Layout.h>
#include <api/ui/Length.h>
#include <api/ui/component/menu/Menu.h>
#include <api/ui/component/menu/Bar.h>
#include <api/ui/component/menu/Item.h>
#include <api/ui/component/menu/Type.h>
#include <api/ui/paint/ComponentRenderer.h>
#include <api/ui/paint/Dressing.h>
#include <api/ui/paint/Text.h>
#include <api/ui/paint/Type.h>
#include <api/ui/tests/Typeset.h>

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
 * A fixed width per character, so a label's width is predictable.
 **/
const float characterWidth = 10.0F;

/**
 * A line the ui drew.
 **/
using Written = v3d::ui::typeset::Written;

/**
 * The dispatcher, the events sent to it, and the bar they came from, all of which have to
 * outlive each other in that order.
 **/
struct Fixture final {
    Fixture() :
        dispatcher(boost::make_shared<entt::dispatcher>()),
        context(boost::make_shared<v3d::event::Context>("test")),
        bar(boost::make_shared<v3d::ui::component::menu::Bar>()),
        renderer(v3d::ui::typeset::body(&written)) {
        dispatcher->sink<v3d::event::Event>().connect<&Fixture::receive>(*this);
        canvas.resize(glm::uvec2(800, 600));
    }

    void receive(const v3d::event::Event& event) {
        sent.push_back(event.str());
    }

    /**
     * An item bound to "test::<name>", or to nothing when the name is empty.
     **/
    /** What an item shows, and the name of the command it is bound to. **/
    struct Labelled {
        std::string label;
        std::string name;
    };

    boost::shared_ptr<v3d::ui::component::menu::Item> item(v3d::ui::component::menu::ItemType type, const Labelled& spec) {
        boost::shared_ptr<v3d::ui::component::menu::Item> made =
            boost::make_shared<v3d::ui::component::menu::Item>(type, spec.label);
        if (!spec.name.empty()) {
            made->event(v3d::event::Event(spec.name, context));
        }
        return made;
    }

    boost::shared_ptr<v3d::ui::component::menu::Menu> menu() {
        return boost::make_shared<v3d::ui::component::menu::Menu>(dispatcher);
    }

    /**
     * Draw the bar, which is also what lays it out.
     **/
    void draw() {
        written.clear();
        canvas.clear();
        renderer.draw(&canvas, bar);
    }

    /**
     * The middle of a component. A test clicks a component there.
     **/
    static glm::vec2 centre(const v3d::ui::Component& component) {
        return component.position() + component.size() * 0.5F;
    }

    /**
     * The middle of a menu's label in the strip, which the bar holds rather than the menu.
     **/
    glm::vec2 label(std::size_t index) const {
        const v3d::type::geometry::Bound2D bounds = bar->bound(index);
        return bounds.position() + bounds.size() * 0.5F;
    }

    boost::shared_ptr<entt::dispatcher> dispatcher;
    boost::shared_ptr<v3d::event::Context> context;
    boost::shared_ptr<v3d::ui::component::menu::Bar> bar;
    v3d::render::realtime::canvas::Canvas canvas;
    std::vector<Written> written;
    std::vector<std::string> sent;
    v3d::ui::paint::ComponentRenderer renderer;
};

/**
 * A bar of two menus: "File" holding an action and a checked item, and "View" holding a
 * submenu that holds one action.
 **/
void build(Fixture* fixture) {
    const boost::shared_ptr<v3d::ui::component::menu::Menu> file = fixture->menu();
    file->addItem(fixture->item(v3d::ui::component::menu::ItemType::Action, {.label = "Open", .name = "open"}));
    file->addItem(fixture->item(v3d::ui::component::menu::ItemType::Check, {.label = "Grid", .name = "grid"}));

    const boost::shared_ptr<v3d::ui::component::menu::Menu> shading = fixture->menu();
    shading->addItem(fixture->item(v3d::ui::component::menu::ItemType::Action, {.label = "Flat", .name = "flat"}));

    const boost::shared_ptr<v3d::ui::component::menu::Item> deeper =
        fixture->item(v3d::ui::component::menu::ItemType::Submenu, {.label = "Shading", .name = ""});
    const boost::shared_ptr<v3d::ui::component::menu::Menu> view = fixture->menu();
    deeper->menu(view);
    deeper->submenu(shading);
    view->addItem(deeper);

    fixture->bar->add("File", file);
    fixture->bar->add("View", view);
}

BOOST_AUTO_TEST_SUITE(menu_bar_test)

/**
 * A closed bar is the strip and one label per menu, and nothing of what the menus hold.
 **/
BOOST_AUTO_TEST_CASE(a_closed_bar_draws_only_its_labels) {
    Fixture fixture;
    build(&fixture);
    fixture.draw();

    BOOST_REQUIRE_EQUAL(fixture.written.size(), 2);
    BOOST_CHECK_EQUAL(fixture.written[0].text, "File");
    BOOST_CHECK_EQUAL(fixture.written[1].text, "View");
    // the strip and the rule under it, and no panel
    BOOST_CHECK_EQUAL(v3d::ui::typeset::shapeVertices(fixture.canvas).size(), 2 * 4);
}

/**
 * A bar's strip is drawn at its opacity, which is held between none and whole.
 **/
BOOST_AUTO_TEST_CASE(a_bar_strip_is_drawn_at_its_opacity) {
    Fixture fixture;
    build(&fixture);
    fixture.bar->opacity(0.8F);
    fixture.draw();
    BOOST_CHECK_CLOSE(v3d::ui::typeset::shapeVertices(fixture.canvas)[0].colour.a, fixture.renderer.dressing().panel.a * 0.8F, 0.001F);

    fixture.bar->opacity(2.0F);
    BOOST_CHECK_EQUAL(fixture.bar->opacity(), 1.0F);
    fixture.bar->opacity(-1.0F);
    BOOST_CHECK_EQUAL(fixture.bar->opacity(), 0.0F);
}

/**
 * A title with a drop mark is wider by the mark and the gap before it, and draws the mark as
 * two strokes after its label.
 **/
BOOST_AUTO_TEST_CASE(a_drop_mark_follows_its_title) {
    Fixture fixture;
    build(&fixture);
    fixture.draw();
    const float plain = fixture.bar->bound(0).size().x;
    const std::size_t drawn = v3d::ui::typeset::shapeVertices(fixture.canvas).size();

    fixture.bar->menu(0)->drop(true);
    fixture.draw();
    BOOST_CHECK_GT(fixture.bar->bound(0).size().x, plain);
    BOOST_CHECK_EQUAL(v3d::ui::typeset::shapeVertices(fixture.canvas).size(), drawn + std::size_t{8});
}

/**
 * A title with a resolved icon draws the icon in place of its label, tinted the label's colour,
 * and is as wide as the icon and the padding.
 **/
BOOST_AUTO_TEST_CASE(an_icon_title_draws_in_place_of_its_label) {
    Fixture fixture;
    build(&fixture);
    fixture.renderer.dressing().iconSize = 16.0F;
    fixture.bar->menu(0)->icon("icon:viewport");
    fixture.bar->menu(0)->image(v3d::ui::Image(v3d::render::realtime::resource::TextureHandle(3)));
    fixture.draw();

    BOOST_REQUIRE_EQUAL(fixture.written.size(), 1U);
    BOOST_CHECK_EQUAL(fixture.written[0].text, "View");
    const v3d::ui::paint::Dressing& dressing = fixture.renderer.dressing();
    BOOST_CHECK_CLOSE(fixture.bar->bound(0).size().x, std::min(dressing.iconSize, dressing.barHeight) + dressing.padding, 0.001F);
    BOOST_CHECK(std::ranges::any_of(v3d::ui::typeset::shapes(fixture.canvas), [](const auto& batch) { return batch.texture.id() == 3U; }));
}

/**
 * A named icon's title shows its label too, semibold, after the icon. The title is wider by the
 * label and a gap.
 **/
BOOST_AUTO_TEST_CASE(a_named_icon_title_shows_its_label_after_the_icon) {
    Fixture fixture;
    build(&fixture);
    fixture.bar->menu(0)->image(v3d::ui::Image(v3d::render::realtime::resource::TextureHandle(3)));
    fixture.draw();
    const float plain = fixture.bar->bound(0).size().x;
    BOOST_CHECK(std::ranges::none_of(fixture.written, [](const auto& line) { return line.text == "File"; }));

    fixture.bar->menu(0)->named(true);
    fixture.written.clear();
    fixture.draw();
    BOOST_CHECK_GT(fixture.bar->bound(0).size().x, plain);
    const auto file = std::ranges::find_if(fixture.written, [](const auto& line) { return line.text == "File"; });
    BOOST_REQUIRE(file != fixture.written.end());
    BOOST_CHECK(file->weight == v3d::ui::paint::Weight::SemiBold);
    BOOST_CHECK_GT(file->pen.x, fixture.bar->bound(0).position().x);
}

/**
 * A bar's note is drawn after its last title in the secondary text colour, and an empty one
 * draws nothing.
 **/
BOOST_AUTO_TEST_CASE(a_note_follows_the_last_title) {
    Fixture fixture;
    build(&fixture);
    fixture.draw();
    const std::size_t plain = fixture.written.size();

    fixture.bar->note("Ctrl+Space restores the areas");
    fixture.written.clear();
    fixture.draw();
    BOOST_REQUIRE_EQUAL(fixture.written.size(), plain + 1U);
    const auto& note = fixture.written.back();
    BOOST_CHECK_EQUAL(note.text, "Ctrl+Space restores the areas");
    BOOST_CHECK(note.colour == fixture.renderer.dressing().secondaryText);
    const v3d::type::geometry::Bound2D last = fixture.bar->bound(1);
    BOOST_CHECK_GE(note.pen.x, last.position().x + last.size().x);
}

/**
 * A title's icon takes the label's colour, or the accent when the menu asks for it.
 **/
BOOST_AUTO_TEST_CASE(an_icon_title_may_take_the_accent) {
    Fixture fixture;
    build(&fixture);
    fixture.bar->menu(0)->image(v3d::ui::Image(v3d::render::realtime::resource::TextureHandle(3)));
    const glm::vec4 accent = fixture.renderer.dressing().fill;
    const auto tinted = [&fixture, &accent]() {
        return std::ranges::any_of(v3d::ui::typeset::shapeVertices(fixture.canvas), [&accent](const auto& vertex) { return vertex.colour == accent; });
    };
    fixture.draw();
    BOOST_CHECK(!tinted());

    fixture.bar->menu(0)->accent(true);
    fixture.draw();
    BOOST_CHECK(tinted());
}

/**
 * A title set apart is written in the chosen item's setting, semibold, on a pill of the widget
 * colour.
 **/
BOOST_AUTO_TEST_CASE(a_title_set_apart_is_semibold_on_a_pill) {
    Fixture fixture;
    build(&fixture);
    v3d::ui::paint::Emphasis sets;
    sets.chosen.type = v3d::ui::typeset::body(&fixture.written).in(v3d::ui::paint::Weight::SemiBold);
    fixture.renderer.emphasis(sets);
    fixture.bar->menu(1)->emphasis(true);
    fixture.draw();

    BOOST_REQUIRE_EQUAL(fixture.written.size(), 2U);
    BOOST_CHECK_EQUAL(fixture.written[0].text, "File");
    BOOST_CHECK(fixture.written[0].weight == v3d::ui::paint::Weight::Regular);
    BOOST_CHECK_EQUAL(fixture.written[1].text, "View");
    BOOST_CHECK(fixture.written[1].weight == v3d::ui::paint::Weight::SemiBold);
    const glm::vec4 widget = fixture.renderer.dressing().widget;
    BOOST_CHECK(std::ranges::any_of(v3d::ui::typeset::shapeVertices(fixture.canvas), [&widget](const auto& vertex) { return vertex.colour == widget; }));
}

/**
 * A bar that is not docked is drawn where its layout puts it, its labels are hit there, and
 * its panel drops from its own bottom edge.
 **/
BOOST_AUTO_TEST_CASE(an_undocked_bar_sits_where_its_layout_puts_it) {
    Fixture fixture;
    build(&fixture);
    fixture.bar->docked(false);
    v3d::ui::Layout& layout = fixture.bar->layout();
    layout.x = v3d::ui::Length(100.0F, v3d::ui::Length::Unit::Pixels);
    layout.y = v3d::ui::Length(200.0F, v3d::ui::Length::Unit::Pixels);
    layout.width = v3d::ui::Length(300.0F, v3d::ui::Length::Unit::Pixels);
    layout.height = v3d::ui::Length(28.0F, v3d::ui::Length::Unit::Pixels);
    fixture.draw();

    BOOST_CHECK(fixture.bar->position() == glm::vec2(100.0F, 200.0F));
    BOOST_CHECK(fixture.bar->size() == glm::vec2(300.0F, 28.0F));
    BOOST_CHECK_GT(fixture.bar->bound(0).position().x, 100.0F);
    BOOST_CHECK_EQUAL(fixture.bar->bound(0).position().y, 200.0F);
    BOOST_REQUIRE_EQUAL(fixture.written.size(), 2U);
    BOOST_CHECK_GT(fixture.written[0].pen.y, 200.0F);

    BOOST_CHECK(!fixture.bar->press(glm::vec2(20.0F, 10.0F)));
    BOOST_REQUIRE(fixture.bar->press(fixture.label(0)));
    fixture.draw();
    BOOST_REQUIRE_EQUAL(fixture.bar->panels().size(), 1U);
    BOOST_CHECK_GE(fixture.bar->panels()[0]->position().y, 228.0F);
}

/**
 * Nothing is hit until something has been drawn: the bounds a click is tested against are
 * what the last draw left on the components.
 **/
BOOST_AUTO_TEST_CASE(a_bar_that_has_not_been_drawn_takes_no_press) {
    Fixture fixture;
    build(&fixture);

    BOOST_CHECK(!fixture.bar->press(glm::vec2(20.0F, 10.0F)));
    BOOST_CHECK(!fixture.bar->active());
}

/**
 * A closed bar takes a press only on a label, so the rest of its band is left for what sits
 * beside the labels. An open bar takes a press there.
 **/
BOOST_AUTO_TEST_CASE(a_closed_bar_takes_a_press_only_on_a_label) {
    Fixture fixture;
    build(&fixture);
    fixture.draw();

    const glm::vec2 beside = fixture.bar->position() + glm::vec2(fixture.bar->size().x - 2.0F, fixture.bar->size().y / 2.0F);
    const v3d::type::geometry::Bound2D last = fixture.bar->bound(fixture.bar->count() - 1);
    BOOST_REQUIRE_GT(beside.x, last.position().x + last.size().x);
    BOOST_CHECK(!fixture.bar->press(beside));
    BOOST_CHECK(fixture.bar->press(fixture.label(0)));
    BOOST_CHECK(fixture.bar->press(beside));
}

/**
 * A press on a label opens that menu, and a second press on the same label closes it.
 **/
BOOST_AUTO_TEST_CASE(a_press_on_a_label_opens_and_closes_the_menu) {
    Fixture fixture;
    build(&fixture);
    fixture.draw();

    const glm::vec2 file = fixture.label(0);
    BOOST_CHECK(fixture.bar->press(file));
    BOOST_CHECK_EQUAL(fixture.bar->open(), 0);
    BOOST_REQUIRE_EQUAL(fixture.bar->panels().size(), 1);

    fixture.draw();
    BOOST_REQUIRE_EQUAL(fixture.written.size(), 4);
    BOOST_CHECK_EQUAL(fixture.written[2].text, "Open");
    BOOST_CHECK_EQUAL(fixture.written[3].text, "Grid");

    BOOST_CHECK(fixture.bar->press(file));
    BOOST_CHECK_EQUAL(fixture.bar->open(), -1);
    BOOST_CHECK(fixture.bar->panels().empty());
}

/**
 * Sliding across the bar with a menu open opens the one the cursor reaches, without a press.
 **/
BOOST_AUTO_TEST_CASE(the_cursor_slides_between_open_menus) {
    Fixture fixture;
    build(&fixture);
    fixture.draw();

    fixture.bar->press(fixture.label(0));
    BOOST_CHECK_EQUAL(fixture.bar->open(), 0);

    fixture.draw();
    BOOST_CHECK(fixture.bar->motion(fixture.label(1)));
    BOOST_CHECK_EQUAL(fixture.bar->open(), 1);

    // and a closed bar only hovers, because sliding across it is not what opens it
    fixture.bar->close();
    fixture.draw();
    BOOST_CHECK(fixture.bar->motion(fixture.label(0)));
    BOOST_CHECK_EQUAL(fixture.bar->hover(), 0);
    BOOST_CHECK(!fixture.bar->active());
}

/**
 * A press on an action item sends that item's command and closes the bar, so the click that
 * invokes a menu item does not leave the menu standing open over what it did.
 **/
BOOST_AUTO_TEST_CASE(a_press_on_an_item_sends_its_command_and_closes) {
    Fixture fixture;
    build(&fixture);
    fixture.draw();

    fixture.bar->press(fixture.label(0));
    fixture.draw();

    const boost::shared_ptr<v3d::ui::component::menu::Menu> panel = fixture.bar->panels().front();
    BOOST_CHECK(fixture.bar->press(Fixture::centre(*(*panel)[0])));

    BOOST_REQUIRE_EQUAL(fixture.sent.size(), 1);
    BOOST_CHECK_EQUAL(fixture.sent[0], "test::open");
    BOOST_CHECK(!fixture.bar->active());
}

/**
 * The cursor crossing a submenu item drops its flyout, out of the right hand edge of the
 * panel it came from and level with the item.
 **/
BOOST_AUTO_TEST_CASE(the_cursor_opens_a_flyout_from_a_submenu_item) {
    Fixture fixture;
    build(&fixture);
    fixture.draw();

    fixture.bar->press(fixture.label(1));
    fixture.draw();

    const boost::shared_ptr<v3d::ui::component::menu::Menu> panel = fixture.bar->panels().front();
    const boost::shared_ptr<v3d::ui::component::menu::Item> shading = (*panel)[0];
    BOOST_CHECK(fixture.bar->motion(Fixture::centre(*shading)));
    BOOST_REQUIRE_EQUAL(fixture.bar->panels().size(), 2);

    fixture.draw();
    const boost::shared_ptr<v3d::ui::component::menu::Menu>& flyout = fixture.bar->panels().back();
    const v3d::type::geometry::Bound2D bounds = panel->bound();
    BOOST_CHECK_CLOSE(flyout->position().x, bounds.position().x + bounds.size().x, 0.01F);
    BOOST_CHECK_CLOSE(flyout->position().y, shading->position().y, 0.01F);

    // the item in the flyout is drawn, and a press on it sends its command
    BOOST_CHECK(fixture.bar->press(Fixture::centre(*(*flyout)[0])));
    BOOST_REQUIRE_EQUAL(fixture.sent.size(), 1);
    BOOST_CHECK_EQUAL(fixture.sent[0], "test::flat");
}

/**
 * With a clock, a submenu waits for the cursor to rest on its item for the delay before it
 * opens. The wait starts on the first frame drawn after the cursor arrives.
 **/
BOOST_AUTO_TEST_CASE(a_submenu_waits_for_the_cursor_to_rest) {
    Fixture fixture;
    build(&fixture);
    fixture.renderer.dressing().submenuDelay = 0.2F;
    const auto drawAt = [&fixture](float now, std::uint64_t frame) {
        fixture.renderer.clock({.now = now, .frame = frame});
        fixture.draw();
    };
    drawAt(0.0F, 1);
    fixture.bar->press(fixture.label(1));
    drawAt(0.0F, 2);

    const boost::shared_ptr<v3d::ui::component::menu::Menu> panel = fixture.bar->panels().front();
    fixture.bar->motion(Fixture::centre(*(*panel)[0]));
    BOOST_CHECK_EQUAL(fixture.bar->panels().size(), 1U);
    drawAt(1.0F, 3);
    drawAt(1.15F, 4);
    BOOST_CHECK_EQUAL(fixture.bar->panels().size(), 1U);
    drawAt(1.25F, 5);
    BOOST_CHECK_EQUAL(fixture.bar->panels().size(), 2U);
}

/**
 * A press on a submenu's item opens it at once, whatever the delay.
 **/
BOOST_AUTO_TEST_CASE(a_press_opens_a_submenu_at_once) {
    Fixture fixture;
    build(&fixture);
    fixture.renderer.dressing().submenuDelay = 0.2F;
    fixture.renderer.clock({.now = 0.0F, .frame = 1});
    fixture.draw();
    fixture.bar->press(fixture.label(1));
    fixture.draw();

    const boost::shared_ptr<v3d::ui::component::menu::Menu> panel = fixture.bar->panels().front();
    fixture.bar->press(Fixture::centre(*(*panel)[0]));
    BOOST_CHECK_EQUAL(fixture.bar->panels().size(), 2U);
}

/**
 * A separator takes half a row, draws a rule and no text, and the cursor passes over it
 * without choosing it.
 **/
BOOST_AUTO_TEST_CASE(a_separator_is_a_rule_nothing_can_choose) {
    Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::menu::Menu> file = fixture.menu();
    file->addItem(fixture.item(v3d::ui::component::menu::ItemType::Action, {.label = "Open", .name = "open"}));
    file->addItem(fixture.item(v3d::ui::component::menu::ItemType::Separator, {.label = "", .name = ""}));
    file->addItem(fixture.item(v3d::ui::component::menu::ItemType::Action, {.label = "Quit", .name = "quit"}));
    fixture.bar->add("File", file);
    fixture.draw();
    fixture.bar->press(fixture.label(0));
    fixture.draw();

    const float line = fixture.renderer.dressing().lineHeight;
    const boost::shared_ptr<v3d::ui::component::menu::Item> rule = (*file)[1];
    BOOST_CHECK_CLOSE(rule->size().y, line * 0.5F, 0.01F);
    BOOST_CHECK_CLOSE((*file)[2]->position().y, (*file)[0]->position().y + (line * 1.5F), 0.01F);
    for (const Written& drawn : fixture.written) {
        BOOST_CHECK(!drawn.text.empty());
    }

    fixture.bar->motion(Fixture::centre(*rule));
    BOOST_CHECK(!file->active());
    BOOST_CHECK(fixture.bar->press(Fixture::centre(*rule)));
    BOOST_CHECK(fixture.sent.empty());
}

/**
 * The keys step over a separator, at either end of the menu too.
 **/
BOOST_AUTO_TEST_CASE(the_keys_step_over_a_separator) {
    Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::menu::Menu> menu = fixture.menu();
    menu->addItem(fixture.item(v3d::ui::component::menu::ItemType::Separator, {.label = "", .name = ""}));
    menu->addItem(fixture.item(v3d::ui::component::menu::ItemType::Action, {.label = "One", .name = "one"}));
    menu->addItem(fixture.item(v3d::ui::component::menu::ItemType::Separator, {.label = "", .name = ""}));
    menu->addItem(fixture.item(v3d::ui::component::menu::ItemType::Action, {.label = "Two", .name = "two"}));
    menu->level(menu);
    menu->active(1);

    menu->next();
    BOOST_CHECK_EQUAL(menu->active()->label(), "Two");
    menu->next();
    BOOST_CHECK_EQUAL(menu->active()->label(), "One");
    menu->previous();
    BOOST_CHECK_EQUAL(menu->active()->label(), "Two");
    menu->previous();
    BOOST_CHECK_EQUAL(menu->active()->label(), "One");
}

/**
 * An item shows the shortcut of its command right-aligned against the arrow column, and the
 * panel widens to make room for it.
 **/
BOOST_AUTO_TEST_CASE(an_item_shows_its_shortcut) {
    Fixture fixture;
    build(&fixture);
    fixture.draw();
    fixture.bar->press(fixture.label(0));
    fixture.draw();
    const boost::shared_ptr<v3d::ui::component::menu::Menu> file = fixture.bar->panels().front();
    const float narrow = static_cast<const v3d::ui::Component&>(*file).size().x;

    fixture.renderer.shortcuts([](const v3d::event::Event& command) {
        return command.str() == "test::open" ? std::string("Ctrl+O") : std::string();
    });
    fixture.draw();
    const glm::vec2 corner = file->position();
    const glm::vec2 size = static_cast<const v3d::ui::Component&>(*file).size();
    BOOST_CHECK_GT(size.x, narrow);

    const auto keys = std::ranges::find_if(fixture.written, [](const Written& line) { return line.text == "Ctrl+O"; });
    BOOST_REQUIRE(keys != fixture.written.end());
    const float column = fixture.renderer.dressing().lineHeight * v3d::ui::Arranger::markColumn;
    BOOST_CHECK_CLOSE(keys->pen.x + (6.0F * characterWidth), corner.x + size.x - column, 0.01F);
    BOOST_CHECK_CLOSE(keys->pen.y, (*file)[0]->position().y + (fixture.renderer.dressing().lineHeight * 0.75F), 0.01F);
    BOOST_CHECK(keys->colour == fixture.renderer.dressing().secondaryText);

    // on the chosen row it is drawn in the active text, which reads against the highlight
    fixture.bar->motion(Fixture::centre(*(*file)[0]));
    fixture.draw();
    const auto chosen = std::ranges::find_if(fixture.written, [](const Written& line) { return line.text == "Ctrl+O"; });
    BOOST_REQUIRE(chosen != fixture.written.end());
    BOOST_CHECK(chosen->colour == fixture.renderer.dressing().activeText);
}

/**
 * Moving off a submenu item onto one of its siblings closes the flyout, so two of them never
 * stand open over each other.
 **/
BOOST_AUTO_TEST_CASE(leaving_a_submenu_item_closes_its_flyout) {
    Fixture fixture;
    build(&fixture);
    fixture.draw();

    fixture.bar->press(fixture.label(0));
    fixture.draw();
    const boost::shared_ptr<v3d::ui::component::menu::Menu> file = fixture.bar->panels().front();

    // File holds no submenu, so crossing either of its items leaves one panel standing
    fixture.bar->motion(Fixture::centre(*(*file)[0]));
    BOOST_CHECK_EQUAL(fixture.bar->panels().size(), 1);

    fixture.bar->press(fixture.label(1));
    fixture.draw();
    const boost::shared_ptr<v3d::ui::component::menu::Menu> view = fixture.bar->panels().front();
    fixture.bar->motion(Fixture::centre(*(*view)[0]));
    BOOST_CHECK_EQUAL(fixture.bar->panels().size(), 2);

    // back onto the strip, which is not one of the panels
    fixture.draw();
    fixture.bar->motion(fixture.label(1));
    BOOST_CHECK_EQUAL(fixture.bar->panels().size(), 1);
}

/**
 * A press away from an open bar closes it and is taken, so the click that dismisses a menu
 * does nothing else. The same press with the bar closed is not taken at all.
 **/
BOOST_AUTO_TEST_CASE(a_press_elsewhere_dismisses_an_open_bar_and_nothing_more) {
    Fixture fixture;
    build(&fixture);
    fixture.draw();

    const glm::vec2 elsewhere(400.0F, 400.0F);
    BOOST_CHECK(!fixture.bar->press(elsewhere));

    fixture.bar->press(fixture.label(0));
    fixture.draw();
    BOOST_CHECK(fixture.bar->press(elsewhere));
    BOOST_CHECK(!fixture.bar->active());
    BOOST_CHECK(fixture.sent.empty());
}

/**
 * A check item draws its mark only when it is checked, and what checks it is whatever handles
 * its command rather than the item's own activation.
 **/
BOOST_AUTO_TEST_CASE(a_check_item_draws_a_mark_only_when_it_is_checked) {
    Fixture fixture;
    build(&fixture);
    fixture.draw();
    fixture.bar->press(fixture.label(0));

    fixture.draw();
    const std::size_t unchecked = v3d::ui::typeset::shapeVertices(fixture.canvas).size();

    const boost::shared_ptr<v3d::ui::component::menu::Item> grid = fixture.bar->find("test::grid");
    BOOST_REQUIRE(grid);
    BOOST_CHECK(!grid->checked());
    grid->checked(true);

    fixture.draw();
    BOOST_CHECK_EQUAL(v3d::ui::typeset::shapeVertices(fixture.canvas).size(), unchecked + 4);

    // activating it sends the command and leaves the mark where it was
    const boost::shared_ptr<v3d::ui::component::menu::Menu> panel = fixture.bar->panels().front();
    fixture.bar->press(Fixture::centre(*(*panel)[1]));
    BOOST_REQUIRE_EQUAL(fixture.sent.size(), 1);
    BOOST_CHECK_EQUAL(fixture.sent[0], "test::grid");
    BOOST_CHECK(grid->checked());
}

/**
 * Only a check or a radio item has a mark. An action item told it is checked still draws none.
 **/
BOOST_AUTO_TEST_CASE(an_action_item_draws_no_mark) {
    Fixture fixture;
    build(&fixture);
    fixture.draw();
    fixture.bar->press(fixture.label(0));

    fixture.draw();
    const std::size_t unmarked = v3d::ui::typeset::shapeVertices(fixture.canvas).size();

    const boost::shared_ptr<v3d::ui::component::menu::Item> open = fixture.bar->find("test::open");
    BOOST_REQUIRE(open);
    open->checked(true);
    BOOST_CHECK(!open->checked());

    fixture.draw();
    BOOST_CHECK_EQUAL(v3d::ui::typeset::shapeVertices(fixture.canvas).size(), unmarked);
}

/**
 * An item is findable by the command it sends, however deep it is, and an unbound one is
 * skipped. A submenu item has no event.
 **/
BOOST_AUTO_TEST_CASE(an_item_is_found_by_the_command_it_sends) {
    Fixture fixture;
    build(&fixture);

    BOOST_CHECK(fixture.bar->find("test::open"));
    BOOST_CHECK(fixture.bar->find("test::flat"));
    BOOST_CHECK(!fixture.bar->find("test::nothing"));
}


/**
 * A disabled item is not lit and sends nothing, as a disabled button does not.
 **/
BOOST_AUTO_TEST_CASE(a_disabled_item_sends_nothing) {
    Fixture fixture;
    build(&fixture);
    fixture.draw();

    fixture.bar->press(fixture.label(0));
    fixture.draw();

    const boost::shared_ptr<v3d::ui::component::menu::Menu> panel = fixture.bar->panels().front();
    (*panel)[0]->enabled(false);
    fixture.bar->press(Fixture::centre(*(*panel)[0]));
    BOOST_CHECK(fixture.sent.empty());
}

/**
 * A submenu inherits from the item that opens it. Disabling the item disables what the
 * submenu holds, and keyboard navigation does not open it.
 **/
BOOST_AUTO_TEST_CASE(a_disabled_submenu_item_disables_its_submenu) {
    Fixture fixture;
    boost::shared_ptr<v3d::ui::component::menu::Menu> shading = fixture.menu();
    shading->addItem(fixture.item(v3d::ui::component::menu::ItemType::Action, {.label = "Flat", .name = "flat"}));
    const boost::shared_ptr<v3d::ui::component::menu::Item> deeper =
        fixture.item(v3d::ui::component::menu::ItemType::Submenu, {.label = "Shading", .name = ""});
    boost::shared_ptr<v3d::ui::component::menu::Menu> view = fixture.menu();
    deeper->menu(view);
    deeper->submenu(shading);
    view->addItem(deeper);
    view->level(view);
    view->active(0);

    BOOST_TEST(v3d::ui::usable(*(*shading)[0]));
    deeper->enabled(false);
    BOOST_TEST(!v3d::ui::usable(*(*shading)[0]));
    BOOST_TEST(!view->down());
    BOOST_TEST(view->level() == view);

    deeper->enabled(true);
    BOOST_TEST(view->down());
    BOOST_TEST(view->level() == shading);
}

/**
 * A null menu is not added, so the row holds only real menus and opening any of them shows one.
 **/
BOOST_AUTO_TEST_CASE(a_null_menu_is_not_added) {
    Fixture fixture;
    fixture.bar->add("Empty", boost::shared_ptr<v3d::ui::component::menu::Menu>());
    BOOST_CHECK_EQUAL(fixture.bar->count(), 0U);

    build(&fixture);
    BOOST_REQUIRE_EQUAL(fixture.bar->count(), 2U);
    BOOST_CHECK_EQUAL(fixture.bar->label(0), "File");
    fixture.draw();

    BOOST_CHECK(fixture.bar->press(fixture.label(0)));
    BOOST_CHECK_EQUAL(fixture.bar->open(), 0);
    BOOST_REQUIRE_EQUAL(fixture.bar->panels().size(), 1U);
    BOOST_CHECK(fixture.bar->panels().front() == fixture.bar->menu(0));
}

/**
 * An item an app still holds after its menu is gone names no parent, so asking whether it is
 * usable does not walk into the menu that was destroyed.
 **/
BOOST_AUTO_TEST_CASE(an_item_outlives_its_menu) {
    Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::menu::Item> kept =
        fixture.item(v3d::ui::component::menu::ItemType::Action, {.label = "Open", .name = "open"});
    {
        const boost::shared_ptr<v3d::ui::component::menu::Menu> menu = fixture.menu();
        menu->addItem(kept);
        BOOST_TEST(kept->parent() == menu.get());
    }
    BOOST_TEST(kept->parent() == nullptr);
    BOOST_TEST(v3d::ui::usable(*kept));
}

/**
 * A submenu that is replaced no longer names the item as its parent, so it does not walk into
 * the item once the item is gone. A null submenu removes the one there was.
 **/
BOOST_AUTO_TEST_CASE(a_replaced_submenu_is_let_go) {
    Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::menu::Menu> first = fixture.menu();
    const boost::shared_ptr<v3d::ui::component::menu::Menu> second = fixture.menu();
    {
        const boost::shared_ptr<v3d::ui::component::menu::Item> deeper =
            fixture.item(v3d::ui::component::menu::ItemType::Submenu, {.label = "Shading", .name = ""});
        deeper->submenu(first);
        BOOST_TEST(static_cast<const v3d::ui::Component&>(*first).parent() == deeper.get());
        deeper->submenu(second);
        BOOST_TEST(static_cast<const v3d::ui::Component&>(*first).parent() == nullptr);
        BOOST_TEST(static_cast<const v3d::ui::Component&>(*second).parent() == deeper.get());
        deeper->submenu(boost::shared_ptr<v3d::ui::component::menu::Menu>());
        BOOST_TEST(static_cast<const v3d::ui::Component&>(*second).parent() == nullptr);
    }
    BOOST_TEST(v3d::ui::usable(*first));
}

/**
 * The arrows walk an open bar. Down and up move through the deepest panel, stepping over a
 * separator and wrapping. Right enters a submenu at once, and left comes back out of it and
 * across to the other menu.
 * Return sends the chosen item and closes the bar. A closed bar takes no key.
 **/
BOOST_AUTO_TEST_CASE(the_arrows_walk_an_open_bar) {
    Fixture fixture;
    build(&fixture);
    const boost::shared_ptr<v3d::ui::component::menu::Menu> file = fixture.bar->menu(0);
    file->addItem(fixture.item(v3d::ui::component::menu::ItemType::Separator, {.label = "", .name = ""}));
    file->addItem(fixture.item(v3d::ui::component::menu::ItemType::Action, {.label = "Quit", .name = "quit"}));
    fixture.draw();
    BOOST_CHECK(!fixture.bar->key("arrow_down"));

    BOOST_REQUIRE(fixture.bar->press(fixture.label(0)));
    BOOST_CHECK(fixture.bar->key("arrow_down"));
    BOOST_CHECK_EQUAL(file->active()->label(), "Open");
    fixture.bar->key("arrow_down");
    fixture.bar->key("arrow_down");
    BOOST_CHECK_EQUAL(file->active()->label(), "Quit");
    fixture.bar->key("arrow_down");
    BOOST_CHECK_EQUAL(file->active()->label(), "Open");
    fixture.bar->key("arrow_up");
    BOOST_CHECK_EQUAL(file->active()->label(), "Quit");

    // Quit has no submenu, so the right arrow moves to View and chooses its first item
    BOOST_CHECK(fixture.bar->key("arrow_right"));
    BOOST_CHECK_EQUAL(fixture.bar->open(), 1);
    BOOST_REQUIRE_EQUAL(fixture.bar->panels().size(), 1U);
    BOOST_CHECK_EQUAL(fixture.bar->panels()[0]->active()->label(), "Shading");
    fixture.bar->key("arrow_right");
    BOOST_REQUIRE_EQUAL(fixture.bar->panels().size(), 2U);
    BOOST_CHECK_EQUAL(fixture.bar->panels()[1]->active()->label(), "Flat");
    fixture.bar->key("arrow_left");
    BOOST_CHECK_EQUAL(fixture.bar->panels().size(), 1U);
    fixture.bar->key("arrow_left");
    BOOST_CHECK_EQUAL(fixture.bar->open(), 0);
    BOOST_CHECK_EQUAL(file->active()->label(), "Open");

    BOOST_CHECK(fixture.bar->key("return"));
    BOOST_CHECK(!fixture.bar->active());
    BOOST_REQUIRE_EQUAL(fixture.sent.size(), 1U);
    BOOST_CHECK_EQUAL(fixture.sent[0], "test::open");
}

/**
 * Escape closes the deepest submenu, and then the bar. A key that walks nothing is not taken.
 **/
BOOST_AUTO_TEST_CASE(escape_backs_out_of_an_open_bar) {
    Fixture fixture;
    build(&fixture);
    fixture.draw();
    BOOST_REQUIRE(fixture.bar->press(fixture.label(1)));
    fixture.bar->key("arrow_down");
    fixture.bar->key("return");
    BOOST_REQUIRE_EQUAL(fixture.bar->panels().size(), 2U);
    BOOST_CHECK(!fixture.bar->key("a"));
    BOOST_CHECK(fixture.bar->key("escape"));
    BOOST_CHECK_EQUAL(fixture.bar->panels().size(), 1U);
    BOOST_CHECK(fixture.bar->key("escape"));
    BOOST_CHECK(!fixture.bar->active());
    BOOST_CHECK(fixture.sent.empty());
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
