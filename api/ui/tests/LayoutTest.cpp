/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/Canvas.h>
#include <api/ui/ComponentRenderer.h>
#include <api/ui/Container.h>
#include <api/ui/component/Bar.h>
#include <api/ui/component/HorizontalBox.h>
#include <api/ui/component/Label.h>
#include <api/ui/component/Panel.h>
#include <api/ui/component/VerticalBox.h>

#include <string>
#include <string_view>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <boost/make_shared.hpp>

namespace {

/**
 * A fixed width per character, so a label's natural width is predictable.
 **/
const float characterWidth = 10.0f;

/**
 * A renderer that measures a string at ten pixels a character and writes nothing, which is
 * enough to lay a tree out without a font, an atlas or a device.
 **/
v3d::ui::ComponentRenderer build() {
    return v3d::ui::ComponentRenderer(
        [](std::string_view text) { return static_cast<float>(text.size()) * characterWidth; },
        [](std::string_view, const glm::vec2&, const glm::vec4&) {});
}

boost::shared_ptr<v3d::ui::component::Panel> panel(const std::string& name) {
    boost::shared_ptr<v3d::ui::component::Panel> component = boost::make_shared<v3d::ui::component::Panel>();
    component->name(name);
    return component;
}

boost::shared_ptr<v3d::ui::component::Label> label(const std::string& name, const std::string& text) {
    boost::shared_ptr<v3d::ui::component::Label> component = boost::make_shared<v3d::ui::component::Label>();
    component->name(name);
    component->text(text);
    return component;
}

};  // namespace

BOOST_AUTO_TEST_SUITE(layout_test)

/**
 * A pixel length is itself, a percentage is of the extent it is given, and an Auto length is
 * whatever the component makes of the axis.
 **/
BOOST_AUTO_TEST_CASE(a_length_resolves_against_an_extent) {
    BOOST_CHECK_CLOSE(v3d::ui::Length(12.0f, v3d::ui::Length::Unit::Pixels).resolve(400.0f, 7.0f), 12.0f, 0.001f);
    BOOST_CHECK_CLOSE(v3d::ui::Length(25.0f, v3d::ui::Length::Unit::Percent).resolve(400.0f, 7.0f), 100.0f, 0.001f);
    BOOST_CHECK_CLOSE(v3d::ui::Length().resolve(400.0f, 7.0f), 7.0f, 0.001f);
}

/**
 * The anchor says which corner of the parent a position is measured from, and the offset
 * always grows inwards from it.
 **/
BOOST_AUTO_TEST_CASE(an_anchor_measures_from_the_corner_it_names) {
    const v3d::type::Bound2D parent(glm::vec2(0.0f, 0.0f), glm::vec2(200.0f, 100.0f));
    v3d::ui::Layout layout;
    layout.x = v3d::ui::Length(10.0f, v3d::ui::Length::Unit::Pixels);
    layout.y = v3d::ui::Length(20.0f, v3d::ui::Length::Unit::Pixels);
    layout.width = v3d::ui::Length(40.0f, v3d::ui::Length::Unit::Pixels);
    layout.height = v3d::ui::Length(30.0f, v3d::ui::Length::Unit::Pixels);

    v3d::type::Bound2D box = layout.resolve(parent, glm::vec2(0.0f, 0.0f));
    BOOST_CHECK_CLOSE(box.position().x, 10.0f, 0.001f);
    BOOST_CHECK_CLOSE(box.position().y, 20.0f, 0.001f);

    layout.anchor = v3d::ui::Layout::Anchor::BottomRight;
    box = layout.resolve(parent, glm::vec2(0.0f, 0.0f));
    BOOST_CHECK_CLOSE(box.position().x, 200.0f - 10.0f - 40.0f, 0.001f);
    BOOST_CHECK_CLOSE(box.position().y, 100.0f - 20.0f - 30.0f, 0.001f);

    layout.anchor = v3d::ui::Layout::Anchor::Centre;
    layout.x = v3d::ui::Length(0.0f, v3d::ui::Length::Unit::Pixels);
    layout.y = v3d::ui::Length(0.0f, v3d::ui::Length::Unit::Pixels);
    box = layout.resolve(parent, glm::vec2(0.0f, 0.0f));
    BOOST_CHECK_CLOSE(box.position().x, 80.0f, 0.001f);
    BOOST_CHECK_CLOSE(box.position().y, 35.0f, 0.001f);
}

