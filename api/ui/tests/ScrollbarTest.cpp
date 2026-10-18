/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/canvas/Canvas.h>
#include <api/ui/Container.h>
#include <api/ui/component/Scrollbar.h>
#include <api/ui/component/SelectList.h>
#include <api/ui/paint/ComponentRenderer.h>

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <boost/make_shared.hpp>

namespace {

/**
 * A vertical bar of a given box, already placed the way a draw would leave it.
 **/
boost::shared_ptr<v3d::ui::component::Scrollbar> bar(const v3d::type::geometry::Bound2D::Area& box) {
    boost::shared_ptr<v3d::ui::component::Scrollbar> component =
        boost::make_shared<v3d::ui::component::Scrollbar>();
    component->name("scroll");
    component->position(box.position);
    component->size(box.size);
    return component;
}

/**
 * A list of numbered rows, already placed and told how tall a row is drawn - both of which
 * a draw leaves on it.
 **/
/** How many rows a list holds, and how tall each is drawn. **/
struct Rows {
    std::size_t count = 0;
    float height = 0.0F;
};

boost::shared_ptr<v3d::ui::component::SelectList> list(const Rows& shape, const v3d::type::geometry::Bound2D::Area& box) {
    const std::size_t rows = shape.count;
    const float rowHeight = shape.height;
    const glm::vec2& position = box.position;
    const glm::vec2& size = box.size;
    boost::shared_ptr<v3d::ui::component::SelectList> component =
        boost::make_shared<v3d::ui::component::SelectList>();
    component->name("list");
    std::vector<std::string> items;
    items.reserve(rows);
    for (std::size_t row = 0; row < rows; row++) {
        items.push_back("row " + std::to_string(row));
    }
    component->items(items);
    component->rowHeight(rowHeight);
    component->position(position);
    component->size(size);
    return component;
}

BOOST_AUTO_TEST_SUITE(scrollbar_test)

/**
 * A page showing all of its content has nowhere to go, and a thumb the length of its track.
 **/
BOOST_AUTO_TEST_CASE(a_page_that_shows_everything_does_not_scroll) {
    const boost::shared_ptr<v3d::ui::component::Scrollbar> component =
        bar({.position = glm::vec2(0.0F, 0.0F), .size = glm::vec2(10.0F, 200.0F)});
    component->range(150.0F, 200.0F);

    BOOST_CHECK(!component->scrollable());
    BOOST_CHECK_CLOSE(component->maximum(), 0.0F, 0.001F);
    BOOST_CHECK_CLOSE(component->thumb(), 200.0F, 0.001F);
    BOOST_CHECK_CLOSE(component->thumbStart(), 0.0F, 0.001F);
}

/**
 * The thumb is as much of the track as the page is of the content, and it travels the rest
 * of the track as the offset crosses what is not shown.
 **/
BOOST_AUTO_TEST_CASE(the_thumb_is_the_fraction_of_the_content_that_is_shown) {
    const boost::shared_ptr<v3d::ui::component::Scrollbar> component =
        bar({.position = glm::vec2(0.0F, 0.0F), .size = glm::vec2(10.0F, 200.0F)});
    component->range(400.0F, 200.0F);

    BOOST_CHECK(component->scrollable());
    BOOST_CHECK_CLOSE(component->maximum(), 200.0F, 0.001F);
    BOOST_CHECK_CLOSE(component->thumb(), 100.0F, 0.001F);
    BOOST_CHECK_CLOSE(component->thumbStart(), 0.0F, 0.001F);

    component->offset(100.0F);
    BOOST_CHECK_CLOSE(component->thumbStart(), 50.0F, 0.001F);

    component->offset(200.0F);
    BOOST_CHECK_CLOSE(component->thumbStart(), 100.0F, 0.001F);
}

/**
 * However little of the content is shown, the thumb stays big enough to take hold of.
 **/
BOOST_AUTO_TEST_CASE(the_thumb_has_a_floor) {
    const boost::shared_ptr<v3d::ui::component::Scrollbar> component =
        bar({.position = glm::vec2(0.0F, 0.0F), .size = glm::vec2(10.0F, 200.0F)});
    component->range(100000.0F, 200.0F);

    BOOST_CHECK_CLOSE(component->thumb(), v3d::ui::component::Scrollbar::minimumThumb, 0.001F);
}

/**
 * The offset is clamped to what there is to scroll, on the way in and again when the range
 * changes - a list that shrank while scrolled to its end comes back to the end of what is
 * left rather than past it.
 **/
BOOST_AUTO_TEST_CASE(the_offset_is_clamped_to_what_there_is_to_scroll) {
    const boost::shared_ptr<v3d::ui::component::Scrollbar> component =
        bar({.position = glm::vec2(0.0F, 0.0F), .size = glm::vec2(10.0F, 200.0F)});
    component->range(400.0F, 200.0F);

    component->offset(1000.0F);
    BOOST_CHECK_CLOSE(component->offset(), 200.0F, 0.001F);

    component->scroll(-1000.0F);
    BOOST_CHECK_CLOSE(component->offset(), 0.0F, 0.001F);

    component->offset(200.0F);
    component->range(250.0F, 200.0F);
    BOOST_CHECK_CLOSE(component->offset(), 50.0F, 0.001F);
}

/**
 * A drag takes the point as the middle of the thumb, so the top of the track scrolls to the
 * start and the bottom of it to the end whatever the thumb's length is.
 **/
BOOST_AUTO_TEST_CASE(a_drag_puts_the_middle_of_the_thumb_under_the_cursor) {
    const boost::shared_ptr<v3d::ui::component::Scrollbar> component =
        bar({.position = glm::vec2(300.0F, 100.0F), .size = glm::vec2(10.0F, 200.0F)});
    component->range(400.0F, 200.0F);

    // the middle of a 100 pixel thumb on a 200 pixel track, which is the middle of the range
    component->drag(glm::vec2(305.0F, 200.0F));
    BOOST_CHECK_CLOSE(component->offset(), 100.0F, 0.001F);

    component->drag(glm::vec2(305.0F, 100.0F));
    BOOST_CHECK_CLOSE(component->offset(), 0.0F, 0.001F);

    component->drag(glm::vec2(305.0F, 300.0F));
    BOOST_CHECK_CLOSE(component->offset(), 200.0F, 0.001F);
}

/**
 * A horizontal bar is the same arithmetic along the other axis, and reads the cursor's x.
 **/
BOOST_AUTO_TEST_CASE(a_horizontal_bar_scrolls_across) {
    const boost::shared_ptr<v3d::ui::component::Scrollbar> component =
        bar({.position = glm::vec2(0.0F, 0.0F), .size = glm::vec2(200.0F, 10.0F)});
    component->direction(v3d::ui::component::Scrollbar::Direction::Horizontal);
    component->range(400.0F, 200.0F);

    BOOST_CHECK_CLOSE(component->thumb(), 100.0F, 0.001F);
    // the middle of the track, which is the middle of the range
    component->drag(glm::vec2(100.0F, 5.0F));
    BOOST_CHECK_CLOSE(component->offset(), 100.0F, 0.001F);
}

/**
 * A bar that has never been drawn has no track, so a drag on it scrolls nowhere rather
 * than dividing by a zero length.
 **/
BOOST_AUTO_TEST_CASE(a_bar_that_was_never_drawn_scrolls_nowhere) {
    const boost::shared_ptr<v3d::ui::component::Scrollbar> component =
        bar({.position = glm::vec2(0.0F, 0.0F), .size = glm::vec2(0.0F, 0.0F)});
    component->range(400.0F, 200.0F);

    component->drag(glm::vec2(50.0F, 50.0F));
    BOOST_CHECK_CLOSE(component->offset(), 0.0F, 0.001F);
}

/**
 * The track is drawn whether or not there is anything to scroll, and the thumb only when
 * there is - a full length thumb would read as a bar scrolled nowhere rather than as one
 * with nowhere to go.
 **/
BOOST_AUTO_TEST_CASE(a_bar_with_nothing_to_scroll_draws_its_track_alone) {
    const v3d::ui::paint::ComponentRenderer renderer(
        [](std::string_view text) { return static_cast<float>(text.size()) * 10.0F; },
        [](std::string_view, const glm::vec2&, const glm::vec4&) {});

    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));

    const boost::shared_ptr<v3d::ui::component::Scrollbar> empty =
        bar({.position = glm::vec2(0.0F, 0.0F), .size = glm::vec2(10.0F, 200.0F)});
    empty->range(100.0F, 200.0F);
    renderer.draw(&canvas, empty);
    const std::size_t track = canvas.indices().size();
    BOOST_CHECK(track > 0);

    canvas.clear();
    const boost::shared_ptr<v3d::ui::component::Scrollbar> scrolled =
        bar({.position = glm::vec2(0.0F, 0.0F), .size = glm::vec2(10.0F, 200.0F)});
    scrolled->range(400.0F, 200.0F);
    renderer.draw(&canvas, scrolled);

    BOOST_CHECK(canvas.indices().size() > track);
}

