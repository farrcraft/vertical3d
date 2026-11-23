/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <api/image/Image.h>
#include <api/log/Logger.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/render/realtime/resource/Handle.h>
#include <api/type/camera/Profile.h>
#include <api/ui/Immediate.h>
#include <api/ui/tests/Typeset.h>
#include <vertical3d/src/control/RenderView.h>
#include <vertical3d/src/render/Batch.h>
#include <vertical3d/src/render/Delegate.h>
#include <vertical3d/src/scene/RenderSettings.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/view/AreaLayout.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <boost/filesystem/path.hpp>
#include <boost/json.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace {

using v3d::editor::control::RenderView;
using v3d::editor::render::Delegate;

/** A tick of a 60 Hz frame. **/
constexpr std::chrono::milliseconds FRAME{ 16 };

/**
 * A renderer that renders nothing, and reports whatever state and pictures it is set to.
 **/
class Fake final : public Delegate {
 public:
    Fake() {
        // each channel's picture is one pixel wide and as tall as the channel's value and one
        for (std::size_t channel = 0; channel < shown.size(); channel++) {
            shown.at(channel) = boost::make_shared<v3d::image::Image>(glm::uvec2(1, channel + 1), static_cast<std::uint8_t>(24));
        }
    }

    void cancel() override {
        cancelled++;
    }

    void resume() override {
        resumed++;
    }

    Status status() const override {
        return now;
    }

    void show(const Showing& asked) override {
        showing = asked;
        shows++;
    }

    boost::shared_ptr<v3d::image::Image> picture(Channel channel) const override {
        return shown.at(static_cast<std::size_t>(channel));
    }

    Status now;
    std::array<boost::shared_ptr<v3d::image::Image>, 4> shown;
    Showing showing;
    int shows = 0;
    int started = 0;
    int edited = 0;
    int cancelled = 0;
    int resumed = 0;

 protected:
    void start(const v3d::editor::scene::Scene& /*scene*/, const v3d::type::camera::Profile& /*from*/,
        const v3d::editor::scene::RenderSettings& /*settings*/) override {
        started++;
    }

    bool edit(const v3d::editor::scene::Scene& /*scene*/, const v3d::type::camera::Profile& /*from*/,
        const v3d::editor::scene::RenderSettings& /*settings*/) override {
        edited++;
        return true;
    }
};

/**
 * A render view over a fake renderer, drawn into a 600 by 400 region at the canvas's corner. The
 * layer measures ten pixels a character and records what it writes. Each picture uploaded is
 * kept, and its texture is the count of uploads.
 **/
struct View {
    View() :
        delegate(boost::make_shared<Fake>()),
        layer(v3d::ui::typeset::mono(&written)),
        view(delegate, boost::make_shared<v3d::editor::render::Batch>(boost::make_shared<v3d::log::Logger>(), boost::filesystem::path("moya.exe")),
            [this](const std::string& /*slot*/, const boost::shared_ptr<v3d::image::Image>& image) {
                uploaded.push_back(image);
                return v3d::render::realtime::resource::TextureHandle(static_cast<std::uint32_t>(uploaded.size()));
            }) {
        canvas.resize(glm::uvec2(700, 500));
        frame(hover(glm::vec2(690.0F, 490.0F)));
    }

    /**
     * Draw one frame of the view under some input.
     * @return what its buttons asked for
     **/
    RenderView::Asked frame(const v3d::ui::Immediate::Input& input) {
        written.clear();
        layer.begin(&canvas, input);
        const RenderView::Asked asked = view.draw(&layer, 1, glm::vec4(0.0F, 0.0F, 600.0F, 400.0F), &channel);
        layer.end();
        return asked;
    }

    /**
     * Click just inside the control whose label was written.
     * @return what the click asked for
     **/
    RenderView::Asked click(const std::string& label) {
        std::optional<glm::vec2> at;
        for (const v3d::ui::typeset::Written& each : written) {
            if (each.text == label) {
                at = each.pen + glm::vec2(2.0F, 2.0F);
            }
        }
        BOOST_REQUIRE(at);
        const glm::vec2 point = at.value_or(glm::vec2(0.0F));
        RenderView::Asked asked = frame(hover(point));
        v3d::ui::Immediate::Input input = hover(point);
        input.down = true;
        input.pressed = true;
        asked = std::max(asked, frame(input));
        input = hover(point);
        input.released = true;
        asked = std::max(asked, frame(input));
        return std::max(asked, frame(hover(point)));
    }

    bool wrote(const std::string& text) const {
        return std::ranges::any_of(written, [&text](const v3d::ui::typeset::Written& each) { return each.text == text; });
    }

