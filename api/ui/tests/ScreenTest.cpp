/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/asset/Manager.h>
#include <api/event/Router.h>
#include <api/log/Logger.h>
#include <api/ui/Interface.h>
#include <api/ui/shell/Screen.h>
#include <api/ui/shell/StatisticsOverlay.h>
#include <api/ui/style/Theme.h>

#include <boost/json/parse.hpp>
#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

#include <entt/entt.hpp>

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

/**
 * A ui with two themes, "dark" active. "light" sets the immediate layer's line height.
 **/
boost::shared_ptr<v3d::ui::Interface> themed() {
    const boost::shared_ptr<entt::dispatcher> dispatcher = boost::make_shared<entt::dispatcher>();
    const boost::shared_ptr<v3d::ui::Interface> ui = boost::make_shared<v3d::ui::Interface>(
        boost::make_shared<v3d::event::Router>(dispatcher), dispatcher, boost::make_shared<v3d::log::Logger>());
    const bool loaded = ui->load(boost::json::parse(R"({
        "themes": [
            { "name": "dark" },
            { "name": "light", "styles": [ { "class": "tools", "name": "default",
                "numbers": [ { "name": "line-height", "value": 9 } ] } ] }
        ],
        "theme": "dark",
        "containers": []
    })").as_object());
    BOOST_REQUIRE(loaded);
    return ui;
}

BOOST_AUTO_TEST_SUITE(screen_test)

/**
 * With no renderer there is no frame to begin, and nothing is sized or reported as resized.
 **/
BOOST_AUTO_TEST_CASE(a_screen_with_no_renderer_begins_no_frame) {
    v3d::ui::shell::Screen screen(nullptr, nowhere(), boost::make_shared<v3d::log::Logger>(), sized(20.0F));

    BOOST_CHECK(!screen.begin(nullptr));
    BOOST_CHECK(!screen.resized());
    BOOST_CHECK_EQUAL(screen.canvas().width(), 0U);
    BOOST_CHECK(screen.text());
    BOOST_CHECK(screen.statistics());
    BOOST_CHECK(screen.immediate() == nullptr);
}

/**
 * The dressing is filled in for the size the text is drawn at: a line and four tenths when
 * the app names none, and whatever the app's function says when it does.
 **/
BOOST_AUTO_TEST_CASE(the_dressing_is_filled_in_for_the_size) {
    v3d::ui::shell::Screen plain(nullptr, nowhere(), boost::make_shared<v3d::log::Logger>(), sized(20.0F));
    BOOST_CHECK_CLOSE(plain.components().dressing().lineHeight, 28.0F, 0.001F);

    v3d::ui::shell::Screen::Options options = sized(10.0F);
    options.dress = [](v3d::ui::paint::Dressing* dressing, float size) {
        dressing->lineHeight = size * 1.5F;
        dressing->barHeight = size * 1.8F;
    };
    options.immediate = true;
    v3d::ui::shell::Screen dressed(nullptr, nowhere(), boost::make_shared<v3d::log::Logger>(), options);
    BOOST_CHECK_CLOSE(dressed.components().dressing().lineHeight, 15.0F, 0.001F);
    BOOST_CHECK_CLOSE(dressed.components().dressing().barHeight, 18.0F, 0.001F);
    BOOST_REQUIRE(dressed.immediate() != nullptr);
    BOOST_CHECK_CLOSE(dressed.immediate()->dressing().lineHeight, 15.0F, 0.001F);
}

/**
 * A frame is drawn with the ui's active theme. Changing the active theme on the ui changes
 * what the component renderer and the immediate layer draw with at the next begin().
 **/
