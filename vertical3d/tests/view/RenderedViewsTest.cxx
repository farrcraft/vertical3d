/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <api/brep/Primitives.h>
#include <api/image/Image.h>
#include <api/log/Logger.h>
#include <api/type/camera/Profile.h>
#include <vertical3d/src/render/Delegate.h>
#include <vertical3d/src/render/MoyaDelegate.h>
#include <vertical3d/src/scene/item/Mesh.h>
#include <vertical3d/src/scene/RenderSettings.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/view/RenderedViews.h>
#include <vertical3d/src/view/Shading.h>
#include <vertical3d/src/view/ViewPort.h>

#include <chrono>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <thread>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace {

using v3d::editor::render::Delegate;
using v3d::editor::view::RenderedViews;
using v3d::editor::view::Shading;
using v3d::editor::view::ViewPort;

/**
 * A renderer that renders nothing, and counts what it is asked to do.
 **/
class Fake final : public Delegate {
 public:
    void cancel() override {
        cancelled++;
        now.state = State::Stopped;
    }

    void resume() override {
        resumed++;
        now.state = State::Rendering;
    }

    Status status() const override {
        return now;
    }

    void show(const Showing& /*showing*/) override {
    }

    boost::shared_ptr<v3d::image::Image> picture(Channel /*channel*/) const override {
        return shown;
    }

    Status now;
    boost::shared_ptr<v3d::image::Image> shown = boost::make_shared<v3d::image::Image>(glm::uvec2(1, 1), static_cast<std::uint8_t>(24));
    int started = 0;
    int edited = 0;
    int cancelled = 0;
    int resumed = 0;
    glm::uvec2 size{ 0 };

 protected:
    void start(const v3d::editor::scene::Scene& /*scene*/, const v3d::type::camera::Profile& /*from*/,
        const v3d::editor::scene::RenderSettings& settings) override {
        started++;
        size = settings.size;
        now.state = State::Rendering;
    }

    bool edit(const v3d::editor::scene::Scene& /*scene*/, const v3d::type::camera::Profile& /*from*/,
        const v3d::editor::scene::RenderSettings& settings) override {
        edited++;
        size = settings.size;
        now.state = State::Rendering;
        return true;
    }
};

/**
 * Rendered views over fake renderers, each kept by the order it was made, and every picture
 * handed on, by view.
 **/
struct Views {
    Views() :
        rendered([this] {
            made.push_back(boost::make_shared<Fake>());
            return made.back();
        },
            [this](const std::string& view, const boost::shared_ptr<v3d::image::Image>& picture) { shown[view] = picture; }) {
    }

    /**
     * @return a view of a region, in a shading
     **/
    static boost::shared_ptr<ViewPort> view(const std::string& name, Shading shading, const glm::vec4& region = glm::vec4(0.0F, 0.0F, 64.0F, 48.0F)) {
        const v3d::type::camera::Profile profile("front");
        const boost::shared_ptr<ViewPort> made = boost::make_shared<ViewPort>(name, profile);
        made->resize(region);
        made->display().shading = shading;
        return made;
    }

    std::vector<boost::shared_ptr<Fake>> made;
    std::map<std::string, boost::shared_ptr<v3d::image::Image>> shown;
    RenderedViews rendered;
    v3d::editor::scene::Scene scene;
};

BOOST_AUTO_TEST_SUITE(rendered_views_test)

/**
 * Only a view in rendered shading has a session, and each such view has its own. Its pictures
 * are handed on under its name, and leaving rendered shading ends the session and its picture.
 **/
BOOST_AUTO_TEST_CASE(a_rendered_view_has_a_session_of_its_own) {
    Views views;
    const boost::shared_ptr<ViewPort> flat = Views::view("flat", Shading::Flat);
    const boost::shared_ptr<ViewPort> first = Views::view("first", Shading::Rendered);
    const boost::shared_ptr<ViewPort> second = Views::view("second", Shading::Rendered);
    views.rendered.tick(views.scene, { flat, first, second });
    BOOST_REQUIRE_EQUAL(views.made.size(), 2U);
    BOOST_CHECK_EQUAL(views.rendered.sessions(), 2U);
    BOOST_CHECK(!views.rendered.session("flat"));
    BOOST_CHECK_EQUAL(views.made[0]->started, 1);

    views.made[0]->now.pictures = 1;
    views.rendered.tick(views.scene, { flat, first, second });
    BOOST_CHECK(views.shown["first"] == views.made[0]->shown);
    BOOST_CHECK(!views.shown.contains("second"));

    first->display().shading = Shading::Smooth;
    views.rendered.tick(views.scene, { flat, first, second });
    BOOST_CHECK_EQUAL(views.rendered.sessions(), 1U);
    BOOST_CHECK_EQUAL(views.made[0]->cancelled, 1);
    BOOST_CHECK(views.shown.contains("first") && !views.shown["first"]);
    BOOST_CHECK_EQUAL(views.made.size(), 2U);
}

/**
 * A view that is not shown pauses its session, which samples on once it is shown again, and a
 * view that is gone ends its session.
 **/
