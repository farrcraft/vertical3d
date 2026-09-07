/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <string>

#include <boost/test/unit_test.hpp>

#include "../ComponentRenderer.h"
#include "../Container.h"
#include "../component/Scrollbar.h"

#include <boost/make_shared.hpp>

namespace {

/**
 * A vertical bar of a given box, already placed the way a draw walk would leave it.
 **/
boost::shared_ptr<v3d::ui::component::Scrollbar> bar(const glm::vec2& position, const glm::vec2& size) {
    boost::shared_ptr<v3d::ui::component::Scrollbar> component =
        boost::make_shared<v3d::ui::component::Scrollbar>();
    component->name("scroll");
    component->position(position);
    component->size(size);
    return component;
}

};  // namespace

BOOST_AUTO_TEST_SUITE(scrollbar_test)

/**
 * A page showing all of its content has nowhere to go, and a thumb the length of its track.
 **/
BOOST_AUTO_TEST_CASE(a_page_that_shows_everything_does_not_scroll) {
    const boost::shared_ptr<v3d::ui::component::Scrollbar> component =
        bar(glm::vec2(0.0f, 0.0f), glm::vec2(10.0f, 200.0f));
    component->range(150.0f, 200.0f);

    BOOST_CHECK(!component->scrollable());
    BOOST_CHECK_CLOSE(component->maximum(), 0.0f, 0.001f);
    BOOST_CHECK_CLOSE(component->thumb(), 200.0f, 0.001f);
    BOOST_CHECK_CLOSE(component->thumbStart(), 0.0f, 0.001f);
}

/**
 * The thumb is as much of the track as the page is of the content, and it travels the rest
 * of the track as the offset crosses what is not shown.
 **/
BOOST_AUTO_TEST_CASE(the_thumb_is_the_fraction_of_the_content_that_is_shown) {
    const boost::shared_ptr<v3d::ui::component::Scrollbar> component =
        bar(glm::vec2(0.0f, 0.0f), glm::vec2(10.0f, 200.0f));
    component->range(400.0f, 200.0f);

    BOOST_CHECK(component->scrollable());
    BOOST_CHECK_CLOSE(component->maximum(), 200.0f, 0.001f);
    BOOST_CHECK_CLOSE(component->thumb(), 100.0f, 0.001f);
    BOOST_CHECK_CLOSE(component->thumbStart(), 0.0f, 0.001f);

    component->offset(100.0f);
    BOOST_CHECK_CLOSE(component->thumbStart(), 50.0f, 0.001f);

    component->offset(200.0f);
    BOOST_CHECK_CLOSE(component->thumbStart(), 100.0f, 0.001f);
}

/**
 * However little of the content is shown, the thumb stays big enough to take hold of.
 **/
BOOST_AUTO_TEST_CASE(the_thumb_has_a_floor) {
    const boost::shared_ptr<v3d::ui::component::Scrollbar> component =
        bar(glm::vec2(0.0f, 0.0f), glm::vec2(10.0f, 200.0f));
    component->range(100000.0f, 200.0f);

    BOOST_CHECK_CLOSE(component->thumb(), v3d::ui::component::Scrollbar::minimumThumb, 0.001f);
}

/**
 * The offset is clamped to what there is to scroll, on the way in and again when the range
 * changes - a list that shrank while scrolled to its end comes back to the end of what is
 * left rather than past it.
 **/
BOOST_AUTO_TEST_CASE(the_offset_is_clamped_to_what_there_is_to_scroll) {
    const boost::shared_ptr<v3d::ui::component::Scrollbar> component =
        bar(glm::vec2(0.0f, 0.0f), glm::vec2(10.0f, 200.0f));
    component->range(400.0f, 200.0f);

    component->offset(1000.0f);
    BOOST_CHECK_CLOSE(component->offset(), 200.0f, 0.001f);

    component->scroll(-1000.0f);
    BOOST_CHECK_CLOSE(component->offset(), 0.0f, 0.001f);

    component->offset(200.0f);
    component->range(250.0f, 200.0f);
    BOOST_CHECK_CLOSE(component->offset(), 50.0f, 0.001f);
}

