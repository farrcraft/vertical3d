/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/ui/Arranger.h>
#include <api/ui/Component.h>
#include <api/ui/Container.h>
#include <api/ui/style/Style.h>
#include <api/ui/component/CheckBox.h>
#include <api/ui/component/Label.h>
#include <api/ui/component/Panel.h>
#include <api/ui/component/Scrollbar.h>
#include <api/ui/component/VerticalBox.h>
#include <api/ui/style/Resolver.h>
#include <api/ui/style/Theme.h>
#include <api/ui/style/property/Number.h>

#include <string>
#include <string_view>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <boost/make_shared.hpp>

namespace {

const float characterWidth = 10.0f;

v3d::ui::Measure measure() {
    return [](std::string_view text) { return static_cast<float>(text.size()) * characterWidth; };
}

boost::shared_ptr<v3d::ui::component::Label> label(const std::string& name, const std::string& text) {
    boost::shared_ptr<v3d::ui::component::Label> made =
        boost::make_shared<v3d::ui::component::Label>();
    made->name(name);
    made->text(text);
    return made;
}

v3d::type::Bound2D canvasArea(float width, float height) {
    return v3d::type::Bound2D(glm::vec2(0.0f, 0.0f), glm::vec2(width, height));
}

};  // namespace

BOOST_AUTO_TEST_SUITE(arranger_test)

/**
 * A tree can be laid out with no canvas and nothing to paint it, which is the thing
 * ADR-0034 recorded as the cost of putting layout in the draw walk. It is the *walk* that
 * layout lives in, and the walk will run without drawing.
 **/
BOOST_AUTO_TEST_CASE(a_tree_is_laid_out_without_being_drawn) {
    v3d::ui::style::Resolver styles;
    const v3d::ui::Arranger arranger(measure(), styles);

    const boost::shared_ptr<v3d::ui::component::Panel> plate =
        boost::make_shared<v3d::ui::component::Panel>();
    plate->layout().x = v3d::ui::Length(10.0f, v3d::ui::Length::Unit::Pixels);
    plate->layout().y = v3d::ui::Length(20.0f, v3d::ui::Length::Unit::Pixels);
    plate->layout().width = v3d::ui::Length(50.0f, v3d::ui::Length::Unit::Percent);
    plate->layout().height = v3d::ui::Length(80.0f, v3d::ui::Length::Unit::Pixels);

    const boost::shared_ptr<v3d::ui::component::Label> inner = label("inner", "abc");
    inner->layout().x = v3d::ui::Length(4.0f, v3d::ui::Length::Unit::Pixels);
    plate->add(inner);

    // no canvas, no paint
    arranger.walk(nullptr, plate,
        plate->layout().resolve(canvasArea(400.0f, 200.0f), arranger.natural(*plate, canvasArea(400.0f, 200.0f))),
        v3d::ui::Arranger::Paint());

    BOOST_CHECK_CLOSE(plate->position().x, 10.0f, 0.001f);
    BOOST_CHECK_CLOSE(plate->size().x, 200.0f, 0.001f);
    BOOST_CHECK_CLOSE(plate->size().y, 80.0f, 0.001f);
    // and the child was placed inside it, and sized from the text it holds
    BOOST_CHECK_CLOSE(inner->position().x, 14.0f, 0.001f);
    BOOST_CHECK_CLOSE(inner->size().x, 3.0f * characterWidth, 0.001f);
}

/**
 * The paint is called once per component, in the order the walk reaches them: a parent
 * before what it holds, so a child is drawn over its plate.
 **/
BOOST_AUTO_TEST_CASE(the_paint_is_called_once_per_component_parent_first) {
    v3d::ui::style::Resolver styles;
    const v3d::ui::Arranger arranger(measure(), styles);

    const boost::shared_ptr<v3d::ui::component::VerticalBox> box =
        boost::make_shared<v3d::ui::component::VerticalBox>();
    box->name("rows");
    box->layout().width = v3d::ui::Length(120.0f, v3d::ui::Length::Unit::Pixels);
    box->layout().height = v3d::ui::Length(90.0f, v3d::ui::Length::Unit::Pixels);
    box->add(label("first", "a"));
    box->add(label("second", "b"));

    std::vector<std::string> painted;
    arranger.walk(nullptr, box,
        box->layout().resolve(canvasArea(400.0f, 200.0f), arranger.natural(*box, canvasArea(400.0f, 200.0f))),
        [&painted](v3d::render::realtime::Canvas*, const boost::shared_ptr<v3d::ui::Component>& each) {
            painted.push_back(std::string(each->name()));
        });

    BOOST_REQUIRE_EQUAL(painted.size(), 3U);
    BOOST_CHECK_EQUAL(painted[0], "rows");
    BOOST_CHECK_EQUAL(painted[1], "first");
    BOOST_CHECK_EQUAL(painted[2], "second");
}

