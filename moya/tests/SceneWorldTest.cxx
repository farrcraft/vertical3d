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

#include <algorithm>
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

#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "Heard.h"

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

// an instance is double-sided under RI's default of two sides, and not under Sides 1, which lasts
// to the end of its attribute block. A count of sides RI does not name is reported and changes
// nothing
BOOST_AUTO_TEST_CASE(moya_sceneclient_sides_decides_whether_an_instance_is_double_sided_test) {
    const Read read(world(
        "Attribute \"identifier\" \"name\" \"both\"\n"
        "Polygon \"P\" [0 0 0 1 0 0 1 1 0]\n"
        "AttributeBegin\n"
        "Sides 1\n"
        "Attribute \"identifier\" \"name\" \"front\"\n"
        "Polygon \"P\" [0 0 0 1 0 0 1 1 0]\n"
        "Sides 3\n"
        "Attribute \"identifier\" \"name\" \"still front\"\n"
        "Polygon \"P\" [0 0 0 1 0 0 1 1 0]\n"
        "AttributeEnd\n"
        "Attribute \"identifier\" \"name\" \"again\"\n"
        "Polygon \"P\" [0 0 0 1 0 0 1 1 0]\n"));
    BOOST_TEST((read.value("instance", "both", "doubleSided") == scene::Value::integer(1)));
    BOOST_TEST((read.value("instance", "front", "doubleSided") == scene::Value::integer(0)));
    BOOST_TEST((read.value("instance", "still front", "doubleSided") == scene::Value::integer(0)));
    BOOST_TEST((read.value("instance", "again", "doubleSided") == scene::Value::integer(1)));
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

// RI's float[2] makes each of a primvar's elements two floats
BOOST_AUTO_TEST_CASE(moya_sceneclient_a_float_array_primvar_is_of_its_width_test) {
    const Read read(world(
        "AttributeBegin\n"
        "Attribute \"identifier\" \"name\" \"floor\"\n"
        "Polygon \"P\" [0 0 0 1 0 0 1 1 0 0 1 0] \"facevarying float[2] st\" [0 0 1 0 1 1 0 1] \"constant float[3] weights\" [1 2 3]\n"
        "AttributeEnd\n"));
    const scene::Handle mesh = read.handle("mesh", "floor");
    const std::vector<scene::Declaration> & primvars = read.scene().primvars(mesh);
    BOOST_TEST_REQUIRE(primvars.size() == 2U);
    for (const scene::Declaration & declared : primvars) {
        BOOST_TEST(declared.width == (declared.name == "st" ? 2U : 3U));
    }
    BOOST_TEST(read.scene().value(mesh, "st").size() == 8U);
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
        "LightSource \"glowlight\" 3\nIlluminate 3 0\n"));
    BOOST_TEST((read.value("sphereLight", "light 1", "instances") == scene::Value::handles({ read.handle("instance", "Sphere 2") })));
    BOOST_TEST((read.value("distantLight", "light 2", "instances") ==
        scene::Value::handles({ read.handle("instance", "Sphere 2"), read.handle("instance", "Sphere 3") })));
    BOOST_TEST((read.value("sphereLight", "light 1", "radius") == scene::Value::number(0.0F)));
    // a point is normalised at four times RI's intensity, so its radiant intensity is RI's
    BOOST_TEST((read.value("sphereLight", "light 1", "intensity") == scene::Value::number(12.0F)));
    BOOST_TEST((read.value("sphereLight", "light 1", "normalize") == scene::Value::integer(1)));
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