/**
 * A percentage is of the parent's box, not of the canvas, so a child of a child compounds.
 **/
BOOST_AUTO_TEST_CASE(a_percentage_is_of_the_box_around_it) {
    v3d::render::realtime::Canvas canvas;
    canvas.resize(400, 200);

    const boost::shared_ptr<v3d::ui::component::Panel> outer = panel("outer");
    outer->layout().width = v3d::ui::Length(50.0f, v3d::ui::Length::Unit::Percent);
    outer->layout().height = v3d::ui::Length(50.0f, v3d::ui::Length::Unit::Percent);

    const boost::shared_ptr<v3d::ui::component::Panel> inner = panel("inner");
    inner->layout().width = v3d::ui::Length(50.0f, v3d::ui::Length::Unit::Percent);
    inner->layout().height = v3d::ui::Length(100.0f, v3d::ui::Length::Unit::Percent);
    inner->layout().x = v3d::ui::Length(10.0f, v3d::ui::Length::Unit::Pixels);
    outer->add(inner);

    v3d::ui::Container container("hud", true);
    container.add(outer);
    build().draw(&canvas, container);

    BOOST_CHECK_CLOSE(outer->size().x, 200.0f, 0.001f);
    BOOST_CHECK_CLOSE(outer->size().y, 100.0f, 0.001f);
    BOOST_CHECK_CLOSE(inner->size().x, 100.0f, 0.001f);
    BOOST_CHECK_CLOSE(inner->size().y, 100.0f, 0.001f);
    // and it is placed inside its parent rather than on the canvas
    BOOST_CHECK_CLOSE(inner->position().x, 10.0f, 0.001f);
    BOOST_CHECK_EQUAL(inner->parent(), outer.get());
}

/**
 * A vertical box stacks however many children it holds, leaves the gap it was given between
 * them, and widens them to itself when it stretches.
 **/
BOOST_AUTO_TEST_CASE(a_vertical_box_stacks_what_it_holds) {
    v3d::render::realtime::Canvas canvas;
    canvas.resize(400, 200);

    const boost::shared_ptr<v3d::ui::component::VerticalBox> box =
        boost::make_shared<v3d::ui::component::VerticalBox>();
    box->name("objectives");
    box->layout().width = v3d::ui::Length(120.0f, v3d::ui::Length::Unit::Pixels);
    box->layout().height = v3d::ui::Length(90.0f, v3d::ui::Length::Unit::Pixels);
    box->layout().x = v3d::ui::Length(20.0f, v3d::ui::Length::Unit::Pixels);
    box->layout().y = v3d::ui::Length(30.0f, v3d::ui::Length::Unit::Pixels);
    box->spacing(4.0f);
    box->stretch(true);

    for (int index = 0; index < 3; index++) {
        box->add(label("row" + std::to_string(index), "ab"));
    }

    v3d::ui::Container container("hud", true);
    container.add(box);
    v3d::ui::ComponentRenderer renderer = build();
    renderer.draw(&canvas, container);

    const float rowHeight = renderer.dressing().lineHeight;
    for (int index = 0; index < 3; index++) {
        const boost::shared_ptr<v3d::ui::Component> row = container.get("row" + std::to_string(index));
        BOOST_REQUIRE(row);
        BOOST_CHECK_CLOSE(row->position().x, 20.0f, 0.001f);
        BOOST_CHECK_CLOSE(row->position().y, 30.0f + static_cast<float>(index) * (rowHeight + 4.0f), 0.001f);
        // stretched across the box, rather than as wide as the two characters it holds
        BOOST_CHECK_CLOSE(row->size().x, 120.0f, 0.001f);
    }
}