/**
 * A hidden component is neither placed nor painted, and nothing it holds is either.
 **/
BOOST_AUTO_TEST_CASE(a_hidden_component_is_not_walked) {
    v3d::ui::style::Resolver styles;
    const v3d::ui::Arranger arranger(measure(), styles);

    const boost::shared_ptr<v3d::ui::component::Panel> plate =
        boost::make_shared<v3d::ui::component::Panel>();
    plate->visible(false);
    plate->add(label("inner", "abc"));

    unsigned int painted = 0;
    arranger.walk(nullptr, plate, canvasArea(400.0f, 200.0f),
        [&painted](v3d::render::realtime::Canvas*, const boost::shared_ptr<v3d::ui::Component>&) {
            painted++;
        });

    BOOST_CHECK_EQUAL(painted, 0U);
}

/**
 * A check box asks for room from its own style class, not from the base.
 *
 * The mark is drawn at the size the "checkbox" class names, so sizing the row from the base
 * would leave a themed box laid out for one mark and drawn with another.
 **/
BOOST_AUTO_TEST_CASE(a_check_box_asks_for_room_from_its_own_class) {
    const boost::shared_ptr<v3d::ui::style::Theme> theme =
        boost::make_shared<v3d::ui::style::Theme>("dark");
    const boost::shared_ptr<v3d::ui::style::Style> styled =
        boost::make_shared<v3d::ui::style::Style>("big", "checkbox");
    styled->addProperty(boost::make_shared<v3d::ui::style::property::Number>("mark-size", 40.0f), "number");
    theme->addStyle(styled);

    const boost::shared_ptr<v3d::ui::component::CheckBox> box =
        boost::make_shared<v3d::ui::component::CheckBox>();
    box->style("big");

    // with no theme, the base is all there is
    v3d::ui::style::Resolver bare;
    bare.base().markSize = 16.0f;
    BOOST_CHECK_CLOSE(v3d::ui::Arranger(measure(), bare).natural(*box, canvasArea(400.0f, 200.0f)).x, 16.0f, 0.001f);

    // with one, the row is sized for the mark that will be drawn in it rather than for the
    // base - which is what a themed box laid out one way and drawn another used to do
    v3d::ui::style::Resolver styles;
    styles.base().markSize = 16.0f;
    styles.theme(theme);
    BOOST_CHECK_CLOSE(v3d::ui::Arranger(measure(), styles).natural(*box, canvasArea(400.0f, 200.0f)).x, 40.0f, 0.001f);
}

/**
 * A component that makes nothing of itself takes the room it is in, and takes it on the very
 * first walk - nothing here is answered from a box an earlier walk wrote, per ADR-0039.
 **/
BOOST_AUTO_TEST_CASE(an_auto_extent_is_the_room_on_the_first_walk) {
    v3d::ui::style::Resolver styles;
    const v3d::ui::Arranger arranger(measure(), styles);

    const boost::shared_ptr<v3d::ui::component::Panel> plate =
        boost::make_shared<v3d::ui::component::Panel>();

    arranger.walk(nullptr, plate, canvasArea(400.0f, 200.0f), v3d::ui::Arranger::Paint());

    BOOST_CHECK_CLOSE(plate->size().x, 400.0f, 0.001f);
    BOOST_CHECK_CLOSE(plate->size().y, 200.0f, 0.001f);
}

/**
 * A component that names neither x nor y sits at the corner it is anchored to, which is its
 * parent's rather than the canvas's - so a child of a panel away from the origin is inside
 * that panel. ADR-0039.
 **/