// each of RenderMan's analytic lights makes UsdLux's light of its shape, with RenderMan's
// parameters under UsdLux's names
BOOST_AUTO_TEST_CASE(moya_sceneclient_renderman_lights_make_usdlux_lights_test) {
    const Read read(world(
        "Light \"PxrRectLight\" \"rect\" \"float intensity\" [3] \"float exposure\" [2] \"color lightColor\" [1 0.5 0.25]"
        " \"int enableTemperature\" [1] \"float temperature\" [3200] \"int areaNormalize\" [1] \"string lightColorMap\" [\"sky.exr\"]"
        " \"float coneAngle\" [40] \"float coneSoftness\" [0.5] \"string iesProfile\" [\"lamp.ies\"] \"float iesProfileScale\" [2]"
        " \"int iesProfileNormalize\" [1]\n"
        "Light \"PxrDiskLight\" \"disk\"\n"
        "Light \"PxrSphereLight\" \"sphere\"\n"
        "Light \"PxrCylinderLight\" \"cylinder\"\n"
        "Light \"PxrDistantLight\" \"sun\" \"float angleExtent\" [2]\n"
        "Light \"PxrDistantLight\" \"plain\"\n"
        "Light \"PxrDomeLight\" \"dome\" \"string lightColorMap\" [\"sky.exr\"] \"int areaNormalize\" [1]\n"
        "Light \"PxrPortalLight\" \"portal\"\n"
        "Sphere 1 -1 1 360\n"));
    BOOST_TEST((read.value("rectLight", "light rect", "intensity") == scene::Value::number(3.0F)));
    BOOST_TEST((read.value("rectLight", "light rect", "exposure") == scene::Value::number(2.0F)));
    BOOST_TEST((read.value("rectLight", "light rect", "colour") == scene::Value::colour(glm::vec3(1.0F, 0.5F, 0.25F))));
    BOOST_TEST((read.value("rectLight", "light rect", "enableColorTemperature") == scene::Value::integer(1)));
    BOOST_TEST((read.value("rectLight", "light rect", "colorTemperature") == scene::Value::number(3200.0F)));
    BOOST_TEST((read.value("rectLight", "light rect", "normalize") == scene::Value::integer(1)));
    BOOST_TEST((read.value("rectLight", "light rect", "texture") == scene::Value::string("sky.exr")));
    BOOST_TEST((read.value("rectLight", "light rect", "coneAngle") == scene::Value::number(40.0F)));
    BOOST_TEST((read.value("rectLight", "light rect", "coneSoftness") == scene::Value::number(0.5F)));
    BOOST_TEST((read.value("rectLight", "light rect", "iesFile") == scene::Value::string("lamp.ies")));
    BOOST_TEST((read.value("rectLight", "light rect", "iesAngleScale") == scene::Value::number(2.0F)));
    BOOST_TEST((read.value("rectLight", "light rect", "iesNormalize") == scene::Value::integer(1)));
    BOOST_TEST(read.scene().objects("diskLight").size() == 1U);
    BOOST_TEST(read.scene().objects("sphereLight").size() == 1U);
    BOOST_TEST(read.scene().objects("cylinderLight").size() == 1U);
    // RenderMan's sun is far brighter than UsdLux's default
    BOOST_TEST((read.value("distantLight", "light sun", "angle") == scene::Value::number(2.0F)));
    BOOST_TEST((read.value("distantLight", "light plain", "intensity") == scene::Value::number(50000.0F)));
    BOOST_TEST((read.value("distantLight", "light plain", "angle") == scene::Value::number(0.53F)));
    BOOST_TEST((read.value("domeLight", "light dome", "texture") == scene::Value::string("sky.exr")));
    BOOST_TEST((read.value("domeLight", "light dome", "normalize") == scene::Value::integer(0)));
    BOOST_TEST(read.heard("makes no light"));
}

// a RenderMan light faces its +z, as UsdLux's faces its -z, and its dome's pole is its +z, as
// UsdLux's is its +y
BOOST_AUTO_TEST_CASE(moya_sceneclient_a_renderman_light_is_turned_onto_usdlux_axes_test) {
    const Read read(world(
        "Translate 1 2 3\n"
        "Light \"PxrRectLight\" \"rect\"\n"
        "Light \"PxrDomeLight\" \"dome\"\n"));
    const glm::mat4 rect = read.value("rectLight", "light rect", "transform").matrixAt(0);
    BOOST_TEST((rect * glm::vec4(0.0F, 0.0F, -1.0F, 0.0F) == glm::vec4(0.0F, 0.0F, 1.0F, 0.0F)));
    // the rect's texture runs from RenderMan's -x and -y, which UsdLux lays at its +x and +y
    BOOST_TEST((rect * glm::vec4(1.0F, 1.0F, 0.0F, 0.0F) == glm::vec4(-1.0F, -1.0F, 0.0F, 0.0F)));
    BOOST_TEST((rect[3] == glm::vec4(1.0F, 2.0F, 3.0F, 1.0F)));
    const glm::mat4 dome = read.value("domeLight", "light dome", "transform").matrixAt(0);
    BOOST_TEST((dome * glm::vec4(0.0F, 1.0F, 0.0F, 0.0F) == glm::vec4(0.0F, 0.0F, 1.0F, 0.0F)));
    BOOST_TEST((dome * glm::vec4(0.0F, 0.0F, 1.0F, 0.0F) == glm::vec4(-1.0F, 0.0F, 0.0F, 0.0F)));
}