/**
 * A hidden child of a box leaves no gap where it would have been, so a list is as long as
 * what is in it.
 **/
BOOST_AUTO_TEST_CASE(a_hidden_row_closes_the_gap_behind_it) {
    v3d::render::realtime::Canvas canvas;
    canvas.resize(400, 200);

    const boost::shared_ptr<v3d::ui::component::VerticalBox> box =
        boost::make_shared<v3d::ui::component::VerticalBox>();
    box->layout().width = v3d::ui::Length(120.0f, v3d::ui::Length::Unit::Pixels);
    box->layout().height = v3d::ui::Length(90.0f, v3d::ui::Length::Unit::Pixels);
    const boost::shared_ptr<v3d::ui::component::Label> first = label("first", "a");
    const boost::shared_ptr<v3d::ui::component::Label> hidden = label("hidden", "b");
    const boost::shared_ptr<v3d::ui::component::Label> last = label("last", "c");
    hidden->visible(false);
    box->add(first);
    box->add(hidden);
    box->add(last);

    v3d::ui::Container container("hud", true);
    container.add(box);
    v3d::ui::ComponentRenderer renderer = build();
    renderer.draw(&canvas, container);

    BOOST_CHECK_CLOSE(last->position().y, first->position().y + renderer.dressing().lineHeight, 0.001f);
}

/**
 * A horizontal box runs the other way, and a child sizes itself along the line when it asks
 * for nothing.
 **/
BOOST_AUTO_TEST_CASE(a_horizontal_box_runs_across) {
    v3d::render::realtime::Canvas canvas;
    canvas.resize(400, 200);

    const boost::shared_ptr<v3d::ui::component::HorizontalBox> box =
        boost::make_shared<v3d::ui::component::HorizontalBox>();
    box->layout().width = v3d::ui::Length(300.0f, v3d::ui::Length::Unit::Pixels);
    box->layout().height = v3d::ui::Length(40.0f, v3d::ui::Length::Unit::Pixels);
    box->spacing(6.0f);
    const boost::shared_ptr<v3d::ui::component::Label> first = label("first", "abc");
    const boost::shared_ptr<v3d::ui::component::Label> second = label("second", "de");
    box->add(first);
    box->add(second);

    v3d::ui::Container container("hud", true);
    container.add(box);
    build().draw(&canvas, container);

    BOOST_CHECK_CLOSE(first->size().x, 3.0f * characterWidth, 0.001f);
    BOOST_CHECK_CLOSE(second->position().x, 3.0f * characterWidth + 6.0f, 0.001f);
}

/**
 * A container draws in z index order, and add order is what components of equal depth keep.
 **/
BOOST_AUTO_TEST_CASE(a_container_draws_in_depth_order) {
    v3d::ui::Container container("hud", true);
    const boost::shared_ptr<v3d::ui::component::Panel> first = panel("first");
    const boost::shared_ptr<v3d::ui::component::Panel> second = panel("second");
    const boost::shared_ptr<v3d::ui::component::Panel> third = panel("third");
    third->depth(5);
    container.add(third);
    container.add(first);
    container.add(second);

    const std::vector<boost::shared_ptr<v3d::ui::Component>> order = container.ordered();
    BOOST_REQUIRE_EQUAL(order.size(), 3U);
    BOOST_CHECK_EQUAL(order[0]->name(), "first");
    BOOST_CHECK_EQUAL(order[1]->name(), "second");
    BOOST_CHECK_EQUAL(order[2]->name(), "third");
}

/**
 * Nothing is picked until something has been drawn, only a pickable component takes the
 * point, and a child is offered it before the component holding it.
 **/
