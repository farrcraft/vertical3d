/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <api/log/Logger.h>
#include <api/render/offline/rib/Reader.h>
#include <moya/libmoya/ri/SceneClient.h>
#include <moya/libmoya/scene/Description.h>
#include <moya/libmoya/scene/Kinds.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Schema.h>
#include <moya/libmoya/scene/Value.h>

#include <fstream>
#include <ios>
#include <istream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "Handling.h"

namespace {

namespace scene = v3d::moya::scene;

/**
 * A scene read from RIB through the client, applied at each frame's commit, with each frame's
 * description kept, as moya --scene prints them.
 **/
class Read final {
 public:
    explicit Read(const std::string & source) : scene_(scene::kinds()), logger_(boost::make_shared<v3d::log::Logger>()) {
        std::istringstream stream(source);
        read(&stream);
    }

    explicit Read(const boost::filesystem::path & file) : scene_(scene::kinds()), logger_(boost::make_shared<v3d::log::Logger>()) {
        std::ifstream stream(file.string(), std::ios::binary);
        read(&stream);
    }

    const scene::Scene & scene() const { return scene_; }
    const std::string & described() const { return described_; }
    bool heard(const std::string & text) const { return heard_.contains(text); }

    const scene::Value & value(const std::string & kind, const std::string & name, const std::string & parameter) const {
        return scene_.value(*scene_.find(kind, name), parameter);
    }

    scene::Handle handle(const std::string & kind, const std::string & name) const {
        return *scene_.find(kind, name);
    }

 private:
    void read(std::istream * stream) {
        const v3d::moya::handling::Heard heard(logger_->get());
        v3d::moya::ri::SceneClient client(scene_, logger_);
        client.frame([this]() {
            scene_.apply();
            described_ += describe(scene_);
        });
        v3d::render::offline::rib::Reader reader(logger_);
        BOOST_REQUIRE(reader.read(*stream, &client));
        for (const char * text : { "makes no light", "not drawn", "not made", "no ObjectBegin", "does not read", "left out" }) {
            if (heard.said(text)) {
                heard_ += std::string(text) + "\n";
            }
        }
    }