BOOST_AUTO_TEST_CASE(an_auto_position_is_the_corner_of_the_parent) {
    v3d::ui::style::Resolver styles;
    const v3d::ui::Arranger arranger(measure(), styles);

    const boost::shared_ptr<v3d::ui::component::Panel> plate =
        boost::make_shared<v3d::ui::component::Panel>();
    plate->layout().x = v3d::ui::Length(100.0f, v3d::ui::Length::Unit::Pixels);
    plate->layout().y = v3d::ui::Length(50.0f, v3d::ui::Length::Unit::Pixels);
    plate->layout().width = v3d::ui::Length(200.0f, v3d::ui::Length::Unit::Pixels);
    plate->layout().height = v3d::ui::Length(100.0f, v3d::ui::Length::Unit::Pixels);

    const boost::shared_ptr<v3d::ui::component::Label> inner = label("inner", "abc");
    plate->add(inner);

    arranger.walk(nullptr, plate,
        plate->layout().resolve(canvasArea(400.0f, 200.0f), arranger.natural(*plate, canvasArea(400.0f, 200.0f))),
        v3d::ui::Arranger::Paint());

    BOOST_CHECK_CLOSE(inner->position().x, 100.0f, 0.001f);
    BOOST_CHECK_CLOSE(inner->position().y, 50.0f, 0.001f);
}

/**
 * A scrollbar decides how thick it is and nothing about how long, so its Auto length is the
 * box it runs down, and it is that on the first walk.
 **/
BOOST_AUTO_TEST_CASE(a_scrollbar_is_as_long_as_the_box_it_runs_down) {
    v3d::ui::style::Resolver styles;
    styles.base().scrollbarWidth = 12.0f;
    const v3d::ui::Arranger arranger(measure(), styles);

    const boost::shared_ptr<v3d::ui::component::Panel> plate =
        boost::make_shared<v3d::ui::component::Panel>();
    const boost::shared_ptr<v3d::ui::component::Scrollbar> bar =
        boost::make_shared<v3d::ui::component::Scrollbar>();
    bar->layout().anchor = v3d::ui::Layout::Anchor::TopRight;
    plate->add(bar);

    arranger.walk(nullptr, plate, canvasArea(400.0f, 200.0f), v3d::ui::Arranger::Paint());

    BOOST_CHECK_CLOSE(bar->size().x, 12.0f, 0.001f);
    BOOST_CHECK_CLOSE(bar->size().y, 200.0f, 0.001f);
}

/**
 * Along the line a flow box lays out, the children share the room, so an Auto extent there
 * is what the child makes of itself and a panel that makes nothing of itself asks for
 * nothing. Across the line it is offered the whole width.
 **/
BOOST_AUTO_TEST_CASE(an_auto_extent_along_a_flow_is_not_the_whole_line) {
    v3d::ui::style::Resolver styles;
    const v3d::ui::Arranger arranger(measure(), styles);

    const boost::shared_ptr<v3d::ui::component::VerticalBox> column =
        boost::make_shared<v3d::ui::component::VerticalBox>();
    const boost::shared_ptr<v3d::ui::component::Panel> first =
        boost::make_shared<v3d::ui::component::Panel>();
    first->layout().height = v3d::ui::Length(30.0f, v3d::ui::Length::Unit::Pixels);
    const boost::shared_ptr<v3d::ui::component::Panel> second =
        boost::make_shared<v3d::ui::component::Panel>();
    column->add(first);
    column->add(second);

    arranger.walk(nullptr, column, canvasArea(400.0f, 200.0f), v3d::ui::Arranger::Paint());

    BOOST_CHECK_CLOSE(first->size().y, 30.0f, 0.001f);
    // the second asks for nothing along the line rather than for the whole of it
    BOOST_CHECK_SMALL(second->size().y, 0.001f);
    BOOST_CHECK_CLOSE(second->position().y, 30.0f, 0.001f);
    // and across it, each is offered the whole width
    BOOST_CHECK_CLOSE(second->size().x, 400.0f, 0.001f);
}

/**
 * The walk never reads the box a previous walk wrote, so the same tree laid out twice lands
 * in the same place - and a tree laid out against a new canvas lands against that one
 * rather than against the size before it. ADR-0039.
 **/