BOOST_AUTO_TEST_CASE(a_point_is_picked_by_the_deepest_pickable_component) {
    v3d::render::realtime::Canvas canvas;
    canvas.resize(400, 200);

    const boost::shared_ptr<v3d::ui::component::Panel> backdrop = panel("backdrop");
    backdrop->layout().width = v3d::ui::Length(100.0f, v3d::ui::Length::Unit::Percent);
    backdrop->layout().height = v3d::ui::Length(100.0f, v3d::ui::Length::Unit::Percent);
    const boost::shared_ptr<v3d::ui::component::Panel> plate = panel("plate");
    plate->layout().x = v3d::ui::Length(10.0f, v3d::ui::Length::Unit::Pixels);
    plate->layout().y = v3d::ui::Length(10.0f, v3d::ui::Length::Unit::Pixels);
    plate->layout().width = v3d::ui::Length(50.0f, v3d::ui::Length::Unit::Pixels);
    plate->layout().height = v3d::ui::Length(50.0f, v3d::ui::Length::Unit::Pixels);
    backdrop->add(plate);

    v3d::ui::Container container("hud", true);
    container.add(backdrop);

    // nothing has been drawn, so nothing has a box to be tested against
    BOOST_CHECK(!container.pick(glm::vec2(20.0f, 20.0f)));

    build().draw(&canvas, container);

    // and nothing is pickable until it says so
    BOOST_CHECK(!container.pick(glm::vec2(20.0f, 20.0f)));

    backdrop->pickable(true);
    BOOST_CHECK_EQUAL(container.pick(glm::vec2(20.0f, 20.0f))->name(), "backdrop");

    plate->pickable(true);
    BOOST_CHECK_EQUAL(container.pick(glm::vec2(20.0f, 20.0f))->name(), "plate");
    // outside the plate, the backdrop under it still takes the point
    BOOST_CHECK_EQUAL(container.pick(glm::vec2(300.0f, 150.0f))->name(), "backdrop");
}

/**
 * A bar is a track with a fraction of it filled. A square one is four runs of outline around
 * one quad of track, and one more for the fill; an empty one draws no fill at all.
 **/
BOOST_AUTO_TEST_CASE(a_bar_fills_a_fraction_of_its_track) {
    v3d::render::realtime::Canvas canvas;
    canvas.resize(400, 200);

    const boost::shared_ptr<v3d::ui::component::Bar> bar = boost::make_shared<v3d::ui::component::Bar>();
    bar->name("health");
    bar->layout().x = v3d::ui::Length(10.0f, v3d::ui::Length::Unit::Pixels);
    bar->layout().y = v3d::ui::Length(10.0f, v3d::ui::Length::Unit::Pixels);
    bar->layout().width = v3d::ui::Length(100.0f, v3d::ui::Length::Unit::Pixels);
    bar->layout().height = v3d::ui::Length(12.0f, v3d::ui::Length::Unit::Pixels);

    v3d::ui::Container container("hud", true);
    container.add(bar);
    v3d::ui::ComponentRenderer renderer = build();

    renderer.draw(&canvas, container);
    // the four runs of the outline and the track, and nothing filled
    BOOST_CHECK_EQUAL(canvas.indices().size(), 5U * 6U);

    bar->fraction(0.5f);
    canvas.clear();
    renderer.draw(&canvas, container);
    BOOST_CHECK_EQUAL(canvas.indices().size(), 6U * 6U);

    // a fraction is a fraction, however it was given
    bar->fraction(4.0f);
    BOOST_CHECK_CLOSE(bar->fraction(), 1.0f, 0.001f);
}

/**
 * A rounded panel is three bands and four wedges rather than one quad, and all of it lands in
 * the one untextured batch.
 **/
