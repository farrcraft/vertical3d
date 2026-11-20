/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <api/animation/Curve.h>
#include <api/brep/Primitives.h>
#include <api/image/Image.h>
#include <api/log/Logger.h>
#include <api/type/camera/Profile.h>
#include <api/image/Factory.h>
#include <vertical3d/src/render/Batch.h>
#include <vertical3d/src/render/Delegate.h>
#include <vertical3d/src/render/MoyaDelegate.h>
#include <vertical3d/src/scene/item/Mesh.h>
#include <vertical3d/src/scene/RenderSettings.h>
#include <vertical3d/src/scene/Scene.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace {

/**
 * A directory in the temporary directory, removed before and after the test.
 **/
class Scratch final {
 public:
    explicit Scratch(const std::string& name) :
        path_(boost::filesystem::temp_directory_path() / ("v3d_delegate_" + name)) {
        boost::filesystem::remove_all(path_);
    }

    ~Scratch() {
        boost::system::error_code error;
        boost::filesystem::remove_all(path_, error);
    }

    Scratch(const Scratch&) = delete;
    Scratch& operator=(const Scratch&) = delete;

    const boost::filesystem::path& path() const {
        return path_;
    }

 private:
    boost::filesystem::path path_;
};

/**
 * A cube in the middle of the default front view.
 **/
struct Cube {
    Cube() :
        mesh(boost::make_shared<v3d::editor::scene::item::Mesh>(v3d::brep::cube())) {
        scene.add(mesh);
        settings.size = glm::uvec2(64, 48);
        settings.samples = 1;
    }

    v3d::editor::scene::Scene scene;
    boost::shared_ptr<v3d::editor::scene::item::Mesh> mesh;
    v3d::type::camera::Profile view{ "front" };
    v3d::editor::scene::RenderSettings settings;
};

/**
 * Wait for a render to end, for as long as a small one could take on a slow machine.
 **/
v3d::editor::render::Delegate::Status settle(const v3d::editor::render::Delegate& delegate) {
    constexpr std::chrono::milliseconds pause{ 10 };
    constexpr int tries = 3000;
    for (int attempt = 0; attempt < tries; attempt++) {
        const v3d::editor::render::Delegate::Status status = delegate.status();
        if (status.state != v3d::editor::render::Delegate::State::Rendering) {
            return status;
        }
        std::this_thread::sleep_for(pause);
    }
    return delegate.status();
}

/**
 * The red of one pixel of an rgb picture.
 **/
unsigned char red(const v3d::image::Image& image, unsigned int column, unsigned int row) {
    return image[((row * image.width()) + column) * 3];
}

BOOST_FIXTURE_TEST_CASE(delegate_renders_test, Cube) {
    v3d::editor::render::MoyaDelegate delegate(boost::make_shared<v3d::log::Logger>());
    BOOST_CHECK(delegate.status().state == v3d::editor::render::Delegate::State::Idle);
    BOOST_CHECK(!delegate.picture());

    // a render runs off the caller's thread and leaves its picture in memory, at the size
    // the settings give. The cube faces the camera's own light, so its middle is lit
    delegate.render(scene, view, settings);
    const v3d::editor::render::Delegate::Status status = settle(delegate);
    BOOST_REQUIRE(status.state == v3d::editor::render::Delegate::State::Finished);
    BOOST_CHECK_EQUAL(status.done, 1.0F);
    BOOST_CHECK_EQUAL(status.pictures, 1U);
    const boost::shared_ptr<v3d::image::Image> picture = delegate.picture();
    BOOST_REQUIRE(picture);
    BOOST_CHECK_EQUAL(picture->width(), 64U);
    BOOST_CHECK_EQUAL(picture->height(), 48U);
    BOOST_CHECK_GT(red(*picture, 32, 24), 0);
    BOOST_CHECK_EQUAL(red(*picture, 1, 1), 0);
}

BOOST_FIXTURE_TEST_CASE(delegate_update_test, Cube) {
    v3d::editor::render::MoyaDelegate delegate(boost::make_shared<v3d::log::Logger>());

    // there is nothing to bring up to date before a first render
    BOOST_CHECK(!delegate.update(scene, view, settings));
    delegate.render(scene, view, settings);
    settle(delegate);

    // the same scene, camera and settings make the same picture, so nothing renders again
    BOOST_CHECK(!delegate.update(scene, view, settings));

    // an item that moves, a camera that moves and settings that change each render again
    mesh->translate(glm::vec3(0.25F, 0.0F, 0.0F));
    BOOST_CHECK(delegate.update(scene, view, settings));
    settle(delegate);
    BOOST_CHECK(!delegate.update(scene, view, settings));
    view.eye(glm::vec3(0.1F, 0.0F, -1.0F));
    BOOST_CHECK(delegate.update(scene, view, settings));
    settle(delegate);
    settings.samples = 2;
    BOOST_CHECK(delegate.update(scene, view, settings));
    BOOST_CHECK(settle(delegate).state == v3d::editor::render::Delegate::State::Finished);
    BOOST_CHECK_EQUAL(delegate.status().pictures, 4U);
}

