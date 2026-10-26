/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/log/Logger.h>
#include <api/type/camera/Profile.h>
#include <vertical3d/src/render/Batch.h>
#include <vertical3d/src/scene/Camera.h>
#include <vertical3d/src/scene/CreateItem.h>
#include <vertical3d/src/scene/CreatePoly.h>
#include <vertical3d/src/scene/Mesh.h>
#include <vertical3d/src/scene/RenderSettings.h>
#include <vertical3d/src/scene/Scene.h>

#include <api/animation/Curve.h>

#include <chrono>
#include <fstream>
#include <iterator>
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
        path_(boost::filesystem::temp_directory_path() / ("v3d_batch_" + name)) {
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
 * A cube in the middle of the default front view, over two frames.
 **/
struct Shots {
    Shots() :
        mesh(boost::make_shared<v3d::editor::Mesh>(v3d::editor::create_poly_cube())) {
        scene.add(mesh);
        scene.timeline({ .start = 1, .end = 2, .frame = 1 });
        settings.size = glm::uvec2(32, 24);
        settings.samples = 1;
    }

    v3d::editor::Scene scene;
    boost::shared_ptr<v3d::editor::Mesh> mesh;
    v3d::type::camera::Profile view{ "front" };
    v3d::editor::RenderSettings settings;
};

/**
 * moya.exe, where the build puts it.
 **/
boost::filesystem::path moya() {
    return boost::filesystem::path(V3D_MOYA);
}

/**
 * Wait for a batch to end, for as long as a few small frames could take on a slow machine.
 **/
v3d::editor::Batch::Status settle(const v3d::editor::Batch& batch) {
    constexpr std::chrono::milliseconds pause{ 20 };
    constexpr int tries = 3000;
    for (int attempt = 0; attempt < tries && batch.status().running; attempt++) {
        std::this_thread::sleep_for(pause);
    }
    return batch.status();
}

std::string read(const boost::filesystem::path& path) {
    std::ifstream file(path.string());
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

BOOST_FIXTURE_TEST_CASE(batch_renders_each_frame_test, Shots) {
    const Scratch scratch("frames");
    v3d::animation::Curve slide;
    slide.set(1.0F, 0.0F);
    slide.set(2.0F, 0.5F);
    BOOST_REQUIRE(scene.curve(mesh->id(), "translation.x", slide));
    v3d::editor::Batch batch(boost::make_shared<v3d::log::Logger>(), moya());

    // with no scene camera, every frame is seen through the view, as a RIB file and the
    // picture moya.exe writes from it
    const std::vector<v3d::editor::Batch::Job> jobs = batch.start(&scene, view, settings, scratch.path());
    BOOST_REQUIRE_EQUAL(jobs.size(), 2U);
    BOOST_CHECK_EQUAL(jobs[0].rib.filename().string(), "view.0001.rib");
    BOOST_CHECK_EQUAL(jobs[1].picture.filename().string(), "view.0002.png");
    BOOST_CHECK(boost::filesystem::exists(jobs[1].rib));

    // each frame's file is the scene posed at that frame, and the scene is posed back at its
    // own frame once they are written
    BOOST_CHECK_NE(read(jobs[0].rib), read(jobs[1].rib));
    BOOST_CHECK_EQUAL(mesh->translation().x, 0.0F);

    const v3d::editor::Batch::Status status = settle(batch);
    BOOST_CHECK(!status.running);
    BOOST_CHECK_EQUAL(status.done, 2U);
    BOOST_CHECK_EQUAL(status.failed, 0U);
    BOOST_CHECK(boost::filesystem::exists(jobs[0].picture));
    BOOST_CHECK(boost::filesystem::exists(jobs[1].picture));
}

BOOST_FIXTURE_TEST_CASE(batch_each_camera_test, Shots) {
    const Scratch scratch("cameras");
    const boost::shared_ptr<v3d::editor::Camera> first =
        v3d::editor::create_camera(view, v3d::editor::Camera::Projection::Orthographic);
    const boost::shared_ptr<v3d::editor::Camera> second =
        v3d::editor::create_camera(view, v3d::editor::Camera::Projection::Perspective);
    scene.add(first);
    scene.add(second);
    v3d::editor::Batch batch(boost::make_shared<v3d::log::Logger>(), moya());

    // every scene camera renders every frame, each into a directory of its own
    const std::vector<v3d::editor::Batch::Job> jobs = batch.start(&scene, view, settings, scratch.path());
    BOOST_REQUIRE_EQUAL(jobs.size(), 4U);
    BOOST_CHECK_EQUAL(jobs[0].rib.parent_path().filename().string(), "camera" + std::to_string(first->documentId()));
    BOOST_CHECK_EQUAL(jobs[1].rib.parent_path().filename().string(), "camera" + std::to_string(second->documentId()));
    BOOST_CHECK_EQUAL(settle(batch).failed, 0U);
}

BOOST_FIXTURE_TEST_CASE(batch_cancel_test, Shots) {
    const Scratch scratch("cancel");
    scene.timeline({ .start = 1, .end = 50, .frame = 1 });
    settings.size = glm::uvec2(2048, 2048);
    settings.samples = 8;
    settings.hider = v3d::editor::RenderSettings::Hider::RayTrace;
    v3d::editor::Batch batch(boost::make_shared<v3d::log::Logger>(), moya());

    // a stop ends the frame being rendered and starts no other
    BOOST_REQUIRE_EQUAL(batch.start(&scene, view, settings, scratch.path()).size(), 50U);
    batch.cancel();
    const v3d::editor::Batch::Status status = batch.status();
    BOOST_CHECK(!status.running);
    BOOST_CHECK_LT(status.done, 50U);
}

BOOST_AUTO_TEST_CASE(batch_missing_program_test) {
    const Scratch scratch("missing");
    v3d::editor::Scene scene;
    scene.add(boost::make_shared<v3d::editor::Mesh>(v3d::editor::create_poly_cube()));
    scene.timeline({ .start = 1, .end = 1, .frame = 1 });
    v3d::editor::Batch batch(boost::make_shared<v3d::log::Logger>(), scratch.path() / "no-such-program.exe");

    // a renderer that cannot be started fails each frame, and the batch still ends
    BOOST_REQUIRE_EQUAL(batch.start(&scene, v3d::type::camera::Profile("front"), v3d::editor::RenderSettings(),
        scratch.path()).size(), 1U);
    const v3d::editor::Batch::Status status = settle(batch);
    BOOST_CHECK(!status.running);
    BOOST_CHECK_EQUAL(status.failed, 1U);
}

};  // namespace