BOOST_AUTO_TEST_CASE(a_rounded_panel_is_bands_and_wedges_in_one_batch) {
    v3d::render::realtime::Canvas canvas;
    canvas.resize(400, 200);

    const boost::shared_ptr<v3d::ui::component::Panel> plate = panel("plate");
    plate->layout().width = v3d::ui::Length(80.0f, v3d::ui::Length::Unit::Pixels);
    plate->layout().height = v3d::ui::Length(40.0f, v3d::ui::Length::Unit::Pixels);

    v3d::ui::Container container("hud", true);
    container.add(plate);
    v3d::ui::ComponentRenderer renderer = build();
    renderer.dressing().borderWidth = 0.0f;

    renderer.draw(&canvas, container);
    BOOST_CHECK_EQUAL(canvas.indices().size(), 6U);

    renderer.dressing().radius = 6.0f;
    canvas.clear();
    renderer.draw(&canvas, container);
    // three quads and four six segment wedges
    BOOST_CHECK_EQUAL(canvas.indices().size(), 3U * 6U + 4U * 6U * 3U);
    BOOST_CHECK_EQUAL(canvas.batches().size(), 1U);
}

/**
 * A component that asks to clip cuts what it holds off at its own box, so the batch its
 * children are drawn in carries that box for the device to scissor to - ADR-0037. The
 * parent's own quads are not cut: a panel draws inside itself already.
 **/
BOOST_AUTO_TEST_CASE(a_component_that_clips_cuts_its_children_to_its_box) {
    v3d::render::realtime::Canvas canvas;
    canvas.resize(400, 200);

    const boost::shared_ptr<v3d::ui::component::Panel> outer = panel("outer");
    outer->layout().width = v3d::ui::Length(100.0f, v3d::ui::Length::Unit::Pixels);
    outer->layout().height = v3d::ui::Length(50.0f, v3d::ui::Length::Unit::Pixels);
    outer->layout().x = v3d::ui::Length(20.0f, v3d::ui::Length::Unit::Pixels);
    outer->layout().y = v3d::ui::Length(30.0f, v3d::ui::Length::Unit::Pixels);
    outer->clip(true);

    const boost::shared_ptr<v3d::ui::component::Panel> inner = panel("inner");
    inner->layout().width = v3d::ui::Length(400.0f, v3d::ui::Length::Unit::Pixels);
    inner->layout().height = v3d::ui::Length(400.0f, v3d::ui::Length::Unit::Pixels);
    outer->add(inner);

    v3d::ui::Container container("hud", true);
    container.add(outer);
    build().draw(&canvas, container);

    // the parent's plate, then the child's under the clip
    BOOST_REQUIRE(canvas.batches().size() >= 2U);
    BOOST_CHECK(!canvas.batches().front().clipped);
    const v3d::render::realtime::Canvas::Batch& cut = canvas.batches().back();
    BOOST_REQUIRE(cut.clipped);
    BOOST_CHECK_CLOSE(cut.clip.x, 20.0f, 0.001f);
    BOOST_CHECK_CLOSE(cut.clip.y, 30.0f, 0.001f);
    BOOST_CHECK_CLOSE(cut.clip.z, 120.0f, 0.001f);
    BOOST_CHECK_CLOSE(cut.clip.w, 80.0f, 0.001f);
    // and the child is left holding the box it asked for rather than the one it can show,
    // which is what the cursor is still tested against
    BOOST_CHECK_CLOSE(inner->size().x, 400.0f, 0.001f);
}

/**
 * Clipping is what a component asked for and not the default, because a menu drops a panel
 * out of the strip it came from and a badge sits half outside its plate.
 **/
BOOST_AUTO_TEST_CASE(a_component_that_does_not_ask_is_not_clipped) {
    v3d::render::realtime::Canvas canvas;
    canvas.resize(400, 200);

    const boost::shared_ptr<v3d::ui::component::Panel> outer = panel("outer");
    outer->layout().width = v3d::ui::Length(100.0f, v3d::ui::Length::Unit::Pixels);
    outer->layout().height = v3d::ui::Length(50.0f, v3d::ui::Length::Unit::Pixels);
    outer->add(panel("inner"));

    v3d::ui::Container container("hud", true);
    container.add(outer);
    build().draw(&canvas, container);

    BOOST_REQUIRE(!canvas.batches().empty());
    for (const v3d::render::realtime::Canvas::Batch& batch : canvas.batches()) {
        BOOST_CHECK(!batch.clipped);
    }
}

BOOST_AUTO_TEST_SUITE_END()
