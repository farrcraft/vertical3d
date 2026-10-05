/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/asset/Manager.h>
#include <api/log/Logger.h>
#include <api/ui/shell/Screen.h>
#include <api/ui/style/Theme.h>

#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

namespace {

/**
 * A screen with no renderer behind it. Its text has no font, since there is no device to
 * take an atlas, but everything over the text is built and can be asked about.
 **/
v3d::ui::shell::Screen::Options sized(float size) {
    v3d::ui::shell::Screen::Options options;
    options.size = size;
    return options;
}

boost::shared_ptr<v3d::asset::Manager> nowhere() {
    return boost::make_shared<v3d::asset::Manager>("no-such-directory/", boost::make_shared<v3d::log::Logger>());
}

};  // namespace

BOOST_AUTO_TEST_SUITE(screen_test)

/**
 * With no renderer there is no frame to begin, and nothing is sized or reported as resized.
 **/
BOOST_AUTO_TEST_CASE(a_screen_with_no_renderer_begins_no_frame) {
    v3d::ui::shell::Screen screen(nullptr, nowhere(), boost::make_shared<v3d::log::Logger>(), sized(20.0f));

    BOOST_CHECK(!screen.begin());
    BOOST_CHECK(!screen.resized());
    BOOST_CHECK_EQUAL(screen.canvas().width(), 0u);
    BOOST_CHECK(screen.text());
    BOOST_CHECK(screen.statistics());
    BOOST_CHECK(screen.immediate() == nullptr);
}

/**
 * The dressing is filled in for the size the text is drawn at: a line and four tenths when
 * the app names none, and whatever the app's function says when it does.
 **/
BOOST_AUTO_TEST_CASE(the_dressing_is_filled_in_for_the_size) {
    v3d::ui::shell::Screen plain(nullptr, nowhere(), boost::make_shared<v3d::log::Logger>(), sized(20.0f));
    BOOST_CHECK_CLOSE(plain.components().dressing().lineHeight, 28.0f, 0.001f);

    v3d::ui::shell::Screen::Options options = sized(10.0f);
    options.dress = [](v3d::ui::paint::Dressing* dressing, float size) {
        dressing->lineHeight = size * 1.5f;
        dressing->barHeight = size * 1.8f;
    };
    options.immediate = true;
    v3d::ui::shell::Screen dressed(nullptr, nowhere(), boost::make_shared<v3d::log::Logger>(), options);
    BOOST_CHECK_CLOSE(dressed.components().dressing().lineHeight, 15.0f, 0.001f);
    BOOST_CHECK_CLOSE(dressed.components().dressing().barHeight, 18.0f, 0.001f);
    BOOST_REQUIRE(dressed.immediate() != nullptr);
    BOOST_CHECK_CLOSE(dressed.immediate()->dressing().lineHeight, 15.0f, 0.001f);
}

/**
 * A rescale rebuilds what closes over the size and dresses it for the new one. The theme goes
 * to what was rebuilt. The immediate layer is the same layer, given the new text, and the
 * overlay is the same overlay, at a new size and as visible as it
 * was - so an app that toggles it through a handle it kept still reaches the one drawn.
 **/
BOOST_AUTO_TEST_CASE(a_rescale_rebuilds_what_closes_over_the_size) {
    v3d::ui::shell::Screen::Options options = sized(20.0f);
    options.immediate = true;
    v3d::ui::shell::Screen screen(nullptr, nowhere(), boost::make_shared<v3d::log::Logger>(), options);
    const boost::shared_ptr<v3d::ui::style::Theme> theme = boost::make_shared<v3d::ui::style::Theme>("dark");
    screen.theme(theme);
    const boost::shared_ptr<v3d::ui::shell::StatisticsOverlay> overlay = screen.statistics();
    overlay->toggle();
    const v3d::ui::paint::ComponentRenderer* before = &screen.components();
    const v3d::ui::Immediate* layer = screen.immediate();

    screen.scale(1.5f);

    BOOST_CHECK_CLOSE(screen.size(), 30.0f, 0.001f);
    BOOST_CHECK(&screen.components() != before);
    BOOST_CHECK_CLOSE(screen.components().dressing().lineHeight, 42.0f, 0.001f);
    BOOST_CHECK(screen.components().theme() == theme);
    BOOST_REQUIRE(screen.immediate() != nullptr);
    BOOST_CHECK_CLOSE(screen.immediate()->dressing().lineHeight, 42.0f, 0.001f);
    // the immediate layer is the one that was there, so what it remembered is kept
    BOOST_CHECK(screen.immediate() == layer);

    BOOST_CHECK(screen.statistics() == overlay);
    BOOST_CHECK(overlay->visible());
    BOOST_CHECK_CLOSE(overlay->size(), v3d::ui::shell::StatisticsOverlay::defaultSize * 1.5f, 0.001f);
}

/**
 * A scale of nothing or less is not a size, and is ignored.
 **/
BOOST_AUTO_TEST_CASE(a_scale_that_is_not_a_size_is_ignored) {
    v3d::ui::shell::Screen screen(nullptr, nowhere(), boost::make_shared<v3d::log::Logger>(), sized(20.0f));
    screen.scale(0.0f);
    screen.scale(-1.0f);
    BOOST_CHECK_EQUAL(screen.scale(), 1.0f);
    BOOST_CHECK_CLOSE(screen.size(), 20.0f, 0.001f);
}

/**
 * Without the overlay asked for there is none, and drawing with no ui and no overlay puts
 * nothing on the canvas.
 **/
BOOST_AUTO_TEST_CASE(a_screen_without_statistics_draws_nothing_for_none) {
    v3d::ui::shell::Screen::Options options = sized(20.0f);
    options.statistics = false;
    v3d::ui::shell::Screen screen(nullptr, nowhere(), boost::make_shared<v3d::log::Logger>(), options);
    BOOST_CHECK(!screen.statistics());

    screen.draw(nullptr, v3d::ui::shell::StatisticsOverlay::Sample());
    BOOST_CHECK(screen.canvas().empty());
}

BOOST_AUTO_TEST_SUITE_END()