/**
 * A bar told which list it scrolls holds no range of its own: what it reports is the list's
 * content, the list's box as the page, and the list's offset.
 **/
BOOST_AUTO_TEST_CASE(a_bound_bar_reports_the_lists_range) {
    const boost::shared_ptr<v3d::ui::component::SelectList> rows =
        list({.count = 20, .height = 10.0F}, {.position = glm::vec2(0.0F, 0.0F), .size = glm::vec2(100.0F, 50.0F)});
    const boost::shared_ptr<v3d::ui::component::Scrollbar> component =
        bar({.position = glm::vec2(100.0F, 0.0F), .size = glm::vec2(10.0F, 50.0F)});

    // a range of its own first, so the binding can be seen to replace it
    component->range(1000.0F, 100.0F);
    BOOST_CHECK_CLOSE(component->content(), 1000.0F, 0.001F);

    component->scrolls(rows);
    BOOST_CHECK(component->scrolls() == rows);
    BOOST_CHECK_CLOSE(component->content(), 200.0F, 0.001F);
    BOOST_CHECK_CLOSE(component->page(), 50.0F, 0.001F);
    BOOST_CHECK_CLOSE(component->maximum(), 150.0F, 0.001F);
    BOOST_CHECK(component->scrollable());

    // the offset is the list's, in both directions
    rows->offset(30.0F);
    BOOST_CHECK_CLOSE(component->offset(), 30.0F, 0.001F);
    component->scroll(20.0F);
    BOOST_CHECK_CLOSE(rows->offset(), 50.0F, 0.001F);
    BOOST_CHECK_CLOSE(component->offset(), 50.0F, 0.001F);

    // and unbinding puts back the range it was given
    component->scrolls(boost::shared_ptr<v3d::ui::component::SelectList>());
    BOOST_CHECK_CLOSE(component->content(), 1000.0F, 0.001F);
}

