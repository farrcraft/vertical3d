/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/log/Logger.h>
#include <api/render/offline/rib/Handler.h>
#include <api/render/offline/rib/Parameters.h>
#include <api/render/offline/rib/Reader.h>
#include <api/render/offline/rib/Requests.h>
#include <api/render/offline/rib/Writer.h>
#include <api/type/camera/Camera.h>
#include <api/type/camera/Profile.h>
#include <vertical3d/src/scene/Camera.h>
#include <vertical3d/src/scene/CreatePoly.h>
#include <vertical3d/src/scene/Group.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Light.h>
#include <vertical3d/src/scene/Mesh.h>
#include <vertical3d/src/scene/RIBExportVisitor.h>
#include <vertical3d/src/scene/RenderSettings.h>
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
 * What the RIB reader parsed from the file. The tests assert on this rather than on the
 * text, because the export has to be readable by a renderer.
 **/
class ImportHandler final : public v3d::render::offline::rib::Handler {
 public:
    void format(const v3d::render::offline::rib::Format & request) override {
        width_ = request.width;
        height_ = request.height;
    }
    void projection(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters) override {
        (void)parameters;
        projection_ = name;
    }
    void screenWindow(const v3d::render::offline::rib::ScreenWindow & request) override {
        right_ = request.right;
        top_ = request.top;
    }
    void clipping(const v3d::render::offline::rib::Clipping & request) override {
        hither_ = request.hither;
        yon_ = request.yon;
    }
    void transform(const glm::mat4x4 & matrix) override {  // NOLINT(build/include_what_you_use)
        camera_ = matrix;
    }
    void concatTransform(const glm::mat4x4 & matrix) override {
        placements_.push_back(matrix);
    }
    void pixelSamples(const glm::uvec2 & samples) override { samples_ = samples; }
    void hider(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters) override {
        (void)parameters;
        hider_ = name;
    }
    void display(const v3d::render::offline::rib::Display & request,
        const v3d::render::offline::rib::ParameterList & parameters) override {
        (void)parameters;
        display_ = request;
    }
    void surface(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters) override {
        (void)parameters;
        surface_ = name;
    }
    void worldBegin() override { worlds_++; }
    void worldEnd() override { worlds_++; }
    void attributeBegin() override {
        blocks_++;
        shape_ += "(";
    }
    void attributeEnd() override { shape_ += ")"; }
    void attribute(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters) override {
        (void)name;
        names_.push_back(parameters.string("name"));
    }
    void lightSource(const v3d::render::offline::rib::LightSource & request,
        const v3d::render::offline::rib::ParameterList & parameters) override {
        lights_.push_back(request.shader);
        order_ += "L";
        froms_.push_back(parameters.points("from").empty() ? glm::vec3(-99.0F) : parameters.points("from").front());
        tos_.push_back(parameters.points("to").empty() ? glm::vec3(-99.0F) : parameters.points("to").front());
        cones_.push_back(parameters.number("coneangle", -1.0F));
        intensities_.push_back(parameters.number("intensity", -1.0F));
    }
    void polygon(unsigned int vertices, const v3d::render::offline::rib::ParameterList & parameters) override {
        order_ += "P";
        if (shape_.empty() || shape_.back() != 'P') {
            shape_ += "P";
        }
        faces_++;
        corners_ += vertices;
        points_ = parameters.points("P");
    }

    std::vector<std::string> lights_;
    std::vector<glm::vec3> froms_;
    std::vector<glm::vec3> tos_;
    std::vector<float> cones_;
    std::vector<float> intensities_;
    std::string order_;
    // each block as brackets, with a P inside for the polygons a block holds
    std::string shape_;
    std::vector<glm::mat4x4> placements_;
    std::vector<std::string> names_;
    std::vector<glm::vec3> points_;
    glm::mat4x4 camera_ = glm::mat4x4(1.0F);
    std::string projection_;
    glm::uvec2 samples_ = glm::uvec2(0U);
    std::string hider_;
    v3d::render::offline::rib::Display display_;
    std::string surface_;
    float right_ = 0.0F;
    float top_ = 0.0F;
    float hither_ = 0.0F;
    float yon_ = 0.0F;
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
    profile.orthoZoom(2.0F);
    profile.pixelAspect(4.0F / 3.0F);
    profile.fov(50.0F);
    profile.clipping(glm::vec2(0.5F, 250.0F));
    profile.eye(glm::vec3(0.0F, 0.0F, -8.0F));

    v3d::type::camera::Camera result(profile);
    result.profile().size(glm::uvec2(320, 240));
    result.createProjection();
    result.createView();
    return result;
}

