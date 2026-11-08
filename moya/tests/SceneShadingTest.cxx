/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/scene/Description.h>
#include <moya/libmoya/scene/Kinds.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Schema.h>
#include <moya/libmoya/scene/Value.h>

#include <stdexcept>
#include <string>

#include <boost/test/unit_test.hpp>

#include <glm/vec3.hpp>

namespace {

namespace scene = v3d::moya::scene;

/** A shader input of one element, of constant interpolation. **/
scene::Declaration input(const std::string & name, const scene::Value & fallback) {
    return scene::Declaration { .name = name, .type = fallback.type(), .fallback = fallback };
}

BOOST_AUTO_TEST_CASE(moya_scene_a_material_network_describes_as_it_was_built_test) {
    scene::Scene scene(scene::kinds());

    // an image feeds a surface's base colour, and the material's surface is the surface's output
    const scene::Handle image = scene.create("shader", "grid");
    scene.set(image, "shader", scene::Value::string("ND_image_color3"));
    scene.declare(image, input("file", scene::Value::string("")));
    scene.set(image, "file", scene::Value::string("grid.exr"));
    const scene::Handle surface = scene.create("shader", "plastic");
    scene.set(surface, "shader", scene::Value::string("ND_open_pbr_surface_surfaceshader"));
    scene.declare(surface, input("specular_roughness", scene::Value::number(0.3F)));
    const scene::Handle feed = scene.create("connection", "grid to plastic");
    scene.set(feed, "from", scene::Value::handle(image));
    scene.set(feed, "to", scene::Value::handle(surface));
    scene.set(feed, "input", scene::Value::string("base_color"));
    const scene::Handle material = scene.create("material", "tiles");
    scene.set(material, "surface", scene::Value::handle(surface));
    BOOST_CHECK_THROW(scene.set(material, "surface", scene::Value::handle(feed)), std::invalid_argument);
    BOOST_CHECK_THROW(scene.set(image, "language", scene::Value::string("glsl")), std::invalid_argument);
    scene.commit();
    scene.apply();

    const std::string expected =
        "connection \"grid to plastic\"\n"
        "    from handle = shader \"grid\"\n"
        "    input string = \"base_color\"\n"
        "    output string = \"out\"\n"
        "    to handle = shader \"plastic\"\n"
        "material \"tiles\"\n"
        "    displacement handle = none\n"
        "    displacementOutput string = \"out\"\n"
        "    surface handle = shader \"plastic\"\n"
        "    surfaceOutput string = \"out\"\n"
        "shader \"grid\"\n"
        "    file string = \"grid.exr\"\n"
        "    language string = \"materialx\"\n"
        "    shader string = \"ND_image_color3\"\n"
        "shader \"plastic\"\n"
        "    language string = \"materialx\"\n"
        "    shader string = \"ND_open_pbr_surface_surfaceshader\"\n"
        "    specular_roughness float = 0.3\n";
    BOOST_TEST(describe(scene) == expected);
}

BOOST_AUTO_TEST_CASE(moya_scene_a_removed_material_leaves_its_instances_with_none_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle matte = scene.create("shader", "matte");
    scene.set(matte, "shader", scene::Value::string("matte"));
    scene.set(matte, "language", scene::Value::string("ri"));
    const scene::Handle material = scene.create("material", "matte");
    scene.set(material, "surface", scene::Value::handle(matte));
    const scene::Handle placed = scene.create("instance", "square here");
    scene.set(placed, "material", scene::Value::handle(material));
    scene.commit();
    scene.apply();
    BOOST_TEST(scene.contains(scene.value(placed, "material").handleArray().front()));

    scene.remove(material);
    scene.commit();
    scene.apply();
    BOOST_TEST(!scene.contains(scene.value(placed, "material").handleArray().front()));
}

BOOST_AUTO_TEST_CASE(moya_scene_lights_take_usdluxs_defaults_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle rect = scene.create("rectLight", "key");
    const scene::Handle sphere = scene.create("sphereLight", "bulb");
    const scene::Handle sun = scene.create("distantLight", "sun");
    const scene::Handle dome = scene.create("domeLight", "sky");
    scene.commit();
    scene.apply();

    BOOST_TEST((scene.value(rect, "colour") == scene::Value::colour(glm::vec3(1.0F))));
    BOOST_TEST((scene.value(rect, "intensity") == scene::Value::number(1.0F)));
    BOOST_TEST((scene.value(rect, "exposure") == scene::Value::number(0.0F)));
    BOOST_TEST((scene.value(rect, "width") == scene::Value::number(1.0F)));
    BOOST_TEST((scene.value(rect, "coneAngle") == scene::Value::number(90.0F)));
    BOOST_TEST((scene.value(sphere, "radius") == scene::Value::number(0.5F)));
    BOOST_TEST((scene.value(sun, "angle") == scene::Value::number(0.53F)));
    BOOST_TEST((scene.value(dome, "texture") == scene::Value::string("")));
    BOOST_CHECK_THROW(scene.samples(sun, "coneAngle"), std::out_of_range);
    for (const char * kind : { "diskLight", "cylinderLight", "meshLight" }) {
        scene.create(kind, "another");
    }
}

BOOST_AUTO_TEST_CASE(moya_scene_a_mesh_light_takes_its_shape_from_a_prototype_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle mesh = scene.create("mesh", "panel");
    const scene::Handle group = scene.create("group", "panels");
    const scene::Handle light = scene.create("meshLight", "glow");
    scene.set(light, "geometry", scene::Value::handle(mesh));
    BOOST_CHECK_THROW(scene.set(light, "geometry", scene::Value::handle(group)), std::invalid_argument);
    scene.commit();
    scene.apply();

    scene.remove(mesh);
    scene.commit();
    scene.apply();
    BOOST_TEST(describe(scene).find("geometry handle = none") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(moya_scene_a_light_names_the_instances_it_lights_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle lit = scene.create("instance", "lit");
    const scene::Handle unlit = scene.create("instance", "unlit");
    const scene::Handle light = scene.create("sphereLight", "bulb");
    scene.set(light, "instances", scene::Value::handles({ lit }));
    BOOST_CHECK_THROW(scene.set(light, "instances", scene::Value::handles({ light })), std::invalid_argument);
    scene.commit();
    scene.apply();
    BOOST_TEST(scene.value(light, "instances").handleArray().size() == 1U);
    BOOST_TEST((scene.value(light, "instances").handleArray().front() == lit));
    BOOST_TEST(scene.contains(unlit));
}

};  // namespace
