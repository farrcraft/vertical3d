/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/Canvas.h>
#include <api/ui/ComponentRenderer.h>
#include <api/ui/Container.h>
#include <api/ui/component/Toolbar.h>
#include <api/ui/component/menu/MenuBar.h>

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
 * The dispatcher, the events sent to it, and the strips they came from, all of which have
 * to outlive each other in that order.
 **/
struct Fixture final {
    Fixture() :
        dispatcher(boost::make_shared<entt::dispatcher>()),
        context(boost::make_shared<v3d::event::Context>("test")),
        renderer(
            [](std::string_view text) { return static_cast<float>(text.size()) * characterWidth; },
            [](std::string_view, const glm::vec2&, const glm::vec4&) {}) {
        dispatcher->sink<v3d::event::Event>().connect<&Fixture::receive>(*this);
        canvas.resize(800, 600);
    }

    void receive(const v3d::event::Event& event) {
        sent.push_back(event.str());
    }

    /**
     * A button labelled and bound to "test::<name>", or to nothing when the name is empty.
     **/
    boost::shared_ptr<v3d::ui::component::Button> button(const std::string& label, const std::string& name,
        bool toggle) {
        boost::shared_ptr<v3d::ui::component::Button> made =
            boost::make_shared<v3d::ui::component::Button>();
        made->label(label);
        made->toggle(toggle);
        if (!name.empty()) {
            made->event(v3d::event::Event(name, context));
        }
        return made;
    }

    /**
     * A strip of three buttons: two toggles and one plain, the last of them unbound.
     **/
    boost::shared_ptr<v3d::ui::component::Toolbar> bar(v3d::ui::component::Toolbar::Edge edge) {
        boost::shared_ptr<v3d::ui::component::Toolbar> made =
            boost::make_shared<v3d::ui::component::Toolbar>(dispatcher, edge);
        made->add(button("Object", "object", true));
        made->add(button("Translate", "translate", true));
        made->add(button("Go", "", false));
        return made;
    }

    /**
     * The middle of a component, which is where a click on it lands.
     **/
    static glm::vec2 centre(const v3d::ui::Component& component) {
        return component.position() + component.size() * 0.5f;
    }

    boost::shared_ptr<entt::dispatcher> dispatcher;
    boost::shared_ptr<v3d::event::Context> context;
    v3d::render::realtime::Canvas canvas;
    std::vector<std::string> sent;
    v3d::ui::ComponentRenderer renderer;
};

};  // namespace

/**
 * A top toolbar is a row: it spans the canvas, and its buttons run left to right, each as
 * wide as its label plus the padding.
 **/
BOOST_AUTO_TEST_CASE(toolbar_row_layout) {
    Fixture fixture;
    boost::shared_ptr<v3d::ui::component::Toolbar> bar =
        fixture.bar(v3d::ui::component::Toolbar::Edge::Top);
    const v3d::ui::Dressing& style = fixture.renderer.dressing();

    fixture.renderer.draw(&fixture.canvas, bar, glm::vec2(0.0f, 30.0f));

    const v3d::ui::Component& strip = *bar;
    BOOST_TEST(strip.position().y == 30.0f);
    BOOST_TEST(strip.size().x == static_cast<float>(fixture.canvas.width()));
    BOOST_TEST(strip.size().y == style.barHeight);

    float pen = 0.0f;
    for (std::size_t index = 0; index < bar->size(); index++) {
        const boost::shared_ptr<v3d::ui::component::Button> button = bar->button(index);
        BOOST_TEST(button->position().x == pen);
        BOOST_TEST(button->position().y == 30.0f);
        BOOST_TEST(button->size().y == style.barHeight);
        pen += static_cast<float>(button->label().size()) * characterWidth + style.padding;
    }
}

/**
 * A left toolbar is a column: it is as wide as its widest label plus the padding, whatever
 * that label belongs to, and its buttons stack a row height apart.
 **/