std::string exportScene(const boost::shared_ptr<v3d::editor::Scene> & scene, const v3d::editor::RIBExportVisitor::Output & output) {
    std::ostringstream stream;
    v3d::render::offline::rib::Writer writer(&stream);
    v3d::editor::RIBExportVisitor visitor(&writer);
    visitor.begin(camera(false), output);
    scene->accept(&visitor);
    visitor.end();
    return stream.str();
}

std::string exportScene(const boost::shared_ptr<v3d::editor::Scene> & scene, bool orthographic) {
    std::ostringstream stream;
    v3d::render::offline::rib::Writer writer(&stream);
    v3d::editor::RIBExportVisitor visitor(&writer);
    const v3d::type::camera::Camera view = camera(orthographic);
    v3d::editor::RIBExportVisitor::Output output;
    output.settings.size = glm::uvec2(320, 240);
    visitor.begin(view, output);
    scene->accept(&visitor);
    visitor.end();
    return stream.str();
}

bool reimport(const std::string & source, ImportHandler * handler) {
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());
    std::istringstream stream(source);
    return reader.read(stream, handler);
}

/**
 * A cube out of the editor comes back as six faces of four corners, inside one world block.
 **/
BOOST_AUTO_TEST_CASE(ribexport_round_trip_test) {
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    scene->add(boost::make_shared<v3d::editor::Mesh>(v3d::editor::create_poly_cube()));

    const std::string exported = exportScene(scene, false);

    // beside the executable, so that a person can render it and check the image by eye. A
    // round trip through the reader cannot check what the render looks like
    boost::filesystem::create_directory("data_out");
    std::ofstream file("data_out/export.rib");
    file << exported;
    file.close();

    ImportHandler handler;
    BOOST_REQUIRE(reimport(exported, &handler));

    BOOST_CHECK_EQUAL(handler.width_, 320U);
    BOOST_CHECK_EQUAL(handler.height_, 240U);
    BOOST_CHECK_EQUAL(handler.worlds_, 2U);
    BOOST_CHECK_EQUAL(handler.blocks_, 1U);
    BOOST_CHECK_EQUAL(handler.faces_, 6U);
    BOOST_CHECK_EQUAL(handler.corners_, 24U);
    BOOST_REQUIRE_EQUAL(handler.names_.size(), 1U);
    BOOST_CHECK_EQUAL(handler.names_[0], "mesh1");
}

/**
 * Two meshes are two attribute blocks, each with its own placement.
 **/
BOOST_AUTO_TEST_CASE(ribexport_placement_test) {
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    const boost::shared_ptr<v3d::editor::Mesh> first = boost::make_shared<v3d::editor::Mesh>(v3d::editor::create_poly_cube());
    first->translation(glm::vec3(3.0F, 4.0F, 5.0F));
    scene->add(first);
    scene->add(boost::make_shared<v3d::editor::Mesh>(v3d::editor::create_poly_plane()));

    ImportHandler handler;
    BOOST_REQUIRE(reimport(exportScene(scene, false), &handler));

    BOOST_CHECK_EQUAL(handler.blocks_, 2U);
    BOOST_REQUIRE_EQUAL(handler.placements_.size(), 2U);

    // the placement carries the mesh's transform, and a transpose here would move the object
    // to where its axes point instead
    const glm::vec4 moved = handler.placements_[0] * glm::vec4(0.0F, 0.0F, 0.0F, 1.0F);
    BOOST_CHECK_CLOSE(moved.x, 3.0F, 0.01F);
    BOOST_CHECK_CLOSE(moved.y, 4.0F, 0.01F);
    BOOST_CHECK_CLOSE(moved.z, 5.0F, 0.01F);
}

/**
 * The camera the export names is the view's, and its aperture is written out rather than
 * left for a reader to derive from the frame aspect.
 **/
BOOST_AUTO_TEST_CASE(ribexport_camera_test) {
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();

    ImportHandler orthographic;
    BOOST_REQUIRE(reimport(exportScene(scene, true), &orthographic));
    BOOST_CHECK_EQUAL(orthographic.projection_, "orthographic");
    BOOST_CHECK_CLOSE(orthographic.top_, 2.0F, 0.01F);
    BOOST_CHECK_CLOSE(orthographic.right_, 2.0F * 4.0F / 3.0F, 0.01F);
    BOOST_CHECK_CLOSE(orthographic.hither_, 0.5F, 0.01F);
    BOOST_CHECK_CLOSE(orthographic.yon_, 250.0F, 0.01F);

    ImportHandler perspective;
    BOOST_REQUIRE(reimport(exportScene(scene, false), &perspective));
    BOOST_CHECK_EQUAL(perspective.projection_, "perspective");
    BOOST_CHECK_CLOSE(perspective.top_, 1.0F, 0.01F);
    BOOST_CHECK_CLOSE(perspective.right_, 4.0F / 3.0F, 0.01F);

    // a view matrix is the world to camera transformation, so it puts an eye eight units
    // back at the origin of camera space
    const glm::vec4 eye = perspective.camera_ * glm::vec4(0.0F, 0.0F, -8.0F, 1.0F);
    BOOST_CHECK_SMALL(eye.x, 0.01F);
    BOOST_CHECK_SMALL(eye.y, 0.01F);
    BOOST_CHECK_SMALL(eye.z, 0.01F);
}

