/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/Canvas.h>
#include <api/ui/Container.h>
#include <api/ui/component/menu/Menu.h>
#include <api/ui/component/menu/MenuBar.h>
#include <api/ui/component/menu/MenuItem.h>
#include <api/ui/paint/ComponentRenderer.h>

#include <cstddef>
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
const float characterWidth = 10.0f;

/**
 * Where the labels went, so a test can check the layout without a font or a device.
 **/
struct Written final {
    std::string text;
    glm::vec2 pen;
};

/**
 * The dispatcher, the events sent to it, and the bar they came from, all of which have to
 * outlive each other in that order.
 **/
struct Fixture final {
    Fixture() :
        dispatcher(boost::make_shared<entt::dispatcher>()),
        context(boost::make_shared<v3d::event::Context>("test")),
        bar(boost::make_shared<v3d::ui::component::MenuBar>()),
        renderer(
            [](std::string_view text) { return static_cast<float>(text.size()) * characterWidth; },
            [this](std::string_view text, const glm::vec2& pen, const glm::vec4&) {
                Written line;
                line.text = text;
                line.pen = pen;
                written.push_back(line);
            }) {
        dispatcher->sink<v3d::event::Event>().connect<&Fixture::receive>(*this);
        canvas.resize(800, 600);
    }

    void receive(const v3d::event::Event& event) {
        sent.push_back(event.str());
    }

    /**
     * An item bound to "test::<name>", or to nothing when the name is empty.
     **/
    boost::shared_ptr<v3d::ui::component::MenuItem> item(v3d::ui::component::menu::ItemType type,
        const std::string& label, const std::string& name) {
        boost::shared_ptr<v3d::ui::component::MenuItem> made =
            boost::make_shared<v3d::ui::component::MenuItem>(type, label);
        if (!name.empty()) {
            made->event(v3d::event::Event(name, context));
        }
        return made;
    }

    boost::shared_ptr<v3d::ui::component::Menu> menu() {
        return boost::make_shared<v3d::ui::component::Menu>(dispatcher);
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
     * The middle of a component, which is where a click on it lands.
     **/
    static glm::vec2 centre(const v3d::ui::Component& component) {
        return component.position() + component.size() * 0.5f;
    }

    /**
     * The middle of a menu's label in the strip, which the bar holds rather than the menu.
     **/
    glm::vec2 label(std::size_t index) const {
        const v3d::type::Bound2D bounds = bar->bound(index);
        return bounds.position() + bounds.size() * 0.5f;
    }

    boost::shared_ptr<entt::dispatcher> dispatcher;
    boost::shared_ptr<v3d::event::Context> context;
    boost::shared_ptr<v3d::ui::component::MenuBar> bar;
    v3d::render::realtime::Canvas canvas;
    std::vector<Written> written;
    std::vector<std::string> sent;
    v3d::ui::paint::ComponentRenderer renderer;
};

/**
 * A bar of two menus: "File" holding an action and a checked item, and "View" holding a
 * submenu that holds one action.
 **/
void build(Fixture* fixture) {
    boost::shared_ptr<v3d::ui::component::Menu> file = fixture->menu();
    file->addItem(fixture->item(v3d::ui::component::menu::ItemType::Action, "Open", "open"));
    file->addItem(fixture->item(v3d::ui::component::menu::ItemType::Check, "Grid", "grid"));

    boost::shared_ptr<v3d::ui::component::Menu> shading = fixture->menu();
    shading->addItem(fixture->item(v3d::ui::component::menu::ItemType::Action, "Flat", "flat"));

    boost::shared_ptr<v3d::ui::component::MenuItem> deeper =
        fixture->item(v3d::ui::component::menu::ItemType::Submenu, "Shading", "");
    boost::shared_ptr<v3d::ui::component::Menu> view = fixture->menu();
    deeper->menu(view);
    deeper->submenu(shading);
    view->addItem(deeper);

    fixture->bar->add("File", file);
    fixture->bar->add("View", view);
}

};  // namespace

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
    BOOST_CHECK_EQUAL(fixture.canvas.vertices().size(), 2 * 4);
}

/**
 * Nothing is hit until something has been drawn: the bounds a click is tested against are
 * what the last draw left on the components, per ADR-0019.
 **/