BOOST_AUTO_TEST_CASE(toolbar_column_layout) {
    Fixture fixture;
    boost::shared_ptr<v3d::ui::component::Toolbar> bar =
        fixture.bar(v3d::ui::component::Toolbar::Edge::Left);
    const v3d::ui::Dressing& style = fixture.renderer.dressing();

    fixture.renderer.draw(&fixture.canvas, bar, glm::vec2(0.0f, 30.0f));

    const v3d::ui::Component& strip = *bar;
    // "Translate" is the widest of the three
    const float width = 9.0f * characterWidth + style.padding;
    BOOST_TEST(strip.size().x == width);
    BOOST_TEST(strip.size().y == static_cast<float>(fixture.canvas.height()) - 30.0f);

    for (std::size_t index = 0; index < bar->size(); index++) {
        const boost::shared_ptr<v3d::ui::component::Button> button = bar->button(index);
        BOOST_TEST(button->size().x == width);
        BOOST_TEST(button->size().y == style.lineHeight);
        BOOST_TEST(button->position().y == 30.0f + style.lineHeight * static_cast<float>(index));
    }
}

/**
 * A press on a button sends that button's command. One on the strip that misses every button
 * is still taken, so a click on the gap does not reach the scene under it, and one off the
 * strip is not taken at all.
 **/
BOOST_AUTO_TEST_CASE(toolbar_press_sends_its_command) {
    Fixture fixture;
    boost::shared_ptr<v3d::ui::component::Toolbar> bar =
        fixture.bar(v3d::ui::component::Toolbar::Edge::Top);
    fixture.renderer.draw(&fixture.canvas, bar, glm::vec2(0.0f, 0.0f));

    BOOST_TEST(bar->press(Fixture::centre(*bar->button(1))));
    BOOST_REQUIRE_EQUAL(fixture.sent.size(), 1U);
    BOOST_TEST(fixture.sent.front() == "test::translate");

    // past the last button, which is the strip's own empty run
    const v3d::ui::Component& last = *bar->button(2);
    BOOST_TEST(bar->press(glm::vec2(last.position().x + last.size().x + 4.0f, last.position().y + 2.0f)));
    BOOST_TEST(fixture.sent.size() == 1U);

    BOOST_TEST(!bar->press(glm::vec2(10.0f, 400.0f)));
    BOOST_TEST(fixture.sent.size() == 1U);
}

/**
 * An unbound button takes its press and sends nothing, rather than sending an event whose
 * context is null.
 **/
BOOST_AUTO_TEST_CASE(toolbar_unbound_button_sends_nothing) {
    Fixture fixture;
    boost::shared_ptr<v3d::ui::component::Toolbar> bar =
        fixture.bar(v3d::ui::component::Toolbar::Edge::Top);
    fixture.renderer.draw(&fixture.canvas, bar, glm::vec2(0.0f, 0.0f));

    BOOST_TEST(bar->press(Fixture::centre(*bar->button(2))));
    BOOST_TEST(fixture.sent.empty());
}

/**
 * The cursor leaves the button it is on in its hover state and every other one back to normal.
 **/
BOOST_AUTO_TEST_CASE(toolbar_hover_follows_the_cursor) {
    Fixture fixture;
    boost::shared_ptr<v3d::ui::component::Toolbar> bar =
        fixture.bar(v3d::ui::component::Toolbar::Edge::Top);
    fixture.renderer.draw(&fixture.canvas, bar, glm::vec2(0.0f, 0.0f));

    BOOST_TEST(bar->motion(Fixture::centre(*bar->button(0))));
    BOOST_TEST((bar->button(0)->state() == v3d::ui::component::Button::STATE_HOVER));
    BOOST_TEST((bar->button(1)->state() == v3d::ui::component::Button::STATE_NORMAL));

    BOOST_TEST(bar->motion(Fixture::centre(*bar->button(1))));
    BOOST_TEST((bar->button(0)->state() == v3d::ui::component::Button::STATE_NORMAL));
    BOOST_TEST((bar->button(1)->state() == v3d::ui::component::Button::STATE_HOVER));

    // off the strip entirely, which leaves nothing hovered
    BOOST_TEST(!bar->motion(glm::vec2(10.0f, 400.0f)));
    BOOST_TEST((bar->button(1)->state() == v3d::ui::component::Button::STATE_NORMAL));
}