/**
 * An empty scene is still a valid file: a header, a camera and an empty world block.
 **/
BOOST_AUTO_TEST_CASE(ribexport_empty_scene_test) {
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();

    ImportHandler handler;
    BOOST_REQUIRE(reimport(exportScene(scene, false), &handler));
    BOOST_CHECK_EQUAL(handler.worlds_, 2U);
    BOOST_CHECK_EQUAL(handler.faces_, 0U);
}

BOOST_AUTO_TEST_CASE(ribexport_lights_test) {
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    scene->add(boost::make_shared<v3d::editor::Mesh>(v3d::editor::create_poly_plane()));

    const boost::shared_ptr<v3d::editor::Light> point = boost::make_shared<v3d::editor::Light>();
    point->translation(glm::vec3(1.0F, 2.0F, 3.0F));
    point->emission({ .type = v3d::editor::Light::Type::Point, .intensity = 12.0F });
    scene->add(point);

    // a spot turned a quarter about y shines along +x
    const boost::shared_ptr<v3d::editor::Light> spot = boost::make_shared<v3d::editor::Light>();
    spot->translation(glm::vec3(0.0F, 5.0F, 0.0F));
    spot->rotation(glm::angleAxis(glm::radians(90.0F), glm::vec3(0.0F, 1.0F, 0.0F)));
    spot->emission({ .type = v3d::editor::Light::Type::Spot, .cone = 0.4F, .delta = 0.05F });
    scene->add(spot);
    scene->add(boost::make_shared<v3d::editor::Light>());
    scene->items().back()->rotation(glm::angleAxis(glm::radians(180.0F), glm::vec3(1.0F, 0.0F, 0.0F)));
    v3d::editor::as<v3d::editor::Light>(scene->items().back())->emission({ .type = v3d::editor::Light::Type::Distant });

    // a camera item is not written: the export renders from the camera it is given
    scene->add(boost::make_shared<v3d::editor::Camera>());

    ImportHandler handler;
    BOOST_REQUIRE(reimport(exportScene(scene, false), &handler));

    // every light is RI's standard shader for its type, written before every mesh, though
    // the plane comes first in the scene
    BOOST_REQUIRE_EQUAL(handler.lights_.size(), 3U);
    BOOST_CHECK_EQUAL(handler.lights_[0], "pointlight");
    BOOST_CHECK_EQUAL(handler.lights_[1], "spotlight");
    BOOST_CHECK_EQUAL(handler.lights_[2], "distantlight");
    BOOST_CHECK_EQUAL(handler.order_, "LLLP");

    // in world space: from where the light is, to one unit along its +z axis
    BOOST_CHECK_SMALL(glm::distance(handler.froms_[0], glm::vec3(1.0F, 2.0F, 3.0F)), 0.0001F);
    BOOST_CHECK_CLOSE(handler.intensities_[0], 12.0F, 0.001F);
    BOOST_CHECK_SMALL(glm::distance(handler.tos_[1], glm::vec3(1.0F, 5.0F, 0.0F)), 0.0001F);
    BOOST_CHECK_CLOSE(handler.cones_[1], 0.4F, 0.001F);
    BOOST_CHECK_SMALL(glm::distance(handler.tos_[2], glm::vec3(0.0F, 0.0F, -1.0F)), 0.0001F);
}

BOOST_AUTO_TEST_CASE(ribexport_picture_test) {
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    scene->add(boost::make_shared<v3d::editor::Mesh>(v3d::editor::create_poly_plane()));
    v3d::editor::RIBExportVisitor::Output output;
    output.settings.size = glm::uvec2(200, 100);
    output.settings.samples = 3;
    output.settings.hider = v3d::editor::RenderSettings::Hider::RayTrace;
    output.file = "C:/renders/frame.png";

    // the settings give the picture's size, its samples and its hider, and the display
    // names the file the picture is written to
    ImportHandler handler;
    BOOST_REQUIRE(reimport(exportScene(scene, output), &handler));
    BOOST_CHECK_EQUAL(handler.width_, 200U);
    BOOST_CHECK_EQUAL(handler.height_, 100U);
    BOOST_CHECK(handler.samples_ == glm::uvec2(3U, 3U));
    BOOST_CHECK_EQUAL(handler.hider_, "raytrace");
    BOOST_CHECK_EQUAL(handler.display_.name, "C:/renders/frame.png");
    BOOST_CHECK_EQUAL(handler.display_.type, "file");
    BOOST_CHECK_EQUAL(handler.surface_, "matte");

    // with no file the picture stays in the renderer
    output.file.clear();
    output.settings.hider = v3d::editor::RenderSettings::Hider::Hidden;
    ImportHandler kept;
    BOOST_REQUIRE(reimport(exportScene(scene, output), &kept));
    BOOST_CHECK_EQUAL(kept.display_.type, "framebuffer");
    BOOST_CHECK_EQUAL(kept.hider_, "hidden");
}