BOOST_AUTO_TEST_CASE(a_bar_that_has_not_been_drawn_takes_no_press) {
    Fixture fixture;
    build(&fixture);

    BOOST_CHECK(!fixture.bar->press(glm::vec2(20.0f, 10.0f)));
    BOOST_CHECK(!fixture.bar->active());
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

    boost::shared_ptr<v3d::ui::component::Menu> panel = fixture.bar->panels().front();
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

    boost::shared_ptr<v3d::ui::component::Menu> panel = fixture.bar->panels().front();
    boost::shared_ptr<v3d::ui::component::MenuItem> shading = (*panel)[0];
    BOOST_CHECK(fixture.bar->motion(Fixture::centre(*shading)));
    BOOST_REQUIRE_EQUAL(fixture.bar->panels().size(), 2);

    fixture.draw();
    const boost::shared_ptr<v3d::ui::component::Menu>& flyout = fixture.bar->panels().back();
    // through the bound, because a menu's own size() is its item count
    const v3d::type::Bound2D bounds = panel->bound();
    BOOST_CHECK_CLOSE(flyout->position().x, bounds.position().x + bounds.size().x, 0.01f);
    BOOST_CHECK_CLOSE(flyout->position().y, shading->position().y, 0.01f);

    // the item in the flyout is drawn, and a press on it sends its command
    BOOST_CHECK(fixture.bar->press(Fixture::centre(*(*flyout)[0])));
    BOOST_REQUIRE_EQUAL(fixture.sent.size(), 1);
    BOOST_CHECK_EQUAL(fixture.sent[0], "test::flat");
}

/**
 * Moving off a submenu item onto one of its siblings closes the flyout, which is what stops
 * two of them standing open over each other.
 **/
BOOST_AUTO_TEST_CASE(leaving_a_submenu_item_closes_its_flyout) {
    Fixture fixture;
    build(&fixture);
    fixture.draw();

    fixture.bar->press(fixture.label(0));
    fixture.draw();
    boost::shared_ptr<v3d::ui::component::Menu> file = fixture.bar->panels().front();

    // File holds no submenu, so crossing either of its items leaves one panel standing
    fixture.bar->motion(Fixture::centre(*(*file)[0]));
    BOOST_CHECK_EQUAL(fixture.bar->panels().size(), 1);

    fixture.bar->press(fixture.label(1));
    fixture.draw();
    boost::shared_ptr<v3d::ui::component::Menu> view = fixture.bar->panels().front();
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

    const glm::vec2 elsewhere(400.0f, 400.0f);
    BOOST_CHECK(!fixture.bar->press(elsewhere));

    fixture.bar->press(fixture.label(0));
    fixture.draw();
    BOOST_CHECK(fixture.bar->press(elsewhere));
    BOOST_CHECK(!fixture.bar->active());
    BOOST_CHECK(fixture.sent.empty());
}

/**
 * A check item draws its mark only when it is checked, and what checks it is whatever answers
 * its command rather than the item's own activation.
 **/
BOOST_AUTO_TEST_CASE(a_check_item_draws_a_mark_only_when_it_is_checked) {
    Fixture fixture;
    build(&fixture);
    fixture.draw();
    fixture.bar->press(fixture.label(0));

    fixture.draw();
    const std::size_t unchecked = fixture.canvas.vertices().size();

    boost::shared_ptr<v3d::ui::component::MenuItem> grid = fixture.bar->find("test::grid");
    BOOST_REQUIRE(grid);
    BOOST_CHECK(!grid->checked());
    grid->checked(true);

    fixture.draw();
    BOOST_CHECK_EQUAL(fixture.canvas.vertices().size(), unchecked + 4);

    // activating it sends the command and leaves the mark where it was
    boost::shared_ptr<v3d::ui::component::Menu> panel = fixture.bar->panels().front();
    fixture.bar->press(Fixture::centre(*(*panel)[1]));
    BOOST_REQUIRE_EQUAL(fixture.sent.size(), 1);
    BOOST_CHECK_EQUAL(fixture.sent[0], "test::grid");
    BOOST_CHECK(grid->checked());
}

/**
 * An item is findable by the command it sends, however deep it is, and an unbound one is
 * skipped rather than dereferenced - a submenu item has no event.
 **/
BOOST_AUTO_TEST_CASE(an_item_is_found_by_the_command_it_sends) {
    Fixture fixture;
    build(&fixture);

    BOOST_CHECK(fixture.bar->find("test::open"));
    BOOST_CHECK(fixture.bar->find("test::flat"));
    BOOST_CHECK(!fixture.bar->find("test::nothing"));
}

BOOST_AUTO_TEST_SUITE_END()
