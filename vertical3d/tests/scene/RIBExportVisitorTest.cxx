/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/offline/rib/Reader.h>
#include <vertical3d/src/scene/CreatePoly.h>
#include <vertical3d/src/scene/RIBExportVisitor.h>
#include <vertical3d/src/scene/Scene.h>

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>
#include <boost/filesystem/operations.hpp>

namespace {

/**
 * What the reader made of the file, which is a stronger assertion than what the text
 * looked like: the export is only worth anything if a renderer can read it back.
 **/
class ImportHandler final : public v3d::render::offline::rib::Handler {
 public:
    void format(unsigned int width, unsigned int height, float pixelAspect) override {
        (void)pixelAspect;
        width_ = width;
        height_ = height;
    }
    void projection(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters) override {
        (void)parameters;
        projection_ = name;
    }
    void screenWindow(float left, float right, float bottom, float top) override {
        (void)left;
        (void)bottom;
        right_ = right;
        top_ = top;
    }
    void clipping(float hither, float yon) override {
        hither_ = hither;
        yon_ = yon;
    }
    void transform(const glm::mat4x4 & matrix) override {  // NOLINT(build/include_what_you_use)
        camera_ = matrix;
    }
    void concatTransform(const glm::mat4x4 & matrix) override {
        placements_.push_back(matrix);
    }
    void worldBegin() override { worlds_++; }
    void worldEnd() override { worlds_++; }
    void attributeBegin() override { blocks_++; }
    void attribute(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters) override {
        (void)name;
        names_.push_back(parameters.string("name", ""));
    }
    void polygon(unsigned int vertices, const v3d::render::offline::rib::ParameterList & parameters) override {
        faces_++;
        corners_ += vertices;
        points_ = parameters.points("P");
    }

    std::vector<glm::mat4x4> placements_;
    std::vector<std::string> names_;
    std::vector<glm::vec3> points_;
    glm::mat4x4 camera_ = glm::mat4x4(1.0f);
    std::string projection_;
    float right_ = 0.0f;
    float top_ = 0.0f;
    float hither_ = 0.0f;
    float yon_ = 0.0f;
    unsigned int width_ = 0;
    unsigned int height_ = 0;
    unsigned int faces_ = 0;
    unsigned int corners_ = 0;
    unsigned int worlds_ = 0;
    unsigned int blocks_ = 0;
};

v3d::type::camera::Camera camera(bool orthographic) {
    v3d::type::camera::Profile profile("export");
    profile.orthographic(orthographic);
    profile.orthoZoom(2.0f);
    profile.pixelAspect(4.0f / 3.0f);
    profile.fov(50.0f);
    profile.clipping(0.5f, 250.0f);
    profile.eye(glm::vec3(0.0f, 0.0f, -8.0f));

    v3d::type::camera::Camera result(profile);
    result.profile().size(320, 240);
    result.createProjection();
    result.createView();
    return result;
}

std::string exportScene(const boost::shared_ptr<v3d::editor::Scene> & scene, bool orthographic) {
    std::ostringstream stream;
    v3d::editor::RIBExportVisitor visitor(&stream);
    const v3d::type::camera::Camera view = camera(orthographic);
    visitor.begin(view, 320, 240);
    scene->accept(&visitor);
    visitor.end();
    return stream.str();
}

bool reimport(const std::string & source, ImportHandler * handler) {
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());
    std::istringstream stream(source);
    return reader.read(stream, handler);
}

};  // namespace

/**
 * A cube out of the editor comes back as six faces of four corners, inside one world block.
 **/