BOOST_AUTO_TEST_CASE(a_view_not_shown_pauses_its_session) {
    Views views;
    boost::shared_ptr<ViewPort> view = Views::view("view", Shading::Rendered);
    views.rendered.tick(views.scene, { view });
    views.rendered.tick(views.scene, {});
    BOOST_REQUIRE_EQUAL(views.made.size(), 1U);
    BOOST_CHECK_EQUAL(views.made[0]->cancelled, 1);
    BOOST_CHECK_EQUAL(views.rendered.sessions(), 1U);
    // a session already paused is not stopped again
    views.rendered.tick(views.scene, {});
    BOOST_CHECK_EQUAL(views.made[0]->cancelled, 1);

    views.rendered.tick(views.scene, { view });
    BOOST_CHECK_EQUAL(views.made[0]->resumed, 1);
    BOOST_CHECK_EQUAL(views.made[0]->started, 1);
    BOOST_CHECK_EQUAL(views.made.size(), 1U);

    view.reset();
    views.rendered.tick(views.scene, {});
    BOOST_CHECK_EQUAL(views.rendered.sessions(), 0U);
}

/**
 * Orbiting a view is an edit of its own session, and of no other.
 **/
BOOST_AUTO_TEST_CASE(an_orbit_is_an_edit_of_the_views_session) {
    Views views;
    const boost::shared_ptr<ViewPort> moved = Views::view("moved", Shading::Rendered);
    const boost::shared_ptr<ViewPort> still = Views::view("still", Shading::Rendered);
    views.rendered.tick(views.scene, { moved, still });
    views.rendered.tick(views.scene, { moved, still });
    BOOST_REQUIRE_EQUAL(views.made.size(), 2U);
    BOOST_CHECK_EQUAL(views.made[0]->edited, 0);

    moved->camera()->profile().eye(glm::vec3(1.0F, 2.0F, -3.0F));
    views.rendered.tick(views.scene, { moved, still });
    BOOST_CHECK_EQUAL(views.made[0]->edited, 1);
    BOOST_CHECK_EQUAL(views.made[1]->edited, 0);
}

/**
 * A picture is the view's size divided by its pixel size, rounded up, and follows the view when
 * it is resized.
 **/
BOOST_AUTO_TEST_CASE(a_pictures_size_is_the_views_over_its_pixel_size) {
    Views views;
    const boost::shared_ptr<ViewPort> view = Views::view("view", Shading::Rendered, glm::vec4(10.0F, 20.0F, 101.0F, 50.0F));
    BOOST_CHECK(RenderedViews::resolution(*view) == glm::uvec2(101, 50));
    view->display().pixelSize = 2;
    BOOST_CHECK(RenderedViews::resolution(*view) == glm::uvec2(51, 25));
    views.rendered.tick(views.scene, { view });
    BOOST_REQUIRE_EQUAL(views.made.size(), 1U);
    BOOST_CHECK(views.made[0]->size == glm::uvec2(51, 25));

    view->resize(glm::vec4(0.0F, 0.0F, 40.0F, 30.0F));
    views.rendered.tick(views.scene, { view });
    BOOST_CHECK_EQUAL(views.made[0]->edited, 1);
    BOOST_CHECK(views.made[0]->size == glm::uvec2(20, 15));
    // an empty view still renders one pixel
    view->resize(glm::vec4(0.0F));
    BOOST_CHECK(RenderedViews::resolution(*view) == glm::uvec2(1, 1));
}

/**
 * A rendered view shows moya's picture from its own camera, and orbiting it makes a new picture
 * from the new camera.
 **/
BOOST_AUTO_TEST_CASE(a_rendered_view_shows_moyas_picture_from_its_camera) {
    std::map<std::string, boost::shared_ptr<v3d::image::Image>> shown;
    RenderedViews rendered([] { return boost::make_shared<v3d::editor::render::MoyaDelegate>(boost::make_shared<v3d::log::Logger>()); },
        [&shown](const std::string& view, const boost::shared_ptr<v3d::image::Image>& picture) { shown[view] = picture; });
    v3d::editor::scene::Scene scene;
    scene.add(boost::make_shared<v3d::editor::scene::item::Mesh>(v3d::brep::cube()));
    v3d::editor::scene::RenderSettings settings = scene.renderSettings();
    settings.samples = 1;
    scene.renderSettings(settings);
    const boost::shared_ptr<ViewPort> view = Views::view("view", Shading::Rendered, glm::vec4(0.0F, 0.0F, 32.0F, 24.0F));

    // tick until the render has finished and its last picture is handed on
    const auto settle = [&] {
        for (int attempt = 0; attempt < 3000; attempt++) {
            rendered.tick(scene, { view });
            const boost::shared_ptr<Delegate> session = rendered.session("view");
            if (session && session->status().state == Delegate::State::Finished) {
                rendered.tick(scene, { view });
                return shown["view"];
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return boost::shared_ptr<v3d::image::Image>();
    };
    const boost::shared_ptr<v3d::image::Image> first = settle();
    BOOST_REQUIRE(first);
    BOOST_CHECK_EQUAL(first->width(), 32U);
    BOOST_CHECK_EQUAL(first->height(), 24U);

    view->camera()->profile().eye(view->camera()->profile().eye() + glm::vec3(0.5F, 0.0F, 0.0F));
    const boost::shared_ptr<v3d::image::Image> orbited = settle();
    BOOST_REQUIRE(orbited);
    BOOST_CHECK(orbited != first);
    BOOST_CHECK(std::memcmp(first->data(), orbited->data(), static_cast<std::size_t>(32) * 24 * 3) != 0);
    rendered.release();
    BOOST_CHECK(!shown["view"]);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