/**
 * A button is found by the command it sends, which is how whatever answers a command marks
 * the button that names it. A button that is not a toggle never shows a mark.
 **/
BOOST_AUTO_TEST_CASE(toolbar_marks_by_command) {
    Fixture fixture;
    boost::shared_ptr<v3d::ui::component::Toolbar> bar =
        fixture.bar(v3d::ui::component::Toolbar::Edge::Top);

    boost::shared_ptr<v3d::ui::component::Button> found = bar->find("test::object");
    BOOST_REQUIRE(found);
    BOOST_TEST(found == bar->button(0));
    BOOST_TEST(!found->checked());
    found->checked(true);
    BOOST_TEST(found->checked());

    BOOST_TEST(!bar->find("test::missing"));

    boost::shared_ptr<v3d::ui::component::Button> plain = bar->button(2);
    plain->checked(true);
    BOOST_TEST(!plain->checked());
}

/**
 * The strips of a container stack, and what they take off the two edges is what insets()
 * reports before anything has been drawn.
 **/
BOOST_AUTO_TEST_CASE(toolbar_insets_match_what_is_drawn) {
    Fixture fixture;
    const v3d::ui::Dressing& style = fixture.renderer.dressing();

    boost::shared_ptr<v3d::ui::component::MenuBar> menu =
        boost::make_shared<v3d::ui::component::MenuBar>();
    boost::shared_ptr<v3d::ui::component::Toolbar> top =
        fixture.bar(v3d::ui::component::Toolbar::Edge::Top);
    boost::shared_ptr<v3d::ui::component::Toolbar> left =
        fixture.bar(v3d::ui::component::Toolbar::Edge::Left);

    v3d::ui::Container container("editor", true);
    container.add(menu);
    container.add(top);
    container.add(left);

    const glm::vec2 insets = fixture.renderer.insets(container);
    const float band = style.barHeight + 1.0f;
    BOOST_TEST(insets.y == band * 2.0f);
    BOOST_TEST(insets.x == 9.0f * characterWidth + style.padding + 1.0f);

    fixture.renderer.draw(&fixture.canvas, container);

    // the top toolbar is under the menu bar, and the column starts below both of them
    const v3d::ui::Component& row = *top;
    BOOST_TEST(row.position().y == band);
    const v3d::ui::Component& column = *left;
    BOOST_TEST(column.position().x == 0.0f);
    BOOST_TEST(column.position().y == band * 2.0f);
    BOOST_TEST(column.size().x + 1.0f == insets.x);
}

/**
 * Two left strips stand side by side rather than on top of each other, on the first frame as
 * well as the ones after it.
 *
 * The draw used to advance past a column by the box the strip was last drawn in, which is
 * nothing until it has been drawn once - so on the first frame both strips were placed at the
 * left edge, and insets() disagreed because it advanced by what the strip would be drawn at.
 * One implementation of the rule is what makes the two agree.
 **/
BOOST_AUTO_TEST_CASE(toolbar_two_columns_stand_side_by_side_on_the_first_frame) {
    Fixture fixture;
    const boost::shared_ptr<v3d::ui::component::Toolbar> first =
        fixture.bar(v3d::ui::component::Toolbar::Edge::Left);
    const boost::shared_ptr<v3d::ui::component::Toolbar> second =
        fixture.bar(v3d::ui::component::Toolbar::Edge::Left);

    v3d::ui::Container container("editor", true);
    container.add(first);
    container.add(second);

    // what an app is told is left to it, before anything has been drawn
    const float reserved = fixture.renderer.insets(container).x;

    fixture.renderer.draw(&fixture.canvas, container);

    // through the base, because a strip's own size() is its button count
    const v3d::ui::Component& left = *first;
    const v3d::ui::Component& right = *second;
    BOOST_TEST(left.position().x == 0.0f);
    BOOST_TEST(right.position().x == left.size().x + 1.0f);
    // and the two together take exactly what the app was told they would
    BOOST_TEST(right.position().x + right.size().x + 1.0f == reserved);
}