BOOST_AUTO_TEST_CASE(ribexport_default_light_test) {
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    scene->add(boost::make_shared<v3d::editor::Mesh>(v3d::editor::create_poly_plane()));

    // a scene with no light is lit from the camera, the way it looks, before every mesh
    ImportHandler handler;
    BOOST_REQUIRE(reimport(exportScene(scene, false), &handler));
    BOOST_REQUIRE_EQUAL(handler.lights_.size(), 1U);
    BOOST_CHECK_EQUAL(handler.lights_[0], "distantlight");
    BOOST_CHECK_EQUAL(handler.order_, "LP");
    BOOST_CHECK_SMALL(glm::distance(handler.froms_[0], glm::vec3(0.0F, 0.0F, -8.0F)), 0.0001F);
    BOOST_CHECK_SMALL(glm::distance(handler.tos_[0], glm::vec3(0.0F, 0.0F, -7.0F)), 0.0001F);

    // and a scene with a light of its own gets none of the camera's
    scene->add(boost::make_shared<v3d::editor::Light>());
    ImportHandler lit;
    BOOST_REQUIRE(reimport(exportScene(scene, false), &lit));
    BOOST_REQUIRE_EQUAL(lit.lights_.size(), 1U);
    BOOST_CHECK_EQUAL(lit.lights_[0], "pointlight");
}

BOOST_AUTO_TEST_CASE(ribexport_hierarchy_test) {
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    const boost::shared_ptr<v3d::editor::Group> outer = boost::make_shared<v3d::editor::Group>();
    const boost::shared_ptr<v3d::editor::Mesh> held = boost::make_shared<v3d::editor::Mesh>(v3d::editor::create_poly_plane());
    const boost::shared_ptr<v3d::editor::Mesh> inner = boost::make_shared<v3d::editor::Mesh>(v3d::editor::create_poly_plane());
    const boost::shared_ptr<v3d::editor::Group> empty = boost::make_shared<v3d::editor::Group>();
    const boost::shared_ptr<v3d::editor::Light> light = boost::make_shared<v3d::editor::Light>();
    outer->translation(glm::vec3(0.0F, 10.0F, 0.0F));
    held->translation(glm::vec3(2.0F, 0.0F, 0.0F));
    light->translation(glm::vec3(0.0F, 0.0F, 3.0F));
    scene->add(outer);
    scene->add(held);
    scene->add(inner);
    scene->add(empty);
    scene->add(light);
    scene->parent(held->id(), outer->id());
    scene->parent(inner->id(), held->id());
    scene->parent(empty->id(), outer->id());
    scene->parent(light->id(), outer->id());

    ImportHandler handler;
    BOOST_REQUIRE(reimport(exportScene(scene, false), &handler));

    // the group's block holds the mesh's, which holds the mesh beneath it. A group with no
    // mesh beneath it, and a light, write no block
    BOOST_CHECK_EQUAL(handler.shape_, "((P(P)))");
    BOOST_CHECK_EQUAL(handler.blocks_, 3U);
    const std::vector<std::string> names{ "group1", "mesh1", "mesh2" };
    BOOST_CHECK(handler.names_ == names);

    // each block holds its item's local matrix, which RI composes as the editor does
    BOOST_REQUIRE_EQUAL(handler.placements_.size(), 3U);
    BOOST_CHECK(handler.placements_[0] == outer->matrix());
    BOOST_CHECK(handler.placements_[1] == held->matrix());
    BOOST_CHECK(handler.placements_[2] == inner->matrix());

    // the light is written at world level, where the group puts it
    BOOST_REQUIRE_EQUAL(handler.froms_.size(), 1U);
    BOOST_CHECK_SMALL(glm::distance(handler.froms_[0], glm::vec3(0.0F, 10.0F, 3.0F)), 0.0001F);
    BOOST_CHECK_EQUAL(handler.order_, "LPP");
}

};  // namespace
