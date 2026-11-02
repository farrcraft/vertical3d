/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/image/Image.h>
#include <api/log/Logger.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/type/camera/Profile.h>
#include <api/ui/Immediate.h>
#include <api/ui/tests/Typeset.h>
#include <vertical3d/src/control/RenderView.h>
#include <vertical3d/src/render/Batch.h>
#include <vertical3d/src/render/Delegate.h>
#include <vertical3d/src/scene/RenderSettings.h>
#include <vertical3d/src/scene/Scene.h>

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

#include <boost/filesystem/path.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace {

using v3d::editor::control::RenderView;
using v3d::editor::render::Delegate;

/**
 * A renderer that renders nothing, and reports whatever state it is set to.
 **/
class Fake final : public Delegate {
 public:
    void cancel() override {
        cancelled++;
    }

    Status status() const override {
        return now;
    }

    boost::shared_ptr<v3d::image::Image> picture() const override {
        return shown;
    }

    Status now;
    boost::shared_ptr<v3d::image::Image> shown;
    int started = 0;
    int cancelled = 0;

 protected:
    void start(const v3d::editor::scene::Scene& /*scene*/, const v3d::type::camera::Profile& /*from*/,
        const v3d::editor::scene::RenderSettings& /*settings*/) override {
        started++;
    }
};

/**
 * A render view over a fake renderer, drawn into a 600 by 400 region at the canvas's corner. The
 * layer measures ten pixels a character and records what it writes.
 **/
struct View {
    View() :
        delegate(boost::make_shared<Fake>()),
        layer(v3d::ui::typeset::mono(&written)),
        view(delegate, boost::make_shared<v3d::editor::render::Batch>(boost::make_shared<v3d::log::Logger>(), boost::filesystem::path("moya.exe")),
            RenderView::Upload()) {
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
        const RenderView::Asked asked = view.draw(&layer, 1, glm::vec4(0.0F, 0.0F, 600.0F, 400.0F));
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
    view.view.tick(view.scene, view.from, true);
    view.frame(View::hover(glm::vec2(690.0F, 490.0F)));
    BOOST_CHECK(view.wrote("Rendering - 25%"));

    view.delegate->now = { .state = Delegate::State::Failed, .done = 0.25F, .pictures = 0 };
    view.view.tick(view.scene, view.from, true);
    BOOST_CHECK_EQUAL(view.view.state(), "The render failed - the log says why");
}

/**
 * The scene is rendered again when it changes only while a render view is shown.
 **/
BOOST_AUTO_TEST_CASE(a_change_renders_again_only_while_shown) {
    View view;
    view.view.render(view.scene, view.from);
    BOOST_CHECK_EQUAL(view.delegate->started, 1);

    v3d::editor::scene::RenderSettings wider = view.scene.renderSettings();
    wider.size.x = 800;
    view.scene.renderSettings(wider);
    view.view.tick(view.scene, view.from, false);
    BOOST_CHECK_EQUAL(view.delegate->started, 1);
    view.view.tick(view.scene, view.from, true);
    BOOST_CHECK_EQUAL(view.delegate->started, 2);
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
 * A stop cancels the render.
 **/
BOOST_AUTO_TEST_CASE(a_stop_cancels_the_render) {
    View view;
    view.view.stop();
    BOOST_CHECK_EQUAL(view.delegate->cancelled, 1);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