/**
 * Dragging a bound bar's thumb scrolls the list.
 **/
BOOST_AUTO_TEST_CASE(dragging_a_bound_bar_moves_the_list) {
    const boost::shared_ptr<v3d::ui::component::SelectList> rows =
        list({.count = 20, .height = 10.0F}, {.position = glm::vec2(0.0F, 0.0F), .size = glm::vec2(100.0F, 50.0F)});
    const boost::shared_ptr<v3d::ui::component::Scrollbar> component =
        bar({.position = glm::vec2(100.0F, 0.0F), .size = glm::vec2(10.0F, 50.0F)});
    component->scrolls(rows);

    // the far end of the track is the end of the list
    component->drag(glm::vec2(105.0F, 50.0F));
    BOOST_CHECK_CLOSE(rows->offset(), rows->content() - rows->size().y, 0.001F);

    // and the near end is the start of it
    component->drag(glm::vec2(105.0F, 0.0F));
    BOOST_CHECK_CLOSE(rows->offset(), 0.0F, 0.001F);

    // a drag past the end is clamped by the list rather than running off it
    component->drag(glm::vec2(105.0F, 500.0F));
    BOOST_CHECK_CLOSE(rows->offset(), rows->content() - rows->size().y, 0.001F);
}

/**
 * A list that fits its box has nothing to scroll, so the bar draws a track and no thumb -
 * the same answer an unbound bar showing all of its content gives.
 **/
BOOST_AUTO_TEST_CASE(a_bound_bar_with_nothing_to_scroll_draws_no_thumb) {
    const boost::shared_ptr<v3d::ui::component::SelectList> rows =
        list({.count = 3, .height = 10.0F}, {.position = glm::vec2(0.0F, 0.0F), .size = glm::vec2(100.0F, 50.0F)});
    const boost::shared_ptr<v3d::ui::component::Scrollbar> component =
        bar({.position = glm::vec2(100.0F, 0.0F), .size = glm::vec2(10.0F, 50.0F)});
    component->scrolls(rows);

    BOOST_CHECK(!component->scrollable());
    BOOST_CHECK_CLOSE(component->thumb(), 50.0F, 0.001F);

    const v3d::ui::paint::ComponentRenderer renderer(
        [](std::string_view text) { return static_cast<float>(text.size()) * 10.0F; },
        [](std::string_view, const glm::vec2&, const glm::vec4&) {});
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(800, 600));
    renderer.draw(&canvas, component);
    const std::size_t track = canvas.indices().size();

    // and the same bar over a list with more rows than it shows draws the thumb as well
    canvas.clear();
    const boost::shared_ptr<v3d::ui::component::SelectList> many =
        list({.count = 40, .height = 10.0F}, {.position = glm::vec2(0.0F, 0.0F), .size = glm::vec2(100.0F, 50.0F)});
    component->scrolls(many);
    renderer.draw(&canvas, component);
    BOOST_CHECK(canvas.indices().size() > track);
}

/**
 * A bar whose list has been unloaded reads as its own range again rather than following a
 * dangling pointer. The bar belongs to its container and so did the list.
 **/
BOOST_AUTO_TEST_CASE(a_bar_whose_list_has_gone_does_not_crash) {
    const boost::shared_ptr<v3d::ui::component::Scrollbar> component =
        bar({.position = glm::vec2(100.0F, 0.0F), .size = glm::vec2(10.0F, 50.0F)});
    component->range(200.0F, 50.0F);
    {
        const boost::shared_ptr<v3d::ui::component::SelectList> rows =
            list({.count = 40, .height = 10.0F}, {.position = glm::vec2(0.0F, 0.0F), .size = glm::vec2(100.0F, 50.0F)});
        component->scrolls(rows);
        BOOST_CHECK_CLOSE(component->content(), 400.0F, 0.001F);
    }

    BOOST_CHECK(!component->scrolls());
    BOOST_CHECK_CLOSE(component->content(), 200.0F, 0.001F);
    component->drag(glm::vec2(105.0F, 50.0F));
    BOOST_CHECK_CLOSE(component->offset(), 150.0F, 0.001F);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