BOOST_FIXTURE_TEST_CASE(delegate_update_frame_test, Cube) {
    v3d::editor::render::MoyaDelegate delegate(boost::make_shared<v3d::log::Logger>());
    v3d::animation::Curve curve;
    curve.set(1.0F, 0.0F);
    curve.set(2.0F, 0.25F);
    scene.curve(mesh->id(), "translation.x", curve);
    delegate.render(scene, view, settings);
    settle(delegate);

    // a frame where a curve moves an item renders again, though no item has changed
    const std::uint64_t revision = mesh->revision();
    scene.frame(2);
    BOOST_CHECK_EQUAL(mesh->revision(), revision);
    BOOST_CHECK(delegate.update(scene, view, settings));
    settle(delegate);
    BOOST_CHECK(!delegate.update(scene, view, settings));
}

BOOST_FIXTURE_TEST_CASE(delegate_cancel_test, Cube) {
    v3d::editor::render::MoyaDelegate delegate(boost::make_shared<v3d::log::Logger>());
    delegate.render(scene, view, settings);
    settle(delegate);
    const boost::shared_ptr<v3d::image::Image> finished = delegate.picture();

    // a render of several passes is stopped before its next pass, and the last finished
    // picture stays the one there is. Eight samples along each side are four passes
    settings.size = glm::uvec2(128, 128);
    settings.samples = 8;
    delegate.render(scene, view, settings);
    delegate.cancel();
    const v3d::editor::render::Delegate::Status status = delegate.status();
    BOOST_CHECK(status.state == v3d::editor::render::Delegate::State::Stopped);
    BOOST_CHECK_EQUAL(status.pictures, 1U);
    BOOST_CHECK(delegate.picture() == finished);
}

// a render reports the passes it has done over the passes it takes, and ends at 1
BOOST_FIXTURE_TEST_CASE(delegate_reports_its_progress_by_passes_test, Cube) {
    v3d::editor::render::MoyaDelegate delegate(boost::make_shared<v3d::log::Logger>());
    settings.size = glm::uvec2(128, 128);
    settings.samples = 8;
    delegate.render(scene, view, settings);
    std::vector<float> seen;
    constexpr int tries = 30000;
    for (int attempt = 0; attempt < tries && delegate.status().state == v3d::editor::render::Delegate::State::Rendering; attempt++) {
        const float done = delegate.status().done;
        if (seen.empty() || seen.back() != done) {
            seen.push_back(done);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    const v3d::editor::render::Delegate::Status status = delegate.status();
    BOOST_REQUIRE(status.state == v3d::editor::render::Delegate::State::Finished);
    BOOST_CHECK_EQUAL(status.done, 1.0F);
    // four passes report a quarter at a time, so whatever the polling saw is one of them
    bool partway = false;
    for (const float done : seen) {
        BOOST_CHECK_MESSAGE(done == 0.0F || done == 0.25F || done == 0.5F || done == 0.75F, done);
        partway = partway || (done > 0.0F && done < 1.0F);
    }
    BOOST_CHECK(partway);
}

// the picture the editor renders is the same, byte for byte, as moya.exe's render of the
// scene's RIB export, which a batch writes and runs
BOOST_FIXTURE_TEST_CASE(delegate_picture_is_moyas_render_of_the_export_test, Cube) {
    const boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    const Scratch scratch("export");
    scene.timeline({ .start = 1, .end = 1, .frame = 1 });
    settings.samples = 2;
    v3d::editor::render::MoyaDelegate delegate(logger);
    delegate.render(scene, view, settings);
    BOOST_REQUIRE(settle(delegate).state == v3d::editor::render::Delegate::State::Finished);
    const boost::shared_ptr<v3d::image::Image> rendered = delegate.picture();

    v3d::editor::render::Batch batch(logger, boost::filesystem::path(V3D_MOYA));
    const std::vector<v3d::editor::render::Batch::Job> jobs = batch.start(&scene, view, settings, scratch.path());
    BOOST_REQUIRE_EQUAL(jobs.size(), 1U);
    constexpr std::chrono::milliseconds pause{ 20 };
    for (int attempt = 0; attempt < 3000 && batch.status().running; attempt++) {
        std::this_thread::sleep_for(pause);
    }
    v3d::image::Factory factory(logger);
    const boost::shared_ptr<v3d::image::Image> written = factory.read(jobs[0].picture.string());
    BOOST_REQUIRE(written);
    BOOST_REQUIRE_EQUAL(written->width(), rendered->width());
    BOOST_REQUIRE_EQUAL(written->height(), rendered->height());
    BOOST_REQUIRE_EQUAL(written->bpp(), rendered->bpp());
    const std::size_t bytes = static_cast<std::size_t>(rendered->width()) * rendered->height() * (rendered->bpp() / 8U);
    BOOST_CHECK(std::memcmp(written->data(), rendered->data(), bytes) == 0);
}

};  // namespace