BOOST_AUTO_TEST_CASE(a_second_walk_lands_where_the_first_did) {
    v3d::ui::style::Resolver styles;
    const v3d::ui::Arranger arranger(measure(), styles);

    const boost::shared_ptr<v3d::ui::component::Panel> plate =
        boost::make_shared<v3d::ui::component::Panel>();
    plate->layout().width = v3d::ui::Length(50.0f, v3d::ui::Length::Unit::Percent);
    const boost::shared_ptr<v3d::ui::component::Panel> inner =
        boost::make_shared<v3d::ui::component::Panel>();
    plate->add(inner);

    arranger.walk(nullptr, plate, canvasArea(400.0f, 200.0f), v3d::ui::Arranger::Paint());
    const glm::vec2 once = inner->size();
    arranger.walk(nullptr, plate, canvasArea(400.0f, 200.0f), v3d::ui::Arranger::Paint());

    BOOST_CHECK_CLOSE(inner->size().x, once.x, 0.001f);
    BOOST_CHECK_CLOSE(inner->size().y, once.y, 0.001f);

    // and the frame after a resize is against the new canvas, not the one before it
    arranger.walk(nullptr, plate, canvasArea(800.0f, 200.0f), v3d::ui::Arranger::Paint());
    BOOST_CHECK_CLOSE(inner->size().x, 800.0f, 0.001f);
}

/**
 * A label given a width wraps to it, and its Auto height becomes the rows it came to. A
 * measure of one unit per character makes where the breaks fall arithmetic.
 **/
BOOST_AUTO_TEST_CASE(a_label_with_a_width_wraps_to_it) {
    v3d::ui::style::Resolver styles;
    const v3d::ui::Arranger arranger(measure(), styles);
    const float line = styles.base().lineHeight;

    // "one two three four" is 18 characters; at 10 a character, 100 pixels holds "one two"
    // and then "three four"
    const boost::shared_ptr<v3d::ui::component::Label> wrapped = label("wrapped", "one two three four");
    wrapped->layout().width = v3d::ui::Length(100.0f, v3d::ui::Length::Unit::Pixels);

    const v3d::type::Bound2D room = canvasArea(400.0f, 200.0f);
    arranger.walk(nullptr, wrapped, wrapped->layout().resolve(room, arranger.natural(*wrapped, room)),
        v3d::ui::Arranger::Paint());
    BOOST_CHECK_CLOSE(wrapped->size().x, 100.0f, 0.001f);
    BOOST_CHECK_CLOSE(wrapped->size().y, 2.0f * line, 0.001f);

    // an Auto width is one line, exactly as before
    const boost::shared_ptr<v3d::ui::component::Label> single = label("single", "one two three four");
    arranger.walk(nullptr, single, single->layout().resolve(room, arranger.natural(*single, room)),
        v3d::ui::Arranger::Paint());
    BOOST_CHECK_CLOSE(single->size().x, 18.0f * characterWidth, 0.001f);
    BOOST_CHECK_CLOSE(single->size().y, line, 0.001f);

    // a percentage is a width like any other, and narrowing it takes more rows
    const boost::shared_ptr<v3d::ui::component::Label> shared = label("shared", "one two three four");
    shared->layout().width = v3d::ui::Length(10.0f, v3d::ui::Length::Unit::Percent);
    arranger.walk(nullptr, shared, shared->layout().resolve(room, arranger.natural(*shared, room)),
        v3d::ui::Arranger::Paint());
    BOOST_CHECK_CLOSE(shared->size().x, 40.0f, 0.001f);
    BOOST_CHECK_CLOSE(shared->size().y, 4.0f * line, 0.001f);

    // a named height is still the height it named - wrapping decides what Auto is offered
    const boost::shared_ptr<v3d::ui::component::Label> fixed = label("fixed", "one two three four");
    fixed->layout().width = v3d::ui::Length(100.0f, v3d::ui::Length::Unit::Pixels);
    fixed->layout().height = v3d::ui::Length(9.0f, v3d::ui::Length::Unit::Pixels);
    arranger.walk(nullptr, fixed, fixed->layout().resolve(room, arranger.natural(*fixed, room)),
        v3d::ui::Arranger::Paint());
    BOOST_CHECK_CLOSE(fixed->size().y, 9.0f, 0.001f);
}

BOOST_AUTO_TEST_SUITE_END()