    static v3d::ui::Immediate::Input hover(const glm::vec2& cursor) {
        v3d::ui::Immediate::Input made;
        made.cursor = cursor;
        return made;
    }

    boost::shared_ptr<Fake> delegate;
    std::vector<boost::shared_ptr<v3d::image::Image>> uploaded;
    RenderView::Channel channel = RenderView::Channel::Beauty;
    v3d::render::realtime::canvas::Canvas canvas;
    std::vector<v3d::ui::typeset::Written> written;
    v3d::ui::Immediate layer;
    RenderView view;
    v3d::editor::scene::Scene scene;
    v3d::type::camera::Profile from{ "front" };
};

BOOST_AUTO_TEST_SUITE(render_view_test)

/**
 * A picture is fitted to its room without changing its shape, and nothing fits in an empty
 * room or an empty picture.
 **/
BOOST_AUTO_TEST_CASE(a_picture_fits_its_room) {
    const glm::vec2 wide = RenderView::fit(glm::uvec2(640, 480), glm::vec2(400.0F, 400.0F));
    BOOST_CHECK_CLOSE(wide.x, 400.0F, 0.001F);
    BOOST_CHECK_CLOSE(wide.y, 300.0F, 0.001F);
    const glm::vec2 tall = RenderView::fit(glm::uvec2(100, 200), glm::vec2(400.0F, 100.0F));
    BOOST_CHECK_CLOSE(tall.x, 50.0F, 0.001F);
    BOOST_CHECK_CLOSE(tall.y, 100.0F, 0.001F);
    BOOST_CHECK(RenderView::fit(glm::uvec2(0, 480), glm::vec2(400.0F)) == glm::vec2(0.0F));
    BOOST_CHECK(RenderView::fit(glm::uvec2(640, 480), glm::vec2(0.0F, 400.0F)) == glm::vec2(0.0F));
}

/**
 * The first row says what the render is doing, as the renderer reports it.
 **/
BOOST_AUTO_TEST_CASE(the_state_follows_the_render) {
    View view;
    BOOST_CHECK(view.wrote("Nothing rendered yet"));

    view.delegate->now = { .state = Delegate::State::Rendering, .done = 0.25F, .pictures = 0 };
    view.view.tick(view.scene, view.from, { RenderView::Channel::Beauty }, FRAME);
    view.frame(View::hover(glm::vec2(690.0F, 490.0F)));
    BOOST_CHECK(view.wrote("Rendering - 25%"));

    view.delegate->now = { .state = Delegate::State::Failed, .done = 0.25F, .pictures = 0 };
    view.view.tick(view.scene, view.from, { RenderView::Channel::Beauty }, FRAME);
    BOOST_CHECK_EQUAL(view.view.state(), "The render failed - the log says why");
}

/**
 * A change to the scene reaches the render only while a render view is shown.
 **/
BOOST_AUTO_TEST_CASE(a_change_reaches_the_render_only_while_shown) {
    View view;
    view.view.render(view.scene, view.from);
    BOOST_CHECK_EQUAL(view.delegate->started, 1);

    v3d::editor::scene::RenderSettings wider = view.scene.renderSettings();
    wider.size.x = 800;
    view.scene.renderSettings(wider);
    view.view.tick(view.scene, view.from, {}, FRAME);
    BOOST_CHECK_EQUAL(view.delegate->edited, 0);
    view.view.tick(view.scene, view.from, { RenderView::Channel::Beauty }, FRAME);
    BOOST_CHECK_EQUAL(view.delegate->edited, 1);
    BOOST_CHECK_EQUAL(view.delegate->started, 1);
}

/**
 * Each button asks for what the Render menu's item of the same name does.
 **/
BOOST_AUTO_TEST_CASE(the_buttons_ask_to_render_and_stop) {
    View view;
    BOOST_CHECK(view.click("Render") == RenderView::Asked::Render);
    BOOST_CHECK(view.click("Stop") == RenderView::Asked::Stop);
}

/**
 * Each picture is shown as it comes, while the render still runs, and the first row says the
 * samples it holds.
 **/