BOOST_AUTO_TEST_CASE(a_frame_is_drawn_with_the_uis_active_theme) {
    v3d::ui::shell::Screen::Options options = sized(20.0F);
    options.immediate = true;
    v3d::ui::shell::Screen screen(nullptr, nowhere(), boost::make_shared<v3d::log::Logger>(), options);
    const boost::shared_ptr<v3d::ui::Interface> ui = themed();
    const float lineHeight = screen.immediate()->dressing().lineHeight;

    BOOST_CHECK(!screen.begin(ui.get()));
    BOOST_CHECK(screen.components().theme() == ui->theme("dark"));
    BOOST_CHECK_CLOSE(screen.immediate()->dressing().lineHeight, lineHeight, 0.001F);

    BOOST_REQUIRE(ui->activeTheme("light"));
    screen.begin(ui.get());
    BOOST_CHECK(screen.components().theme() == ui->theme("light"));
    BOOST_CHECK_CLOSE(screen.immediate()->dressing().lineHeight, 9.0F, 0.001F);

    // no ui keeps the theme drawn with
    screen.begin(nullptr);
    BOOST_CHECK(screen.components().theme() == ui->theme("light"));
}

/**
 * A rescale rebuilds what closes over the size and dresses it for the new one. The theme goes
 * to what was rebuilt. The immediate layer is the same layer, given the new text. The
 * overlay is the same overlay, at a new size and as visible as it was. An app that toggles
 * it through a handle it kept still reaches the one drawn.
 **/
BOOST_AUTO_TEST_CASE(a_rescale_rebuilds_what_closes_over_the_size) {
    v3d::ui::shell::Screen::Options options = sized(20.0F);
    options.immediate = true;
    v3d::ui::shell::Screen screen(nullptr, nowhere(), boost::make_shared<v3d::log::Logger>(), options);
    const boost::shared_ptr<v3d::ui::Interface> ui = themed();
    screen.begin(ui.get());
    const boost::shared_ptr<v3d::ui::style::Theme> theme = ui->activeTheme();
    const boost::shared_ptr<v3d::ui::shell::StatisticsOverlay> overlay = screen.statistics();
    overlay->toggle();
    const v3d::ui::paint::ComponentRenderer* before = &screen.components();
    const v3d::ui::Immediate* layer = screen.immediate();

    screen.scale(1.5F);

    BOOST_CHECK_CLOSE(screen.size(), 30.0F, 0.001F);
    BOOST_CHECK(&screen.components() != before);
    BOOST_CHECK_CLOSE(screen.components().dressing().lineHeight, 42.0F, 0.001F);
    BOOST_CHECK(screen.components().theme() == theme);
    BOOST_REQUIRE(screen.immediate() != nullptr);
    BOOST_CHECK_CLOSE(screen.immediate()->dressing().lineHeight, 42.0F, 0.001F);
    // the immediate layer is the one that was there, so what it remembered is kept
    BOOST_CHECK(screen.immediate() == layer);

    BOOST_CHECK(screen.statistics() == overlay);
    BOOST_CHECK(overlay->visible());
    BOOST_CHECK_CLOSE(overlay->size(), v3d::ui::shell::StatisticsOverlay::defaultSize * 1.5F, 0.001F);
}

/**
 * A scale of nothing or less is not a size, and is ignored.
 **/
BOOST_AUTO_TEST_CASE(a_scale_that_is_not_a_size_is_ignored) {
    v3d::ui::shell::Screen screen(nullptr, nowhere(), boost::make_shared<v3d::log::Logger>(), sized(20.0F));
    screen.scale(0.0F);
    screen.scale(-1.0F);
    BOOST_CHECK_EQUAL(screen.scale(), 1.0F);
    BOOST_CHECK_CLOSE(screen.size(), 20.0F, 0.001F);
}

/**
 * Without the overlay asked for there is none, and drawing with no ui and no overlay puts
 * nothing on the canvas.
 **/
BOOST_AUTO_TEST_CASE(a_screen_without_statistics_draws_nothing_for_none) {
    v3d::ui::shell::Screen::Options options = sized(20.0F);
    options.statistics = false;
    v3d::ui::shell::Screen screen(nullptr, nowhere(), boost::make_shared<v3d::log::Logger>(), options);
    BOOST_CHECK(!screen.statistics());

    screen.draw(nullptr, v3d::ui::shell::StatisticsOverlay::Sample());
    BOOST_CHECK(screen.canvas().empty());
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