// a RenderMan light is on as a LightSource is: Illuminate turns it off, and AttributeEnd turns
// off one made inside the block
BOOST_AUTO_TEST_CASE(moya_sceneclient_a_renderman_light_is_turned_off_as_a_light_source_is_test) {
    const Read read(world(
        "AttributeBegin\n"
        "Light \"PxrSphereLight\" \"key\"\n"
        "Light \"PxrRectLight\" \"fill\"\n"
        "Sphere 1 -1 1 360\n"
        "Illuminate \"key\" 0\n"
        "Sphere 2 -2 2 360\n"
        "AttributeEnd\n"
        "Sphere 3 -3 3 360\n"));
    BOOST_TEST((read.value("sphereLight", "light key", "instances") == scene::Value::handles({ read.handle("instance", "Sphere 1") })));
    BOOST_TEST((read.value("rectLight", "light fill", "instances") ==
        scene::Value::handles({ read.handle("instance", "Sphere 1"), read.handle("instance", "Sphere 2") })));
    BOOST_TEST((read.value("rectLight", "light fill", "includeRoot") == scene::Value::integer(0)));
}

// PxrMeshLight makes a mesh light of each geometry after it in its attribute block, with its
// textureColor tinting its colour
BOOST_AUTO_TEST_CASE(moya_sceneclient_a_renderman_mesh_light_makes_a_mesh_light_of_its_geometry_test) {
    const Read read(world(
        "AttributeBegin\n"
        "Light \"PxrMeshLight\" \"glow\" \"float intensity\" [2] \"color lightColor\" [1 0.5 1] \"color textureColor\" [0.5 1 1]"
        " \"int areaNormalize\" [1] \"float coneAngle\" [10]\n"
        "Polygon \"P\" [0 0 0 1 0 0 1 1 0]\n"
        "AttributeEnd\n"
        "Polygon \"P\" [0 0 1 1 0 1 1 1 1]\n"));
    BOOST_TEST(read.scene().objects("meshLight").size() == 1U);
    BOOST_TEST((read.value("meshLight", "light glow", "geometry") == scene::Value::handle(read.handle("mesh", "Polygon 1"))));
    BOOST_TEST((read.value("meshLight", "light glow", "intensity") == scene::Value::number(2.0F)));
    BOOST_TEST((read.value("meshLight", "light glow", "colour") == scene::Value::colour(glm::vec3(0.5F, 0.5F, 1.0F))));
    BOOST_TEST((read.value("meshLight", "light glow", "normalize") == scene::Value::integer(1)));
    // no cone shapes a mesh light
    BOOST_TEST((read.value("meshLight", "light glow", "coneAngle") == scene::Value::number(90.0F)));
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

// a displacement bound is held by the subdivision surfaces after it in its attribute block, in
// object space: a bound given in world space under a scale of two is half as long
BOOST_AUTO_TEST_CASE(moya_sceneclient_a_displacement_bound_is_held_in_object_space_test) {
    const std::string mesh = "SubdivisionMesh \"catmull-clark\" [4] [0 1 2 3] [] [] [] [] \"P\" [0 0 0 1 0 0 1 1 0 0 1 0]\n";
    const Read read(world("AttributeBegin\nAttribute \"displacementbound\" \"float sphere\" [0.3]\n" + mesh +
        "Scale 2 2 2\nAttribute \"displacementbound\" \"float sphere\" [0.4] \"string coordinatesystem\" [\"world\"]\n" + mesh + "AttributeEnd\n" + mesh));
    BOOST_TEST(read.value("subdivision", "SubdivisionMesh 1", "displacementBound").floatArray().front() == 0.3F);
    BOOST_TEST(read.value("subdivision", "SubdivisionMesh 2", "displacementBound").floatArray().front() == 0.2F);
    BOOST_TEST(read.value("subdivision", "SubdivisionMesh 3", "displacementBound").floatArray().front() == 0.0F);
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
    BOOST_TEST(scenes == 31U);
}

/** Where an instance's transformation takes a point. **/
glm::vec3 placed(const Read & read, const std::string & instance, const glm::vec3 & point) {
    return glm::vec3(read.value("instance", instance, "transform").matrixAt(0) * glm::vec4(point, 1.0F));
}

/** Whether two points lie within a small distance of each other. **/
bool near(const glm::vec3 & left, const glm::vec3 & right) {
    return glm::length(left - right) < 1.0e-5F;
}

// the unit cube of the RenderMan Interface specification's example: one polygon at z = +0.5,
// turned by the rotations before each copy into the faces the specification names
BOOST_AUTO_TEST_CASE(moya_sceneclient_rotations_build_the_specifications_cube_test) {
    const Read read(boost::filesystem::path("data") / "spec-cube.rib");
    const glm::vec3 far { 0.0F, 0.0F, 0.5F };
    BOOST_TEST(near(placed(read, "unitcube", far), far));
    BOOST_TEST(near(placed(read, "unitcube 2", far), glm::vec3(0.5F, 0.0F, 0.0F)));
    BOOST_TEST(near(placed(read, "unitcube 3", far), glm::vec3(0.0F, 0.0F, -0.5F)));
    BOOST_TEST(near(placed(read, "unitcube 4", far), glm::vec3(-0.5F, 0.0F, 0.0F)));
    BOOST_TEST(near(placed(read, "unitcube 5", far), glm::vec3(0.0F, -0.5F, 0.0F)));
    BOOST_TEST(near(placed(read, "unitcube 6", far), glm::vec3(0.0F, 0.5F, 0.0F)));
    BOOST_TEST((read.value("camera", "camera", "transform").matrixAt(0)[3] == glm::vec4(0.0F, 0.0F, -5.0F, 1.0F)));
}

// TransformEnd restores the transformation TransformBegin saved, and leaves the colour set
// inside the block in force
BOOST_AUTO_TEST_CASE(moya_sceneclient_a_transform_block_restores_the_transformation_alone_test) {
    const Read read(world("TransformBegin\nTranslate 5 5 5\nColor 0 1 0\nTransformEnd\nPolygon \"P\" [0 0 0 1 0 0 1 1 0]\n"));
    BOOST_TEST((read.value("instance", "Polygon 1", "transform").matrixAt(0) == glm::mat4(1.0F)));
    const scene::Handle material = read.value("instance", "Polygon 1", "material").handleArray().front();
    const scene::Handle surface = read.scene().value(material, "surface").handleArray().front();
    BOOST_TEST((read.scene().value(surface, "Cs") == scene::Value::colour(glm::vec3(0.0F, 1.0F, 0.0F))));
}

// each attribute block starts from the state before it, and its colour and transformation end
// with it
BOOST_AUTO_TEST_CASE(moya_sceneclient_an_attribute_block_restores_the_state_test) {
    const Read read(world(
        "AttributeBegin\nColor 1 0 0\nTranslate 0 0 5\nPolygon \"P\" [0 0 0 1 0 0 1 1 0]\nAttributeEnd\n"
        "AttributeBegin\nColor 0 0 1\nTranslate 0.5 0 5\nPolygon \"P\" [0 0 0 1 0 0 1 1 0]\nAttributeEnd\n"
        "Polygon \"P\" [0 0 0 1 0 0 1 1 0]\n"));
    BOOST_TEST((read.value("instance", "Polygon 2", "transform").matrixAt(0)[3] == glm::vec4(0.5F, 0.0F, 5.0F, 1.0F)));
    BOOST_TEST((read.value("instance", "Polygon 3", "transform").matrixAt(0) == glm::mat4(1.0F)));
    const auto colourOf = [&read](const std::string & instance) {
        const scene::Handle material = read.value("instance", instance, "material").handleArray().front();
        return read.scene().value(read.scene().value(material, "surface").handleArray().front(), "Cs");
    };
    BOOST_TEST((colourOf("Polygon 2") == scene::Value::colour(glm::vec3(0.0F, 0.0F, 1.0F))));
    BOOST_TEST((colourOf("Polygon 3") == scene::Value::colour(glm::vec3(1.0F))));
    BOOST_TEST(!(read.value("instance", "Polygon 1", "material") == read.value("instance", "Polygon 2", "material")));
}

// CoordinateSystem names the current transformation, CoordSysTransform makes a named one current,
// and "world" is the identity
BOOST_AUTO_TEST_CASE(moya_sceneclient_named_spaces_hold_their_transformations_test) {
    const Read read(world(
        "Scale 2 2 2\nCoordinateSystem \"lamp\"\nIdentity\nCoordSysTransform \"lamp\"\nCoordinateSystem \"copy\"\nIdentity\n"
        "CoordSysTransform \"copy\"\nPolygon \"P\" [0 0 0 1 0 0 1 1 0]\n"
        "CoordSysTransform \"world\"\nPolygon \"P\" [0 0 0 1 0 0 1 1 0]\n"));
    const glm::mat4 doubled { glm::vec4(2.0F, 0.0F, 0.0F, 0.0F), glm::vec4(0.0F, 2.0F, 0.0F, 0.0F), glm::vec4(0.0F, 0.0F, 2.0F, 0.0F),
        glm::vec4(0.0F, 0.0F, 0.0F, 1.0F) };
    BOOST_TEST((read.value("instance", "Polygon 1", "transform").matrixAt(0) == doubled));
    BOOST_TEST((read.value("instance", "Polygon 2", "transform").matrixAt(0) == glm::mat4(1.0F)));
}

// a shader of no name changes nothing, so the surface before it stays in force
BOOST_AUTO_TEST_CASE(moya_sceneclient_a_shader_of_no_name_changes_nothing_test) {
    const Read read(world("Surface \"plastic\"\nSurface \"\"\nPolygon \"P\" [0 0 0 1 0 0 1 1 0]\n"));
    const scene::Handle material = read.value("instance", "Polygon 1", "material").handleArray().front();
    const scene::Handle surface = read.scene().value(material, "surface").handleArray().front();
    BOOST_TEST((read.scene().value(surface, "shader") == scene::Value::string("plastic")));
}

// an RI displacement is a node of the material, with its parameters, and ends with its block
BOOST_AUTO_TEST_CASE(moya_sceneclient_an_ri_displacement_is_held_by_the_material_test) {
    const Read read(world(
        "AttributeBegin\nSurface \"matte\"\nDisplacement \"lift\" \"float amount\" [0.1]\nPolygon \"P\" [0 0 0 1 0 0 1 1 0]\nAttributeEnd\n"
        "Polygon \"P\" [0 0 0 1 0 0 1 1 0]\n"));
    const scene::Handle material = read.value("instance", "Polygon 1", "material").handleArray().front();
    const std::vector<scene::Handle> & displacement = read.scene().value(material, "displacement").handleArray();
    BOOST_TEST_REQUIRE(displacement.size() == 1U);
    BOOST_TEST((read.scene().value(displacement.front(), "shader") == scene::Value::string("lift")));
    BOOST_TEST((read.scene().value(displacement.front(), "language") == scene::Value::string("ri")));
    BOOST_TEST((read.scene().value(displacement.front(), "amount") == scene::Value::floats({ 0.1F })));
    const scene::Handle after = read.value("instance", "Polygon 2", "material").handleArray().front();
    BOOST_TEST((read.scene().value(after, "displacement") == scene::Value::handle({})));
}

// a transformation that moves from nothing at one end of the shutter to nothing at the other
// keeps both samples
BOOST_AUTO_TEST_CASE(moya_sceneclient_a_motion_flat_at_both_ends_keeps_both_test) {
    const Read read("Shutter 0 1\n" + world("MotionBegin [0 1]\nScale 0 1 1\nScale 1 0 1\nMotionEnd\nPolygon \"P\" [0 0 0 1 0 0 1 1 0]\n"));
    const scene::Samples & samples = read.scene().samples(read.handle("instance", "Polygon 1"), "transform");
    BOOST_TEST(samples.times == (std::vector<float> { 0.0F, 1.0F }), boost::test_tools::per_element());
    BOOST_TEST_REQUIRE(samples.values.size() == 2U);
    BOOST_TEST((samples.values[0].matrixAt(0)[0] == glm::vec4(0.0F)));
    BOOST_TEST((samples.values[1].matrixAt(0)[1] == glm::vec4(0.0F)));
}

// every geometry after an area light in its block emits, a quadric as a mesh does, and a light
// before the block makes no mesh light
BOOST_AUTO_TEST_CASE(moya_sceneclient_an_area_light_takes_every_geometry_in_its_block_test) {
    const Read read(world(
        "LightSource \"pointlight\" 1\n"
        "AttributeBegin\nAreaLightSource \"arealight\" 2\nPolygon \"P\" [0 0 0 1 0 0 1 1 0]\nSphere 1 -1 1 360\nAttributeEnd\n"));
    BOOST_TEST(read.scene().objects("meshLight").size() == 2U);
    std::vector<scene::Handle> lit;
    for (const scene::Handle & light : read.scene().objects("meshLight")) {
        lit.push_back(read.scene().value(light, "geometry").handleArray().front());
    }
    BOOST_TEST((std::find(lit.begin(), lit.end(), read.handle("mesh", "Polygon 1")) != lit.end()));
    BOOST_TEST((std::find(lit.begin(), lit.end(), read.handle("sphere", "Sphere 1")) != lit.end()));
}

// a curve that names no width takes RI's width of one
BOOST_AUTO_TEST_CASE(moya_sceneclient_a_curve_of_no_width_is_one_wide_test) {
    const Read read(world("Curves \"linear\" [2] \"nonperiodic\" \"P\" [0 0 0 0 0 1]\n"));
    BOOST_TEST((read.value("curves", "Curves 1", "constantWidth") == scene::Value::number(1.0F)));
    BOOST_TEST(read.value("curves", "Curves 1", "width").floatArray().empty());
}

};  // namespace
