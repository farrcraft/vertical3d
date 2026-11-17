/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/kernel/surface/OpenPbr.h>
#include <moya/libmoya/kernel/volume/Interior.h>
#include <moya/libmoya/kernel/volume/Medium.h>
#include <moya/libmoya/material/Compile.h>
#include <moya/libmoya/material/Library.h>
#include <moya/libmoya/scene/Declaration.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Kinds.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/vec3.hpp>

namespace {

namespace kernel = v3d::moya::kernel;
namespace material = v3d::moya::material;
namespace scene = v3d::moya::scene;

constexpr float kClose = 1.0e-5F;

/** A MaterialX node of a definition. **/
scene::Handle node(scene::Scene & scene, const std::string & name, const char * definition) {
    const scene::Handle out = scene.create("shader", name);
    scene.set(out, "shader", scene::Value::string(definition));
    return out;
}

/** Sets a node's input, declaring it of the value's type. **/
void setInput(scene::Scene & scene, const scene::Handle & node, const std::string & name, const scene::Value & value) {
    scene.declare(node, scene::Declaration { .name = name, .type = value.type(), .count = static_cast<std::uint32_t>(value.size()), .fallback = value });
    scene.set(node, name, value);
}

/** Connects an output to an input, by a connection named after the node and input it feeds. **/
void connect(scene::Scene & scene, const scene::Handle & from, const scene::Handle & to, const std::string & input) {
    const scene::Handle out = scene.create("connection", std::to_string(to.index) + " " + input);
    scene.set(out, "from", scene::Value::handle(from));
    scene.set(out, "to", scene::Value::handle(to));
    scene.set(out, "input", scene::Value::string(input));
}

/** An anisotropic VDF of an absorption, a scattering and an anisotropy. **/
scene::Handle anisotropic(scene::Scene & scene, const std::string & name, const glm::vec3 & absorption, const glm::vec3 & scattering, float anisotropy) {
    const scene::Handle out = node(scene, name, "ND_anisotropic_vdf");
    setInput(scene, out, "absorption", scene::Value::vector(absorption));
    setInput(scene, out, "scattering", scene::Value::vector(scattering));
    setInput(scene, out, "anisotropy", scene::Value::number(anisotropy));
    return out;
}

/** A material whose volume terminal is a volume fed by a VDF and, unless it is none, an EDF. **/
scene::Handle volumeOf(scene::Scene & scene, const scene::Handle & vdf, const scene::Handle & edf = {}) {
    const scene::Handle shader = node(scene, "volume", "ND_volume");
    connect(scene, vdf, shader, "vdf");
    if (edf.valid()) {
        connect(scene, edf, shader, "edf");
    }
    const scene::Handle out = scene.create("material", "medium");
    scene.set(out, "volume", scene::Value::handle(shader));
    return out;
}

material::Compiled compiled(scene::Scene & scene, const scene::Handle & held) {
    scene.commit();
    scene.apply();
    std::vector<std::string> primvars;
    return material::compileVolume(material::Library::installed(), scene, held, primvars);
}

bool close(const glm::vec3 & given, const glm::vec3 & expected) {
    return std::abs(given.x - expected.x) < kClose && std::abs(given.y - expected.y) < kClose && std::abs(given.z - expected.z) < kClose;
}

bool reports(const material::Compiled & made, const std::string & part) {
    return std::ranges::any_of(made.reports, [&part](const std::string & report) { return report.find(part) != std::string::npos; });
}

// a volume of one anisotropic VDF holds its absorption, and scatters as one lobe by its scattering
// and its anisotropy
BOOST_AUTO_TEST_CASE(moya_volume_shader_one_vdf_gives_the_medium_it_names_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle fog = anisotropic(scene, "fog", glm::vec3(0.1F, 0.2F, 0.3F), glm::vec3(1.0F, 2.0F, 3.0F), 0.6F);
    const material::Compiled made = compiled(scene, volumeOf(scene, fog));

    BOOST_TEST(made.reports.empty());
    BOOST_TEST(close(made.medium.absorption, glm::vec3(0.1F, 0.2F, 0.3F)));
    BOOST_TEST(close(made.medium.lobes[0].scattering, glm::vec3(1.0F, 2.0F, 3.0F)));
    BOOST_TEST(made.medium.lobes[0].anisotropy == 0.6F);
    BOOST_TEST(close(made.medium.lobes[1].scattering, glm::vec3(0.0F)));
    BOOST_TEST(close(made.medium.emission, glm::vec3(0.0F)));
}

// a mix weighs what its fg gives by its mix and what its bg gives by the rest. Each anisotropic
// VDF scatters as a lobe of its own, in the tree's order, fg first
BOOST_AUTO_TEST_CASE(moya_volume_shader_a_mix_weighs_each_vdf_and_each_scatters_as_its_own_lobe_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle forward = anisotropic(scene, "forward", glm::vec3(4.0F, 0.0F, 0.0F), glm::vec3(8.0F), 0.5F);
    const scene::Handle back = anisotropic(scene, "back", glm::vec3(0.0F, 8.0F, 0.0F), glm::vec3(4.0F), -0.3F);
    const scene::Handle mixed = node(scene, "mixed", "ND_mix_vdf");
    setInput(scene, mixed, "mix", scene::Value::number(0.25F));
    connect(scene, forward, mixed, "fg");
    connect(scene, back, mixed, "bg");
    const material::Compiled made = compiled(scene, volumeOf(scene, mixed));

    BOOST_TEST(made.reports.empty());
    BOOST_TEST(close(made.medium.absorption, glm::vec3(1.0F, 6.0F, 0.0F)));
    BOOST_TEST(close(made.medium.lobes[0].scattering, glm::vec3(2.0F)));
    BOOST_TEST(made.medium.lobes[0].anisotropy == 0.5F);
    BOOST_TEST(close(made.medium.lobes[1].scattering, glm::vec3(3.0F)));
    BOOST_TEST(made.medium.lobes[1].anisotropy == -0.3F);
}

// an add sums what its two VDFs give, an absorption VDF absorbs and scatters nothing, and a third
// anisotropic VDF adds its scattering to the second lobe and is reported
BOOST_AUTO_TEST_CASE(moya_volume_shader_a_third_lobe_joins_the_second_and_is_reported_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle first = anisotropic(scene, "first", glm::vec3(0.0F), glm::vec3(1.0F), 0.2F);
    const scene::Handle second = anisotropic(scene, "second", glm::vec3(0.0F), glm::vec3(2.0F), 0.4F);
    const scene::Handle third = anisotropic(scene, "third", glm::vec3(0.0F), glm::vec3(3.0F), 0.8F);
    const scene::Handle dark = node(scene, "dark", "ND_absorption_vdf");
    setInput(scene, dark, "absorption", scene::Value::vector(glm::vec3(0.5F)));
    const scene::Handle lower = node(scene, "lower", "ND_add_vdf");
    const scene::Handle middle = node(scene, "middle", "ND_add_vdf");
    const scene::Handle upper = node(scene, "upper", "ND_add_vdf");
    connect(scene, first, lower, "in1");
    connect(scene, second, lower, "in2");
    connect(scene, lower, middle, "in1");
    connect(scene, third, middle, "in2");
    connect(scene, middle, upper, "in1");
    connect(scene, dark, upper, "in2");
    const material::Compiled made = compiled(scene, volumeOf(scene, upper));

    BOOST_TEST(reports(made, "more than 2 lobes"));
    BOOST_TEST(close(made.medium.absorption, glm::vec3(0.5F)));
    BOOST_TEST(close(made.medium.lobes[0].scattering, glm::vec3(1.0F)));
    BOOST_TEST(made.medium.lobes[0].anisotropy == 0.2F);
    BOOST_TEST(close(made.medium.lobes[1].scattering, glm::vec3(5.0F)));
    BOOST_TEST(made.medium.lobes[1].anisotropy == 0.4F);
}

// a uniform EDF's colour is the medium's emission, held with the colour space it names so it is
// converted as every colour is, and an add of two sums them
BOOST_AUTO_TEST_CASE(moya_volume_shader_a_uniform_edf_is_the_mediums_emission_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle fog = anisotropic(scene, "fog", glm::vec3(0.0F), glm::vec3(1.0F), 0.0F);
    const scene::Handle warm = node(scene, "warm", "ND_uniform_edf");
    const scene::Handle cool = node(scene, "cool", "ND_uniform_edf");
    setInput(scene, warm, "color", scene::Value::colour(glm::vec3(1.0F, 0.5F, 0.0F), "lin_rec709"));
    setInput(scene, cool, "color", scene::Value::colour(glm::vec3(0.0F, 0.25F, 1.0F), "lin_rec709"));
    const scene::Handle both = node(scene, "both", "ND_add_edf");
    connect(scene, warm, both, "in1");
    connect(scene, cool, both, "in2");
    const material::Compiled made = compiled(scene, volumeOf(scene, fog, both));

    BOOST_TEST(made.reports.empty());
    BOOST_TEST(close(made.medium.emission, glm::vec3(1.0F, 0.75F, 1.0F)));
    BOOST_TEST(std::ranges::count(made.spaces, std::string("lin_rec709")) == 2);
}

// a pattern that feeds a VDF runs in the volume's program, so its medium is what the program
// gives, and not a constant
BOOST_AUTO_TEST_CASE(moya_volume_shader_a_pattern_feeding_a_vdf_runs_in_its_program_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle fog = anisotropic(scene, "fog", glm::vec3(0.0F), glm::vec3(1.0F), 0.0F);
    const scene::Handle density = node(scene, "density", "ND_geompropvalue_vector3");
    setInput(scene, density, "geomprop", scene::Value::string("density"));
    connect(scene, density, fog, "scattering");
    const material::Compiled made = compiled(scene, volumeOf(scene, fog));

    BOOST_TEST(made.reports.empty());
    BOOST_TEST(!made.instructions.empty());
    BOOST_TEST(made.material.program.count == made.instructions.size());
}

// a volume terminal of any definition but MaterialX's volume is reported, and holds no medium
BOOST_AUTO_TEST_CASE(moya_volume_shader_another_definition_is_reported_and_holds_no_medium_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle surface = node(scene, "surface", "ND_open_pbr_surface_surfaceshader");
    const scene::Handle held = scene.create("material", "medium");
    scene.set(held, "volume", scene::Value::handle(surface));
    const material::Compiled made = compiled(scene, held);

    BOOST_TEST(reports(made, "holds no medium"));
    BOOST_TEST(kernel::volume::empty(made.medium));
    BOOST_TEST(made.values.empty());
}

/** The surface of a compiled OpenPBR glass of a transmission colour, a depth and a scatter. **/
kernel::surface::OpenPbr glass(const glm::vec3 & colour, float depth, const glm::vec3 & scatter) {
    scene::Scene scene(scene::kinds());
    const scene::Handle surface = node(scene, "glass", "ND_open_pbr_surface_surfaceshader");
    setInput(scene, surface, "transmission_weight", scene::Value::number(1.0F));
    setInput(scene, surface, "transmission_color", scene::Value::colour(colour, "lin_ap1"));
    setInput(scene, surface, "transmission_depth", scene::Value::number(depth));
    setInput(scene, surface, "transmission_scatter", scene::Value::colour(scatter, "lin_ap1"));
    setInput(scene, surface, "transmission_scatter_anisotropy", scene::Value::number(0.25F));
    const scene::Handle held = scene.create("material", "glass");
    scene.set(held, "surface", scene::Value::handle(surface));
    scene.commit();
    scene.apply();
    std::vector<std::string> primvars;
    return material::compile(material::Library::installed(), scene, held, primvars).material.surface;
}

// white light that travels an OpenPBR glass's depth through the medium inside it takes the
// glass's transmission colour, and the medium scatters by the scatter over the depth
BOOST_AUTO_TEST_CASE(moya_volume_shader_an_openpbr_glass_with_a_depth_holds_a_medium_of_its_colour_test) {
    const glm::vec3 colour(0.5F, 0.25F, 0.8F);
    const float depth = 2.0F;
    const kernel::volume::Medium inside = kernel::volume::interior(glass(colour, depth, glm::vec3(0.2F)));

    const glm::vec3 extinct = kernel::volume::extinction(inside);
    const glm::vec3 passed { std::exp(-extinct.x * depth), std::exp(-extinct.y * depth), std::exp(-extinct.z * depth) };
    BOOST_TEST(close(passed, colour));
    BOOST_TEST(close(inside.lobes[0].scattering, glm::vec3(0.1F)));
    BOOST_TEST(inside.lobes[0].anisotropy == 0.25F);
}

// a scatter larger than the colour's extinction would leave a channel absorbing less than nothing,
// so every channel's absorption is raised until the least is zero
BOOST_AUTO_TEST_CASE(moya_volume_shader_an_openpbr_scatter_beyond_its_extinction_raises_the_absorption_test) {
    const kernel::volume::Medium inside = kernel::volume::interior(glass(glm::vec3(0.5F, 1.0F, 1.0F), 1.0F, glm::vec3(0.1F, 0.4F, 0.1F)));
    BOOST_TEST(close(inside.absorption, glm::vec3(std::log(2.0F) - 0.1F + 0.4F, 0.0F, 0.3F)));
}

// a glass of no depth, or a thin wall, holds no medium, as OpenPBR gives it
BOOST_AUTO_TEST_CASE(moya_volume_shader_an_openpbr_glass_of_no_depth_or_a_thin_wall_holds_no_medium_test) {
    BOOST_TEST(kernel::volume::empty(kernel::volume::interior(glass(glm::vec3(0.5F), 0.0F, glm::vec3(0.2F)))));
    kernel::surface::OpenPbr thin = glass(glm::vec3(0.5F), 1.0F, glm::vec3(0.2F));
    thin.thinWalled = 1;
    BOOST_TEST(kernel::volume::empty(kernel::volume::interior(thin)));
}

// OpenPBR's subsurface radius, its scale and its anisotropy reach the surface's parameters, and a
// dispersion is reported, since nothing renders one
BOOST_AUTO_TEST_CASE(moya_volume_shader_openpbrs_subsurface_inputs_are_read_and_a_dispersion_reported_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle surface = node(scene, "skin", "ND_open_pbr_surface_surfaceshader");
    setInput(scene, surface, "subsurface_radius", scene::Value::number(0.5F));
    setInput(scene, surface, "subsurface_radius_scale", scene::Value::colour(glm::vec3(1.0F, 0.25F, 0.125F), "lin_ap1"));
    setInput(scene, surface, "subsurface_scatter_anisotropy", scene::Value::number(-0.5F));
    setInput(scene, surface, "transmission_dispersion_scale", scene::Value::number(0.5F));
    const scene::Handle held = scene.create("material", "skin");
    scene.set(held, "surface", scene::Value::handle(surface));
    scene.commit();
    scene.apply();
    std::vector<std::string> primvars;
    const material::Compiled made = material::compile(material::Library::installed(), scene, held, primvars);

    BOOST_TEST(made.material.surface.subsurfaceRadius == 0.5F);
    BOOST_TEST(close(made.material.surface.subsurfaceRadiusScale, glm::vec3(1.0F, 0.25F, 0.125F)));
    BOOST_TEST(made.material.surface.subsurfaceScatterAnisotropy == -0.5F);
    BOOST_TEST(made.material.surface.transmissionDispersionScale == 0.5F);
    BOOST_TEST(reports(made, "may disperse light"));
}

};  // namespace
