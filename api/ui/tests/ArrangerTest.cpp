/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <string>
#include <string_view>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "../Arranger.h"

#include "../Component.h"
#include "../Container.h"
#include "../Style.h"
#include "../component/CheckBox.h"
#include "../component/Label.h"
#include "../component/Panel.h"
#include "../component/VerticalBox.h"
#include "../style/Resolver.h"
#include "../style/Theme.h"
#include "../style/property/Number.h"

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
        plate->layout().resolve(canvasArea(400.0f, 200.0f), arranger.natural(*plate), plate->position()),
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
        box->layout().resolve(canvasArea(400.0f, 200.0f), arranger.natural(*box), box->position()),
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
    const boost::shared_ptr<v3d::ui::Style> styled =
        boost::make_shared<v3d::ui::Style>("big", "checkbox");
    styled->addProperty(boost::make_shared<v3d::ui::style::property::Number>("mark-size", 40.0f), "number");
    theme->addStyle(styled);

    const boost::shared_ptr<v3d::ui::component::CheckBox> box =
        boost::make_shared<v3d::ui::component::CheckBox>();
    box->style("big");

    // with no theme, the base is all there is
    v3d::ui::style::Resolver bare;
    bare.base().markSize = 16.0f;
    BOOST_CHECK_CLOSE(v3d::ui::Arranger(measure(), bare).natural(*box).x, 16.0f, 0.001f);

    // with one, the row is sized for the mark that will be drawn in it rather than for the
    // base - which is what a themed box laid out one way and drawn another used to do
    v3d::ui::style::Resolver styles;
    styles.base().markSize = 16.0f;
    styles.theme(theme);
    BOOST_CHECK_CLOSE(v3d::ui::Arranger(measure(), styles).natural(*box).x, 40.0f, 0.001f);
}

BOOST_AUTO_TEST_SUITE_END()