BOOST_AUTO_TEST_CASE(each_picture_is_shown_as_it_comes) {
    View view;
    view.delegate->now = { .state = Delegate::State::Rendering, .done = 0.25F, .samples = 4, .total = 16, .pictures = 1 };
    view.view.tick(view.scene, view.from, { RenderView::Channel::Beauty }, FRAME);
    BOOST_CHECK_EQUAL(view.uploaded.size(), 1U);
    BOOST_CHECK_EQUAL(view.view.state(), "Rendering - 25% - 4 of 16 samples");

    // the same picture is not taken up twice, and the next is taken while the render runs
    view.view.tick(view.scene, view.from, { RenderView::Channel::Beauty }, FRAME);
    BOOST_CHECK_EQUAL(view.uploaded.size(), 1U);
    view.delegate->now = { .state = Delegate::State::Rendering, .done = 0.5F, .samples = 8, .total = 16, .pictures = 2 };
    view.view.tick(view.scene, view.from, { RenderView::Channel::Beauty }, FRAME);
    BOOST_CHECK_EQUAL(view.uploaded.size(), 2U);

    view.delegate->now = { .state = Delegate::State::Finished, .done = 1.0F, .samples = 16, .total = 16, .pictures = 3 };
    view.view.tick(view.scene, view.from, { RenderView::Channel::Beauty }, FRAME);
    BOOST_CHECK_EQUAL(view.view.state(), "Rendered - 16 samples");
}

/**
 * The view asks for pictures no more often than it is drawn, and no more often than its least.
 **/
BOOST_AUTO_TEST_CASE(the_view_sets_the_rate_of_pictures) {
    View view;
    view.view.tick(view.scene, view.from, { RenderView::Channel::Beauty }, FRAME);
    BOOST_CHECK_EQUAL(view.delegate->showing.interval, RenderView::LEAST_INTERVAL);
    BOOST_CHECK(!view.delegate->showing.denoised);
    BOOST_CHECK(!view.delegate->showing.guides);

    // a view drawn twice a second asks for a picture as often
    const int shows = view.delegate->shows;
    for (int tick = 0; tick < 100; tick++) {
        view.view.tick(view.scene, view.from, { RenderView::Channel::Beauty }, std::chrono::milliseconds(500));
    }
    BOOST_CHECK_GT(view.delegate->shows, shows);
    BOOST_CHECK_GT(view.delegate->showing.interval, 0.3);
    BOOST_CHECK_LE(view.delegate->showing.interval, 0.5);
}

/**
 * Choosing a channel shows that channel's picture, and asks the render for it.
 **/
BOOST_AUTO_TEST_CASE(choosing_a_channel_shows_its_picture) {
    View view;
    view.delegate->now = { .state = Delegate::State::Finished, .done = 1.0F, .samples = 1, .total = 1, .pictures = 1 };
    view.view.tick(view.scene, view.from, { view.channel }, FRAME);
    BOOST_REQUIRE_EQUAL(view.uploaded.size(), 1U);
    BOOST_CHECK(view.uploaded.back() == view.delegate->shown.at(0));

    view.click("Channel");
    view.click("Normal");
    BOOST_CHECK(view.channel == RenderView::Channel::Normal);
    view.view.tick(view.scene, view.from, { view.channel }, FRAME);
    BOOST_CHECK(view.delegate->showing.guides);
    BOOST_CHECK(!view.delegate->showing.denoised);
    BOOST_REQUIRE_EQUAL(view.uploaded.size(), 2U);
    BOOST_CHECK(view.uploaded.back() == view.delegate->shown.at(3));

    view.channel = RenderView::Channel::Denoised;
    view.view.tick(view.scene, view.from, { view.channel }, FRAME);
    BOOST_CHECK(view.delegate->showing.denoised);
    BOOST_CHECK(!view.delegate->showing.guides);
    BOOST_REQUIRE_EQUAL(view.uploaded.size(), 3U);
    BOOST_CHECK(view.uploaded.back() == view.delegate->shown.at(1));
}

/**
 * A layout saved with a render view's channel chosen reads back with it, and a state that names
 * none of the four channels is the beauty.
 **/
BOOST_AUTO_TEST_CASE(a_saved_workspace_keeps_the_channel) {
    const boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::editor::view::AreaLayout layout(logger);
    BOOST_REQUIRE(layout.load(boost::json::parse(R"({"root": {"editors": [{"kind": "render-view"}]}})").as_object()));
    BOOST_CHECK(RenderView::channelOf(layout.areas()[0].editor().state) == RenderView::Channel::Beauty);
    BOOST_REQUIRE(layout.set(layout.areas()[0].editor().id, RenderView::CHANNEL, boost::json::string(RenderView::name(RenderView::Channel::Denoised))));

    v3d::editor::view::AreaLayout again(logger);
    BOOST_REQUIRE(again.load(layout.save()));
    BOOST_CHECK(RenderView::channelOf(again.areas()[0].editor().state) == RenderView::Channel::Denoised);
    BOOST_CHECK(RenderView::channelOf(boost::json::parse(R"({"channel": "depth"})").as_object()) == RenderView::Channel::Beauty);
}

/**
 * A stop cancels the render.
 **/
BOOST_AUTO_TEST_CASE(a_stop_cancels_the_render) {
    View view;
    view.view.stop();
    BOOST_CHECK_EQUAL(view.delegate->cancelled, 1);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