/**
 * A drag takes the point as the middle of the thumb, so the top of the track scrolls to the
 * start and the bottom of it to the end whatever the thumb's length is.
 **/
BOOST_AUTO_TEST_CASE(a_drag_puts_the_middle_of_the_thumb_under_the_cursor) {
    const boost::shared_ptr<v3d::ui::component::Scrollbar> component =
        bar(glm::vec2(300.0f, 100.0f), glm::vec2(10.0f, 200.0f));
    component->range(400.0f, 200.0f);

    // the middle of a 100 pixel thumb on a 200 pixel track, which is the middle of the range
    component->drag(glm::vec2(305.0f, 200.0f));
    BOOST_CHECK_CLOSE(component->offset(), 100.0f, 0.001f);

    component->drag(glm::vec2(305.0f, 100.0f));
    BOOST_CHECK_CLOSE(component->offset(), 0.0f, 0.001f);

    component->drag(glm::vec2(305.0f, 300.0f));
    BOOST_CHECK_CLOSE(component->offset(), 200.0f, 0.001f);
}

/**
 * A horizontal bar is the same arithmetic along the other axis, and reads the cursor's x.
 **/
BOOST_AUTO_TEST_CASE(a_horizontal_bar_scrolls_across) {
    const boost::shared_ptr<v3d::ui::component::Scrollbar> component =
        bar(glm::vec2(0.0f, 0.0f), glm::vec2(200.0f, 10.0f));
    component->direction(v3d::ui::component::Scrollbar::Direction::Horizontal);
    component->range(400.0f, 200.0f);

    BOOST_CHECK_CLOSE(component->thumb(), 100.0f, 0.001f);
    // the middle of the track, which is the middle of the range
    component->drag(glm::vec2(100.0f, 5.0f));
    BOOST_CHECK_CLOSE(component->offset(), 100.0f, 0.001f);
}

/**
 * A bar that has never been drawn has no track, so a drag on it scrolls nowhere rather
 * than dividing by a zero length - the same rule as picking one, per ADR-0019.
 **/
BOOST_AUTO_TEST_CASE(a_bar_that_was_never_drawn_scrolls_nowhere) {
    const boost::shared_ptr<v3d::ui::component::Scrollbar> component =
        bar(glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, 0.0f));
    component->range(400.0f, 200.0f);

    component->drag(glm::vec2(50.0f, 50.0f));
    BOOST_CHECK_CLOSE(component->offset(), 0.0f, 0.001f);
}

/**
 * The track is drawn whether or not there is anything to scroll, and the thumb only when
 * there is - a full length thumb would read as a bar scrolled nowhere rather than as one
 * with nowhere to go.
 **/
BOOST_AUTO_TEST_CASE(a_bar_with_nothing_to_scroll_draws_its_track_alone) {
    v3d::ui::ComponentRenderer renderer(
        [](const std::string& text) { return static_cast<float>(text.size()) * 10.0f; },
        [](const std::string&, const glm::vec2&, const glm::vec4&) {});

    v3d::render::realtime::Canvas canvas;
    canvas.resize(400, 300);

    const boost::shared_ptr<v3d::ui::component::Scrollbar> empty =
        bar(glm::vec2(0.0f, 0.0f), glm::vec2(10.0f, 200.0f));
    empty->range(100.0f, 200.0f);
    renderer.draw(&canvas, empty);
    const std::size_t track = canvas.indices().size();
    BOOST_CHECK(track > 0);

    canvas.clear();
    const boost::shared_ptr<v3d::ui::component::Scrollbar> scrolled =
        bar(glm::vec2(0.0f, 0.0f), glm::vec2(10.0f, 200.0f));
    scrolled->range(400.0f, 200.0f);
    renderer.draw(&canvas, scrolled);

    BOOST_CHECK(canvas.indices().size() > track);
}

BOOST_AUTO_TEST_SUITE_END()