BOOST_AUTO_TEST_CASE(ribexport_round_trip_test) {
    boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    scene->add(v3d::editor::create_poly_cube());

    const std::string exported = exportScene(scene, false);

    // beside the executable, so that a person can hand it to a renderer - which is the only
    // thing the export is for and the one thing a round trip cannot assert
    boost::filesystem::create_directory("data_out");
    std::ofstream file("data_out/export.rib");
    file << exported;
    file.close();

    ImportHandler handler;
    BOOST_REQUIRE(reimport(exported, &handler));

    BOOST_CHECK_EQUAL(handler.width_, 320u);
    BOOST_CHECK_EQUAL(handler.height_, 240u);
    BOOST_CHECK_EQUAL(handler.worlds_, 2u);
    BOOST_CHECK_EQUAL(handler.blocks_, 1u);
    BOOST_CHECK_EQUAL(handler.faces_, 6u);
    BOOST_CHECK_EQUAL(handler.corners_, 24u);
    BOOST_REQUIRE_EQUAL(handler.names_.size(), 1u);
    BOOST_CHECK_EQUAL(handler.names_[0], "mesh1");
}

/**
 * Two meshes are two attribute blocks, each with its own placement.
 **/
BOOST_AUTO_TEST_CASE(ribexport_placement_test) {
    boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    boost::shared_ptr<v3d::brep::BRep> first = v3d::editor::create_poly_cube();
    first->translation(glm::vec3(3.0f, 4.0f, 5.0f));
    scene->add(first);
    scene->add(v3d::editor::create_poly_plane());

    ImportHandler handler;
    BOOST_REQUIRE(reimport(exportScene(scene, false), &handler));

    BOOST_CHECK_EQUAL(handler.blocks_, 2u);
    BOOST_REQUIRE_EQUAL(handler.placements_.size(), 2u);

    // the placement carries the mesh's transform, and a transpose here would move the object
    // to where its axes point instead
    const glm::vec4 moved = handler.placements_[0] * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    BOOST_CHECK_CLOSE(moved.x, 3.0f, 0.01f);
    BOOST_CHECK_CLOSE(moved.y, 4.0f, 0.01f);
    BOOST_CHECK_CLOSE(moved.z, 5.0f, 0.01f);
}

/**
 * The camera the export names is the view's, and its aperture is written rather than left to
 * a reader to rederive from the frame aspect.
 **/
BOOST_AUTO_TEST_CASE(ribexport_camera_test) {
    boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();

    ImportHandler orthographic;
    BOOST_REQUIRE(reimport(exportScene(scene, true), &orthographic));
    BOOST_CHECK_EQUAL(orthographic.projection_, "orthographic");
    BOOST_CHECK_CLOSE(orthographic.top_, 2.0f, 0.01f);
    BOOST_CHECK_CLOSE(orthographic.right_, 2.0f * 4.0f / 3.0f, 0.01f);
    BOOST_CHECK_CLOSE(orthographic.hither_, 0.5f, 0.01f);
    BOOST_CHECK_CLOSE(orthographic.yon_, 250.0f, 0.01f);

    ImportHandler perspective;
    BOOST_REQUIRE(reimport(exportScene(scene, false), &perspective));
    BOOST_CHECK_EQUAL(perspective.projection_, "perspective");
    BOOST_CHECK_CLOSE(perspective.top_, 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(perspective.right_, 4.0f / 3.0f, 0.01f);

    // the world to camera transformation is what a view matrix is, so it puts an eye eight
    // units back at the origin of camera space
    const glm::vec4 eye = perspective.camera_ * glm::vec4(0.0f, 0.0f, -8.0f, 1.0f);
    BOOST_CHECK_SMALL(eye.x, 0.01f);
    BOOST_CHECK_SMALL(eye.y, 0.01f);
    BOOST_CHECK_SMALL(eye.z, 0.01f);
}

/**
 * An empty scene is still a valid file: a header, a camera and an empty world block.
 **/
BOOST_AUTO_TEST_CASE(ribexport_empty_scene_test) {
    boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();

    ImportHandler handler;
    BOOST_REQUIRE(reimport(exportScene(scene, false), &handler));
    BOOST_CHECK_EQUAL(handler.worlds_, 2u);
    BOOST_CHECK_EQUAL(handler.faces_, 0u);
}
