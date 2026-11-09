/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <api/log/Logger.h>
#include <api/render/offline/rib/Reader.h>
#include <moya/libmoya/ri/SceneClient.h>
#include <moya/libmoya/scene/Declaration.h>
#include <moya/libmoya/scene/Description.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Kinds.h>
#include <moya/libmoya/scene/Samples.h>
#include <moya/libmoya/scene/Scene.h>
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
        for (const char * text : { "makes no light", "not drawn", "not made", "no ObjectBegin", "does not read", "left out", "subdivided by", "left open" }) {
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

// an object or an attribute block left open at WorldEnd ends there, so the next world's geometry
// names none of the objects the new world removed
BOOST_AUTO_TEST_CASE(moya_sceneclient_a_block_left_open_ends_with_its_world_test) {
    const Read object(std::string("WorldBegin\nObjectBegin 1\nWorldEnd\nWorldBegin\nSphere 1 -1 1 360\nWorldEnd\n"));
    BOOST_TEST((object.value("instance", "Sphere 1", "group") == scene::Value::handle({})));
    BOOST_TEST(object.heard("left open"));

    const Read block(std::string("WorldBegin\nSphere 1 -1 1 360\nAttributeBegin\nWorldEnd\nWorldBegin\nAttributeEnd\nSphere 2 -2 2 360\nWorldEnd\n"));
    // names start again in each world, so the second world's sphere is its first
    BOOST_TEST(block.scene().contains(block.value("instance", "Sphere 1", "material").handleArray().front()));
    BOOST_TEST(block.heard("left open"));
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

    BOOST_TEST((read.value("sphereLight", "light 1", "includeRoot") == scene::Value::integer(0)));
    BOOST_TEST((read.value("distantLight", "light 2", "includeRoot") == scene::Value::integer(0)));

    // a light on for every instance keeps includeRoot and names none
    const Read all(world("LightSource \"pointlight\" 1\nSphere 1 -1 1 360\nSphere 2 -2 2 360\n"));
    BOOST_TEST((all.value("sphereLight", "light 1", "includeRoot") == scene::Value::integer(1)));
    BOOST_TEST((all.value("sphereLight", "light 1", "instances") == scene::Value::handles({})));
}

// a light turned off before any geometry, or declared in an attribute block that closes before
// any, lights none of the instances rather than all of them
BOOST_AUTO_TEST_CASE(moya_sceneclient_a_light_off_for_every_instance_lights_none_test) {
    const Read off(world("LightSource \"pointlight\" 1\nIlluminate 1 0\nSphere 1 -1 1 360\n"));
    BOOST_TEST((off.value("sphereLight", "light 1", "includeRoot") == scene::Value::integer(0)));
    BOOST_TEST((off.value("sphereLight", "light 1", "instances") == scene::Value::handles({})));

    const Read closed(world("AttributeBegin\nLightSource \"pointlight\" 1\nAttributeEnd\nSphere 1 -1 1 360\n"));
    BOOST_TEST((closed.value("sphereLight", "light 1", "includeRoot") == scene::Value::integer(0)));
    BOOST_TEST((closed.value("sphereLight", "light 1", "instances") == scene::Value::handles({})));
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

/**
 * Each quadric's request becomes its kind's parameters, each from its own argument. The values
 * differ from each other and from each kind's defaults.
 *
 * Covers the base: 6787203e added the quadrics to the scene client. Checked by swapping zmin and
 * zmax in the cylinder's shape, and phimin and phimax in the torus's, which each fail this case.
 **/
BOOST_AUTO_TEST_CASE(moya_sceneclient_each_quadric_takes_each_of_its_arguments_test) {
    const Read read(world(
        "Sphere 2 -0.5 1.5 270\n"
        "Cone 3 1.5 180\n"
        "Cylinder 1.25 -0.25 0.75 90\n"
        "Hyperboloid 1 2 3 4 5 6 120\n"
        "Paraboloid 2.5 0.5 1.5 300\n"
        "Disk 0.5 2.5 45\n"
        "Torus 3 0.5 30 200 330\n"));
    BOOST_TEST((read.value("sphere", "Sphere 1", "radius") == scene::Value::number(2.0F)));
    BOOST_TEST((read.value("sphere", "Sphere 1", "zmin") == scene::Value::number(-0.5F)));
    BOOST_TEST((read.value("sphere", "Sphere 1", "zmax") == scene::Value::number(1.5F)));
    BOOST_TEST((read.value("sphere", "Sphere 1", "thetamax") == scene::Value::number(270.0F)));

    BOOST_TEST((read.value("cone", "Cone 1", "height") == scene::Value::number(3.0F)));
    BOOST_TEST((read.value("cone", "Cone 1", "radius") == scene::Value::number(1.5F)));
    BOOST_TEST((read.value("cone", "Cone 1", "thetamax") == scene::Value::number(180.0F)));

    BOOST_TEST((read.value("cylinder", "Cylinder 1", "radius") == scene::Value::number(1.25F)));
    BOOST_TEST((read.value("cylinder", "Cylinder 1", "zmin") == scene::Value::number(-0.25F)));
    BOOST_TEST((read.value("cylinder", "Cylinder 1", "zmax") == scene::Value::number(0.75F)));
    BOOST_TEST((read.value("cylinder", "Cylinder 1", "thetamax") == scene::Value::number(90.0F)));

    BOOST_TEST((read.value("hyperboloid", "Hyperboloid 1", "point1") == scene::Value::point(glm::vec3(1.0F, 2.0F, 3.0F))));
    BOOST_TEST((read.value("hyperboloid", "Hyperboloid 1", "point2") == scene::Value::point(glm::vec3(4.0F, 5.0F, 6.0F))));
    BOOST_TEST((read.value("hyperboloid", "Hyperboloid 1", "thetamax") == scene::Value::number(120.0F)));

    BOOST_TEST((read.value("paraboloid", "Paraboloid 1", "rmax") == scene::Value::number(2.5F)));
    BOOST_TEST((read.value("paraboloid", "Paraboloid 1", "zmin") == scene::Value::number(0.5F)));
    BOOST_TEST((read.value("paraboloid", "Paraboloid 1", "zmax") == scene::Value::number(1.5F)));
    BOOST_TEST((read.value("paraboloid", "Paraboloid 1", "thetamax") == scene::Value::number(300.0F)));

    BOOST_TEST((read.value("disk", "Disk 1", "height") == scene::Value::number(0.5F)));
    BOOST_TEST((read.value("disk", "Disk 1", "radius") == scene::Value::number(2.5F)));
    BOOST_TEST((read.value("disk", "Disk 1", "thetamax") == scene::Value::number(45.0F)));

    BOOST_TEST((read.value("torus", "Torus 1", "majorRadius") == scene::Value::number(3.0F)));
    BOOST_TEST((read.value("torus", "Torus 1", "minorRadius") == scene::Value::number(0.5F)));
    BOOST_TEST((read.value("torus", "Torus 1", "phimin") == scene::Value::number(30.0F)));
    BOOST_TEST((read.value("torus", "Torus 1", "phimax") == scene::Value::number(200.0F)));
    BOOST_TEST((read.value("torus", "Torus 1", "thetamax") == scene::Value::number(330.0F)));
}

/**
 * A curve takes its type, its wrap, its counts and the basis in force. Its "width" and
 * "constantwidth" become the kind's widths, and not primvars.
 *
 * Covers the base: 6787203e added curves to the scene client. Checked by reading the curve's
 * basis as "bezier" whatever Basis said, and by dropping its "constantwidth", which each fail this
 * case.
 **/
BOOST_AUTO_TEST_CASE(moya_sceneclient_a_curve_takes_its_basis_and_widths_test) {
    const Read read(world(
        "Basis \"catmull-rom\" 1 \"catmull-rom\" 1\n"
        "Curves \"cubic\" [4] \"periodic\" \"P\" [0 0 0 1 0 0 1 1 0 0 1 0] \"width\" [0.1 0.2 0.3 0.4]\n"
        "Curves \"linear\" [2] \"nonperiodic\" \"P\" [0 0 0 0 0 1] \"constantwidth\" [0.25]\n"));
    BOOST_TEST((read.value("curves", "Curves 1", "type") == scene::Value::string("cubic")));
    BOOST_TEST((read.value("curves", "Curves 1", "wrap") == scene::Value::string("periodic")));
    BOOST_TEST((read.value("curves", "Curves 1", "basis") == scene::Value::string("catmull-rom")));
    BOOST_TEST((read.value("curves", "Curves 1", "vertices") == scene::Value::integers({ 4 })));
    BOOST_TEST((read.value("curves", "Curves 1", "width") == scene::Value::floats({ 0.1F, 0.2F, 0.3F, 0.4F })));
    BOOST_TEST(read.scene().primvars(read.handle("curves", "Curves 1")).empty());

    BOOST_TEST((read.value("curves", "Curves 2", "type") == scene::Value::string("linear")));
    BOOST_TEST((read.value("curves", "Curves 2", "wrap") == scene::Value::string("nonperiodic")));
    BOOST_TEST((read.value("curves", "Curves 2", "P") == scene::Value::points({ glm::vec3(0.0F), glm::vec3(0.0F, 0.0F, 1.0F) })));
    BOOST_TEST((read.value("curves", "Curves 2", "constantWidth") == scene::Value::number(0.25F)));
    BOOST_TEST(read.scene().primvars(read.handle("curves", "Curves 2")).empty());
}

/**
 * Points take their positions, and their "width" or "constantwidth" as the kind's widths.
 *
 * Covers the base: 6787203e added points to the scene client. Checked by dropping the points'
 * "width", which fails this case.
 **/
BOOST_AUTO_TEST_CASE(moya_sceneclient_points_take_their_widths_test) {
    const Read read(world(
        "Points \"P\" [0 0 0 1 0 0] \"width\" [0.1 0.2]\n"
        "Points \"P\" [0 1 0] \"constantwidth\" [0.5]\n"));
    BOOST_TEST((read.value("points", "Points 1", "P") == scene::Value::points({ glm::vec3(0.0F), glm::vec3(1.0F, 0.0F, 0.0F) })));
    BOOST_TEST((read.value("points", "Points 1", "width") == scene::Value::floats({ 0.1F, 0.2F })));
    BOOST_TEST((read.value("points", "Points 2", "constantWidth") == scene::Value::number(0.5F)));
    BOOST_TEST(read.scene().primvars(read.handle("points", "Points 1")).empty());
}

/**
 * A quadric or a curve repeated in a motion block deforms: each parameter its kind declares for
 * motion holds a sample at each of the block's times.
 *
 * Covers the base: 6787203e added deforming primitives to the scene client. Checked by leaving
 * the cone and the curves out of the deforming handler's poses, which fails this case.
 **/
BOOST_AUTO_TEST_CASE(moya_sceneclient_a_quadric_or_a_curve_deforms_test) {
    const Read read(world(
        "MotionBegin [0 1]\nCone 1 1 360\nCone 2 1.5 360\nMotionEnd\n"
        "MotionBegin [0 0.5]\n"
        "Curves \"linear\" [2] \"nonperiodic\" \"P\" [0 0 0 1 0 0] \"width\" [0.1 0.1]\n"
        "Curves \"linear\" [2] \"nonperiodic\" \"P\" [0 1 0 1 1 0] \"width\" [0.2 0.3]\n"
        "MotionEnd\n"));
    const scene::Samples & height = read.scene().samples(read.handle("cone", "Cone 1"), "height");
    BOOST_TEST(height.times == (std::vector<float> { 0.0F, 1.0F }), boost::test_tools::per_element());
    BOOST_REQUIRE(height.values.size() == 2U);
    BOOST_TEST((height.values[1] == scene::Value::number(2.0F)));
    const scene::Samples & radius = read.scene().samples(read.handle("cone", "Cone 1"), "radius");
    BOOST_REQUIRE(radius.values.size() == 2U);
    BOOST_TEST((radius.values[1] == scene::Value::number(1.5F)));
    BOOST_TEST(read.scene().objects("cone").size() == 1U);

    const scene::Handle curves = read.handle("curves", "Curves 1");
    const scene::Samples & points = read.scene().samples(curves, "P");
    BOOST_TEST(points.times == (std::vector<float> { 0.0F, 0.5F }), boost::test_tools::per_element());
    BOOST_REQUIRE(points.values.size() == 2U);
    BOOST_TEST((points.values[1].triple(0) == glm::vec3(0.0F, 1.0F, 0.0F)));
    const scene::Samples & width = read.scene().samples(curves, "width");
    BOOST_REQUIRE(width.values.size() == 2U);
    BOOST_TEST((width.values[1] == scene::Value::floats({ 0.2F, 0.3F })));
    BOOST_TEST(read.scene().objects("curves").size() == 1U);
}

/**
 * A subdivision mesh keeps a loop or a bilinear scheme, and one moya does not subdivide by is
 * logged and becomes catmull-clark. A mesh with no tags has no creases, corners or holes, and
 * no boundary rule.
 *
 * Covers the base: 6787203e added subdivision meshes to the scene client. Checked by reading
 * every scheme as catmull-clark, and by starting a mesh's boundary at "edges", which each fail
 * this case.
 **/
BOOST_AUTO_TEST_CASE(moya_sceneclient_a_subdivision_mesh_keeps_its_scheme_test) {
    const Read read(world(
        "SubdivisionMesh \"loop\" [3] [0 1 2] [] [] [] [] \"P\" [0 0 0 1 0 0 1 1 0]\n"
        "SubdivisionMesh \"bilinear\" [3] [0 1 2] [] [] [] [] \"P\" [0 0 0 1 0 0 1 1 0]\n"
        "SubdivisionMesh \"sqrt3\" [3] [0 1 2] [] [] [] [] \"P\" [0 0 0 1 0 0 1 1 0]\n"));
    BOOST_TEST((read.value("subdivision", "SubdivisionMesh 1", "scheme") == scene::Value::string("loop")));
    BOOST_TEST((read.value("subdivision", "SubdivisionMesh 2", "scheme") == scene::Value::string("bilinear")));
    BOOST_TEST((read.value("subdivision", "SubdivisionMesh 3", "scheme") == scene::Value::string("catmull-clark")));
    BOOST_TEST(read.heard("subdivided by"));

    BOOST_TEST((read.value("subdivision", "SubdivisionMesh 1", "vertices") == scene::Value::integers({ 3 })));
    BOOST_TEST((read.value("subdivision", "SubdivisionMesh 1", "indices") == scene::Value::integers({ 0, 1, 2 })));
    BOOST_TEST((read.value("subdivision", "SubdivisionMesh 1", "creases") == scene::Value::integers({})));
    BOOST_TEST((read.value("subdivision", "SubdivisionMesh 1", "creaseLengths") == scene::Value::integers({})));
    BOOST_TEST((read.value("subdivision", "SubdivisionMesh 1", "corners") == scene::Value::integers({})));
    BOOST_TEST((read.value("subdivision", "SubdivisionMesh 1", "holes") == scene::Value::integers({})));
    BOOST_TEST((read.value("subdivision", "SubdivisionMesh 1", "boundary") == scene::Value::string("none")));
    BOOST_TEST(!read.heard("does not read"));
}

/**
 * A facevarying primvar keeps its storage class, with a value at each corner of each face,
 * whether the request declares it inline or a Declare does.
 **/
BOOST_AUTO_TEST_CASE(moya_sceneclient_a_facevarying_primvar_is_kept_test) {
    const Read read(world(
        "PointsPolygons [3 3] [0 1 2 0 2 3] \"P\" [0 0 0 1 0 0 1 1 0 0 1 0] \"facevarying float s\" [0 0.1 0.2 0.3 0.4 0.5]\n"
        "Declare \"t\" \"facevarying float\"\n"
        "Polygon \"P\" [0 0 0 1 0 0 1 1 0] \"t\" [0 0.5 1]\n"));
    const std::vector<scene::Declaration> & mesh = read.scene().primvars(read.handle("mesh", "PointsPolygons 1"));
    BOOST_REQUIRE(mesh.size() == 1U);
    BOOST_TEST(mesh.front().name == "s");
    BOOST_TEST((mesh.front().interpolation == scene::Interpolation::FaceVarying));
    BOOST_TEST((mesh.front().fallback == scene::Value::floats({ 0.0F, 0.1F, 0.2F, 0.3F, 0.4F, 0.5F })));

    const std::vector<scene::Declaration> & polygon = read.scene().primvars(read.handle("mesh", "Polygon 1"));
    BOOST_REQUIRE(polygon.size() == 1U);
    BOOST_TEST((polygon.front().interpolation == scene::Interpolation::FaceVarying));
    BOOST_TEST((polygon.front().fallback == scene::Value::floats({ 0.0F, 0.5F, 1.0F })));
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