    scene::Scene scene_;
    boost::shared_ptr<v3d::log::Logger> logger_;
    std::string described_;
    std::string heard_;
};

/** A world around some requests, with RI's camera. **/
std::string world(const std::string & requests) {
    return "WorldBegin\n" + requests + "WorldEnd\n";
}

BOOST_AUTO_TEST_CASE(moya_sceneclient_a_primitive_is_a_prototype_and_an_instance_test) {
    const Read read(world(
        "AttributeBegin\n"
        "Attribute \"identifier\" \"name\" \"floor\"\n"
        "Color 0.5 0.25 1\n"
        "Surface \"plastic\" \"Ks\" [0.7]\n"
        "Translate 0 0 5\n"
        "Polygon \"P\" [0 0 0 1 0 0 1 1 0 0 1 0] \"Cs\" [1 0 0 0 1 0 0 0 1 1 1 1]\n"
        "AttributeEnd\n"));
    const scene::Handle mesh = read.handle("mesh", "floor");
    BOOST_TEST((read.value("mesh", "floor", "vertices") == scene::Value::integers({ 4 })));
    BOOST_TEST((read.value("mesh", "floor", "indices") == scene::Value::integers({ 0, 1, 2, 3 })));
    BOOST_TEST(read.scene().primvars(mesh).size() == 1U);
    BOOST_TEST((read.scene().primvars(mesh).front().interpolation == scene::Interpolation::Varying));

    BOOST_TEST((read.value("instance", "floor", "prototype") == scene::Value::handle(mesh)));
    BOOST_TEST((read.value("instance", "floor", "transform").matrixAt(0)[3] == glm::vec4(0.0F, 0.0F, 5.0F, 1.0F)));
    const scene::Handle material = read.value("instance", "floor", "material").handleArray().front();
    const scene::Handle surface = read.scene().value(material, "surface").handleArray().front();
    BOOST_TEST((read.scene().value(surface, "shader") == scene::Value::string("plastic")));
    BOOST_TEST((read.scene().value(surface, "language") == scene::Value::string("ri")));
    BOOST_TEST((read.scene().value(surface, "Ks") == scene::Value::floats({ 0.7F })));
    BOOST_TEST((read.scene().value(surface, "Cs") == scene::Value::colour(glm::vec3(0.5F, 0.25F, 1.0F))));
}

BOOST_AUTO_TEST_CASE(moya_sceneclient_instances_under_one_attribute_state_share_a_material_test) {
    const Read read(world(
        "Surface \"matte\"\n"
        "Sphere 1 -1 1 360\nSphere 2 -2 2 360\n"
        "Color 1 0 0\n"
        "Sphere 3 -3 3 360\n"));
    BOOST_TEST(read.scene().objects("material").size() == 2U);
    BOOST_TEST((read.value("instance", "Sphere 1", "material") == read.value("instance", "Sphere 2", "material")));
    BOOST_TEST(!(read.value("instance", "Sphere 2", "material") == read.value("instance", "Sphere 3", "material")));
    BOOST_TEST((read.value("sphere", "Sphere 3", "radius") == scene::Value::number(3.0F)));
}

BOOST_AUTO_TEST_CASE(moya_sceneclient_an_object_instanced_twice_is_one_prototype_and_two_instances_test) {
    const Read read(world(
        "ObjectBegin \"leaf\"\n"
        "Translate 0 1 0\n"
        "Disk 0 1 360\n"
        "ObjectEnd\n"
        "Translate 2 0 0\nObjectInstance \"leaf\"\n"
        "Translate 2 0 0\nObjectInstance \"leaf\"\n"
        "ObjectInstance \"twig\"\n"));
    const scene::Handle group = read.handle("group", "object leaf");
    BOOST_TEST(read.scene().objects("disk").size() == 1U);
    // the disk is placed in the object's own space, and takes no material until the object is
    // placed
    BOOST_TEST((read.value("instance", "Disk 1", "group") == scene::Value::handle(group)));
    BOOST_TEST((read.value("instance", "Disk 1", "transform").matrixAt(0)[3] == glm::vec4(0.0F, 1.0F, 0.0F, 1.0F)));
    BOOST_TEST(!read.scene().contains(read.value("instance", "Disk 1", "material").handleArray().front()));
    BOOST_TEST((read.value("instance", "object leaf 1", "prototype") == scene::Value::handle(group)));
    BOOST_TEST((read.value("instance", "object leaf 2", "transform").matrixAt(0)[3] == glm::vec4(4.0F, 0.0F, 0.0F, 1.0F)));
    BOOST_TEST(read.heard("no ObjectBegin"));
}

BOOST_AUTO_TEST_CASE(moya_sceneclient_a_light_names_the_instances_it_lit_test) {
    const Read read(world(
        "Sphere 1 -1 1 360\n"
        "LightSource \"pointlight\" 1 \"from\" [0 2 0] \"intensity\" [3]\n"
        "LightSource \"distantlight\" 2\n"
        "Sphere 2 -2 2 360\n"
        "Illuminate 1 0\n"
        "Sphere 3 -3 3 360\n"
        "LightSource \"ambientlight\" 3\nIlluminate 3 0\n"));
    BOOST_TEST((read.value("sphereLight", "light 1", "instances") == scene::Value::handles({ read.handle("instance", "Sphere 2") })));
    BOOST_TEST((read.value("distantLight", "light 2", "instances") ==
        scene::Value::handles({ read.handle("instance", "Sphere 2"), read.handle("instance", "Sphere 3") })));
    BOOST_TEST((read.value("sphereLight", "light 1", "radius") == scene::Value::number(0.0F)));
    BOOST_TEST((read.value("sphereLight", "light 1", "intensity") == scene::Value::number(3.0F)));
    BOOST_TEST((read.value("sphereLight", "light 1", "transform").matrixAt(0)[3] == glm::vec4(0.0F, 2.0F, 0.0F, 1.0F)));
    BOOST_TEST(read.heard("makes no light"));
    BOOST_TEST(!read.heard("not made"));

    // a light on for every instance names none
    const Read all(world("LightSource \"pointlight\" 1\nSphere 1 -1 1 360\nSphere 2 -2 2 360\n"));
    BOOST_TEST((all.value("sphereLight", "light 1", "instances") == scene::Value::handles({})));
}

BOOST_AUTO_TEST_CASE(moya_sceneclient_a_light_points_from_from_to_to_test) {
    const Read read(world(
        "LightSource \"spotlight\" 1 \"from\" [0 0 0] \"to\" [1 0 0] \"coneangle\" [0.5] \"conedeltaangle\" [0.1]\n"));
    const glm::mat4 placed = read.value("sphereLight", "light 1", "transform").matrixAt(0);
    // UsdLux's lights emit down their -z
    const glm::vec3 emits = -glm::vec3(placed[2]);
    BOOST_TEST(emits.x == 1.0F, boost::test_tools::tolerance(1.0e-6F));
    BOOST_TEST(emits.y == 0.0F, boost::test_tools::tolerance(1.0e-6F));
    BOOST_TEST(read.value("sphereLight", "light 1", "coneAngle").floatArray().front() == 28.6479F, boost::test_tools::tolerance(1.0e-4F));
    BOOST_TEST(read.value("sphereLight", "light 1", "coneSoftness").floatArray().front() == 0.2F, boost::test_tools::tolerance(1.0e-5F));
}

BOOST_AUTO_TEST_CASE(moya_sceneclient_an_area_light_makes_a_mesh_light_of_its_geometry_test) {
    const Read read(world(
        "AttributeBegin\n"
        "AreaLightSource \"arealight\" 4 \"intensity\" [2]\n"
        "Polygon \"P\" [0 0 0 1 0 0 1 1 0]\n"
        "AttributeEnd\n"
        "Polygon \"P\" [0 0 1 1 0 1 1 1 1]\n"));
    BOOST_TEST(read.scene().objects("meshLight").size() == 1U);
    BOOST_TEST((read.value("meshLight", "light 4", "geometry") == scene::Value::handle(read.handle("mesh", "Polygon 1"))));
    BOOST_TEST((read.value("meshLight", "light 4", "intensity") == scene::Value::number(2.0F)));
    BOOST_TEST(read.scene().objects("instance").size() == 2U);
}

BOOST_AUTO_TEST_CASE(moya_sceneclient_a_primitive_repeated_in_a_motion_block_deforms_test) {
    const Read read(world(
        "MotionBegin [0 0.5]\n"
        "Polygon \"P\" [0 0 0 1 0 0 1 1 0]\n"
        "Polygon \"P\" [0 0 1 1 0 1 1 1 1]\n"
        "MotionEnd\n"
        "MotionBegin [0 1]\nSphere 1 -1 1 360\nSphere 2 -1 1 360\nMotionEnd\n"));
    const scene::Samples & points = read.scene().samples(read.handle("mesh", "Polygon 1"), "P");
    BOOST_TEST(points.times == (std::vector<float> { 0.0F, 0.5F }), boost::test_tools::per_element());
    BOOST_TEST((points.values[1].triple(2) == glm::vec3(1.0F, 1.0F, 1.0F)));
    const scene::Samples & radius = read.scene().samples(read.handle("sphere", "Sphere 1"), "radius");
    BOOST_TEST(radius.values.size() == 2U);
    BOOST_TEST(read.scene().objects("mesh").size() == 1U);
}

BOOST_AUTO_TEST_CASE(moya_sceneclient_a_subdivision_meshs_tags_become_its_parameters_test) {
    const Read read(world(
        "SubdivisionMesh \"catmull-clark\" [4] [0 1 2 3] [\"crease\" \"corner\" \"hole\" \"interpolateboundary\" \"facevaryingpropagatecorners\"]"
        " [2 1 1 1 1 0 0 0 0 0] [0 1 2 0] [2.5 4] \"P\" [0 0 0 1 0 0 1 1 0 0 1 0]\n"));
    BOOST_TEST((read.value("subdivision", "SubdivisionMesh 1", "creases") == scene::Value::integers({ 0, 1 })));
    BOOST_TEST((read.value("subdivision", "SubdivisionMesh 1", "creaseLengths") == scene::Value::integers({ 2 })));
    BOOST_TEST((read.value("subdivision", "SubdivisionMesh 1", "creaseSharpness") == scene::Value::floats({ 2.5F })));
    BOOST_TEST((read.value("subdivision", "SubdivisionMesh 1", "corners") == scene::Value::integers({ 2 })));
    BOOST_TEST((read.value("subdivision", "SubdivisionMesh 1", "cornerSharpness") == scene::Value::floats({ 4.0F })));
    BOOST_TEST((read.value("subdivision", "SubdivisionMesh 1", "holes") == scene::Value::integers({ 0 })));
    BOOST_TEST((read.value("subdivision", "SubdivisionMesh 1", "boundary") == scene::Value::string("edges-and-corners")));
    BOOST_TEST(read.heard("does not read"));
}

BOOST_AUTO_TEST_CASE(moya_sceneclient_a_new_world_replaces_the_one_before_test) {
    const Read read(
        "FrameBegin 1\n" + world("Sphere 1 -1 1 360\nLightSource \"pointlight\" 1\n") + "FrameEnd\n"
        "FrameBegin 2\n" + world("Disk 0 1 360\n") + "FrameEnd\n");
    BOOST_TEST(read.scene().objects("sphere").empty());
    BOOST_TEST(read.scene().objects("sphereLight").empty());
    BOOST_TEST(read.scene().objects("disk").size() == 1U);
    BOOST_TEST(read.scene().objects("instance").size() == 1U);
    BOOST_TEST(read.scene().objects("camera").size() == 1U);
}

BOOST_AUTO_TEST_CASE(moya_sceneclient_a_primitive_outside_the_world_is_not_drawn_test) {
    const Read read(std::string("Sphere 1 -1 1 360\nWorldBegin\nWorldEnd\n"));
    BOOST_TEST(read.scene().objects("sphere").empty());
    BOOST_TEST(read.heard("not drawn"));
}

/**
 * Every RIB scene moya's suite renders reads through the client into the scene its committed
 * description holds. moya --scene writes the descriptions.
 **/
BOOST_AUTO_TEST_CASE(moya_sceneclient_every_test_scene_reads_as_committed_test) {
    unsigned int scenes = 0;
    for (const boost::filesystem::directory_entry & entry : boost::filesystem::directory_iterator("data")) {
        if (entry.path().extension() != ".rib") {
            continue;
        }
        const boost::filesystem::path committed = boost::filesystem::path("data") / "scenes" / (entry.path().stem().string() + ".txt");
        std::ifstream stream(committed.string(), std::ios::binary);
        BOOST_REQUIRE_MESSAGE(stream.is_open(), committed.string() + " is missing");
        const std::string expected((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
        const Read read(entry.path());
        BOOST_TEST(read.described() == expected, entry.path().filename().string());
        scenes++;
    }
    BOOST_TEST(scenes == 22U);
}

};  // namespace
