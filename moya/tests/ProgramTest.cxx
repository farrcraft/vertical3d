/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/colour/RgbTables.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/geometry/ConvertedMaterials.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/geometry/Surfaces.h>
#include <moya/libmoya/kernel/Array.h>
#include <moya/libmoya/kernel/bsdf/AtSurface.h>
#include <moya/libmoya/kernel/bsdf/Bsdf.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/program/Instruction.h>
#include <moya/libmoya/kernel/program/Program.h>
#include <moya/libmoya/kernel/program/Run.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>
#include <moya/libmoya/kernel/spectrum/Wavelengths.h>
#include <moya/libmoya/kernel/surface/OpenPbr.h>
#include <moya/libmoya/kernel/surface/ShadingPoint.h>
#include <moya/libmoya/kernel/surface/Surfaces.h>
#include <moya/libmoya/material/Albedos.h>
#include <moya/libmoya/material/Compile.h>
#include <moya/libmoya/material/Library.h>
#include <moya/libmoya/scene/Declaration.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Kinds.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>
#include <moya/libmoya/view/Traced.h>
#include <moya/libmoya/view/Tracing.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

namespace {

namespace colour = v3d::moya::colour;
namespace geometry = v3d::moya::geometry;
namespace kernel = v3d::moya::kernel;
namespace material = v3d::moya::material;
namespace program = v3d::moya::kernel::program;
namespace scene = v3d::moya::scene;
namespace view = v3d::moya::view;
using kernel::surface::OpenPbr;

constexpr const char * kOpenPbr = "ND_open_pbr_surface_surfaceshader";
constexpr float kPi = 3.14159265F;

/** An input of a node and its value. **/
struct Given {
    const char * name;
    scene::Value value;
};

/** The float inputs of open_pbr_surface that a test reads up to four components through. **/
constexpr const char * kReadThrough[4] = { "base_weight", "base_diffuse_roughness", "base_metalness", "specular_weight" };

/** A node's definition, its output's type, its inputs, and what MaterialX defines it to give. **/
struct Case {
    const char * definition;
    const char * type;
    std::vector<Given> inputs;
    std::vector<float> expected;
};

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the node's name, then its definition
scene::Handle node(scene::Scene & scene, const std::string & name, const std::string & definition, const std::vector<Given> & inputs) {
    const scene::Handle out = scene.create("shader", name);
    scene.set(out, "shader", scene::Value::string(definition));
    for (const Given & input : inputs) {
        scene.declare(out, scene::Declaration { .name = input.name, .type = input.value.type(), .count = static_cast<std::uint32_t>(input.value.size()), .fallback = input.value });
        scene.set(out, input.name, input.value);
    }
    return out;
}

void connect(scene::Scene & scene, const scene::Handle & from, const scene::Handle & to, const std::string & input) {
    const scene::Handle out = scene.create("connection", std::to_string(from.index) + " to " + input + " of " + std::to_string(to.index));
    scene.set(out, "from", scene::Value::handle(from));
    scene.set(out, "to", scene::Value::handle(to));
    scene.set(out, "input", scene::Value::string(input));
}

scene::Handle materialOf(scene::Scene & scene, const std::string & name, const scene::Handle & surface) {
    const scene::Handle out = scene.create("material", name);
    scene.set(out, "surface", scene::Value::handle(surface));
    scene.commit();
    scene.apply();
    return out;
}

/** The parameters a compiled material's program gives when it runs. **/
OpenPbr run(const material::Compiled & compiled) {
    std::vector<program::Instruction> instructions = compiled.instructions;
    std::vector<float> values = compiled.values;
    return program::evaluate(kernel::Array<program::Instruction> { instructions.data(), static_cast<std::uint32_t>(instructions.size()) },
        kernel::Array<float> { values.data(), static_cast<std::uint32_t>(values.size()) }, compiled.material.program);
}

/** How many of the reports mention a text. **/
std::size_t mentioning(const std::vector<std::string> & reports, const std::string & text) {
    return static_cast<std::size_t>(std::count_if(reports.begin(), reports.end(), [&](const std::string & report) {
        return report.find(text) != std::string::npos;
    }));
}

float readBack(const OpenPbr & layers, std::size_t component) {
    const float read[4] = { layers.baseWeight, layers.baseDiffuseRoughness, layers.baseMetalness, layers.specularWeight };
    return read[component];
}

/**
 * Each component of a node's output, of a MaterialX type, at its inputs, as the program gives it
 * at a surface. A colour is read through base_color, a float through base_weight, and a vector
 * or a colour of four components through an extract node for each.
 **/
std::vector<float> outputOf(const char * definition, const std::string & type, const std::vector<Given> & inputs) {
    scene::Scene scene(scene::kinds());
    const scene::Handle tested = node(scene, "tested", definition, inputs);
    const scene::Handle surface = node(scene, "surface", kOpenPbr, {});
    std::size_t components = 1;
    if (type == "color3") {
        connect(scene, tested, surface, "base_color");
        components = 3;
    } else if (type == "float") {
        connect(scene, tested, surface, kReadThrough[0]);
    } else if (type == "integer" || type == "boolean") {
        const scene::Handle converted = node(scene, "converted", "ND_convert_" + type + "_float", {});
        connect(scene, tested, converted, "in");
        connect(scene, converted, surface, kReadThrough[0]);
    } else {
        components = type == "vector2" ? 2U : 4U;
        if (type == "vector3") {
            components = 3U;
        }
        for (std::size_t i = 0; i < components; ++i) {
            const scene::Handle extract = node(scene, "extract" + std::to_string(i), "ND_extract_" + type, { { "index", scene::Value::integer(static_cast<std::int32_t>(i)) } });
            connect(scene, tested, extract, "in");
            connect(scene, extract, surface, kReadThrough[i]);
        }
    }
    const scene::Handle held = materialOf(scene, "tested", surface);
    const material::Compiled compiled = material::compile(material::Library::installed(), scene, held);
    BOOST_TEST(compiled.reports.empty(), definition << " reports " << (compiled.reports.empty() ? std::string() : compiled.reports.front()));
    const OpenPbr layers = run(compiled);
    std::vector<float> out;
    out.reserve(components);
    for (std::size_t i = 0; i < components; ++i) {
        out.push_back(type == "color3" ? layers.baseColour[static_cast<glm::length_t>(i)] : readBack(layers, i));
    }
    return out;
}

void check(const std::vector<Case> & cases) {
    for (const Case & each : cases) {
        BOOST_TEST_CONTEXT(each.definition) {
            const std::vector<float> got = outputOf(each.definition, each.type, each.inputs);
            BOOST_TEST_REQUIRE(got.size() == each.expected.size());
            for (std::size_t i = 0; i < got.size(); ++i) {
                BOOST_TEST(got[i] == each.expected[i], boost::test_tools::tolerance(1.0e-5F));
            }
        }
    }
}

scene::Value number(float value) {
    return scene::Value::number(value);
}

scene::Value vector(float x, float y, float z) {
    return scene::Value::vector(glm::vec3 { x, y, z });
}

scene::Value colour3(float red, float green, float blue) {
    return scene::Value::colour(glm::vec3 { red, green, blue });
}

BOOST_AUTO_TEST_CASE(moya_program_math_nodes_give_what_materialx_defines_test) {
    check({
        { "ND_add_float", "float", { { "in1", number(1.5F) }, { "in2", number(2.0F) } }, { 3.5F } },
        { "ND_add_color3FA", "color3", { { "in1", colour3(0.1F, 0.2F, 0.3F) }, { "in2", number(0.5F) } }, { 0.6F, 0.7F, 0.8F } },
        { "ND_subtract_vector3", "vector3", { { "in1", vector(1.0F, 2.0F, 3.0F) }, { "in2", vector(0.5F, 0.5F, 0.5F) } }, { 0.5F, 1.5F, 2.5F } },
        { "ND_multiply_vector2", "vector2", { { "in1", scene::Value::floats({ 2.0F, 3.0F }) }, { "in2", scene::Value::floats({ 4.0F, 5.0F }) } }, { 8.0F, 15.0F } },
        { "ND_divide_float", "float", { { "in1", number(3.0F) }, { "in2", number(4.0F) } }, { 0.75F } },
        { "ND_modulo_float", "float", { { "in1", number(-1.0F) }, { "in2", number(3.0F) } }, { 2.0F } },
        { "ND_fract_float", "float", { { "in", number(-0.25F) } }, { 0.75F } },
        { "ND_invert_float", "float", { { "in", number(0.25F) } }, { 0.75F } },
        { "ND_absval_float", "float", { { "in", number(-2.0F) } }, { 2.0F } },
        { "ND_floor_float", "float", { { "in", number(-1.5F) } }, { -2.0F } },
        { "ND_floor_integer", "integer", { { "in", number(2.7F) } }, { 2.0F } },
        { "ND_ceil_float", "float", { { "in", number(1.2F) } }, { 2.0F } },
        { "ND_round_float", "float", { { "in", number(2.5F) } }, { 3.0F } },
        { "ND_power_float", "float", { { "in1", number(2.0F) }, { "in2", number(3.0F) } }, { 8.0F } },
        { "ND_sin_float", "float", { { "in", number(kPi / 2.0F) } }, { 1.0F } },
        { "ND_cos_float", "float", { { "in", number(0.0F) } }, { 1.0F } },
        { "ND_tan_float", "float", { { "in", number(kPi / 4.0F) } }, { 1.0F } },
        { "ND_asin_float", "float", { { "in", number(1.0F) } }, { kPi / 2.0F } },
        { "ND_acos_float", "float", { { "in", number(1.0F) } }, { 0.0F } },
        { "ND_atan2_float", "float", { { "iny", number(1.0F) }, { "inx", number(0.0F) } }, { kPi / 2.0F } },
        { "ND_sqrt_float", "float", { { "in", number(9.0F) } }, { 3.0F } },
        { "ND_ln_float", "float", { { "in", number(std::exp(1.0F)) } }, { 1.0F } },
        { "ND_exp_float", "float", { { "in", number(0.0F) } }, { 1.0F } },
        { "ND_sign_float", "float", { { "in", number(-3.0F) } }, { -1.0F } },
        { "ND_clamp_float", "float", { { "in", number(1.5F) } }, { 1.0F } },
        { "ND_clamp_vector3FA", "vector3", { { "in", vector(-1.0F, 0.5F, 2.0F) }, { "low", number(0.0F) }, { "high", number(1.0F) } }, { 0.0F, 0.5F, 1.0F } },
        { "ND_min_float", "float", { { "in1", number(1.0F) }, { "in2", number(2.0F) } }, { 1.0F } },
        { "ND_max_float", "float", { { "in1", number(1.0F) }, { "in2", number(2.0F) } }, { 2.0F } },
        { "ND_normalize_vector3", "vector3", { { "in", vector(3.0F, 0.0F, 4.0F) } }, { 0.6F, 0.0F, 0.8F } },
        { "ND_magnitude_vector3", "float", { { "in", vector(3.0F, 0.0F, 4.0F) } }, { 5.0F } },
        { "ND_dotproduct_vector3", "float", { { "in1", vector(1.0F, 2.0F, 3.0F) }, { "in2", vector(4.0F, 5.0F, 6.0F) } }, { 32.0F } },
        { "ND_crossproduct_vector3", "vector3", { { "in1", vector(1.0F, 0.0F, 0.0F) }, { "in2", vector(0.0F, 1.0F, 0.0F) } }, { 0.0F, 0.0F, 1.0F } },
    });
}

BOOST_AUTO_TEST_CASE(moya_program_colour_nodes_give_what_materialx_defines_test) {
    check({
        { "ND_remap_float", "float", { { "in", number(0.5F) }, { "outlow", number(2.0F) }, { "outhigh", number(4.0F) } }, { 3.0F } },
        { "ND_smoothstep_float", "float", { { "in", number(0.25F) } }, { 0.15625F } },
        { "ND_mix_color3", "color3", { { "fg", colour3(1.0F, 0.0F, 0.0F) }, { "bg", colour3(0.0F, 0.0F, 1.0F) }, { "mix", number(0.25F) } }, { 0.25F, 0.0F, 0.75F } },
        { "ND_plus_float", "float", { { "fg", number(0.25F) }, { "bg", number(0.5F) } }, { 0.75F } },
        { "ND_minus_float", "float", { { "fg", number(0.25F) }, { "bg", number(0.5F) } }, { 0.25F } },
        { "ND_difference_float", "float", { { "fg", number(0.75F) }, { "bg", number(0.25F) } }, { 0.5F } },
        { "ND_burn_float", "float", { { "fg", number(0.5F) }, { "bg", number(0.75F) } }, { 0.5F } },
        { "ND_dodge_float", "float", { { "fg", number(0.5F) }, { "bg", number(0.25F) } }, { 0.5F } },
        { "ND_screen_float", "float", { { "fg", number(0.5F) }, { "bg", number(0.5F) }, { "mix", number(0.5F) } }, { 0.625F } },
        { "ND_inside_float", "float", { { "in", number(0.5F) }, { "mask", number(0.5F) } }, { 0.25F } },
        { "ND_outside_float", "float", { { "in", number(0.5F) }, { "mask", number(0.25F) } }, { 0.375F } },
        { "ND_luminance_color3", "color3", { { "in", colour3(1.0F, 0.0F, 0.0F) } }, { 0.2722287F, 0.2722287F, 0.2722287F } },
        { "ND_rgbtohsv_color3", "color3", { { "in", colour3(0.0F, 0.5F, 1.0F) } }, { 7.0F / 12.0F, 1.0F, 1.0F } },
        { "ND_hsvtorgb_color3", "color3", { { "in", colour3(7.0F / 12.0F, 1.0F, 1.0F) } }, { 0.0F, 0.5F, 1.0F } },
    });
}

BOOST_AUTO_TEST_CASE(moya_program_logic_nodes_give_what_materialx_defines_test) {
    check({
        { "ND_ifgreater_float", "float", { { "value1", number(2.0F) }, { "value2", number(1.0F) }, { "in1", number(5.0F) }, { "in2", number(7.0F) } }, { 5.0F } },
        { "ND_ifgreatereq_float", "float", { { "value1", number(1.0F) }, { "value2", number(1.0F) }, { "in1", number(5.0F) }, { "in2", number(7.0F) } }, { 5.0F } },
        { "ND_ifequal_float", "float", { { "value1", number(1.0F) }, { "value2", number(2.0F) }, { "in1", number(5.0F) }, { "in2", number(7.0F) } }, { 7.0F } },
        { "ND_ifequal_floatB", "float", { { "value1", scene::Value::integer(1) }, { "value2", scene::Value::integer(1) }, { "in1", number(5.0F) } }, { 5.0F } },
        { "ND_logical_and", "boolean", { { "in1", scene::Value::integer(1) }, { "in2", scene::Value::integer(0) } }, { 0.0F } },
        { "ND_logical_or", "boolean", { { "in1", scene::Value::integer(1) }, { "in2", scene::Value::integer(0) } }, { 1.0F } },
        { "ND_logical_not", "boolean", { { "in", scene::Value::integer(0) } }, { 1.0F } },
    });
}

BOOST_AUTO_TEST_CASE(moya_program_channel_nodes_give_what_materialx_defines_test) {
    check({
        { "ND_constant_color3", "color3", { { "value", colour3(0.1F, 0.2F, 0.3F) } }, { 0.1F, 0.2F, 0.3F } },
        { "ND_dot_float", "float", { { "in", number(0.4F) } }, { 0.4F } },
        { "ND_extract_vector3", "float", { { "in", vector(1.0F, 2.0F, 3.0F) }, { "index", scene::Value::integer(2) } }, { 3.0F } },
        { "ND_combine2_vector2", "vector2", { { "in1", number(1.0F) }, { "in2", number(2.0F) } }, { 1.0F, 2.0F } },
        { "ND_combine3_vector3", "vector3", { { "in1", number(1.0F) }, { "in2", number(2.0F) }, { "in3", number(3.0F) } }, { 1.0F, 2.0F, 3.0F } },
        { "ND_combine2_vector4VF", "vector4", { { "in1", vector(1.0F, 2.0F, 3.0F) }, { "in2", number(4.0F) } }, { 1.0F, 2.0F, 3.0F, 4.0F } },
        { "ND_combine4_color4", "color4", { { "in1", number(0.1F) }, { "in2", number(0.2F) }, { "in3", number(0.3F) }, { "in4", number(0.4F) } }, { 0.1F, 0.2F, 0.3F, 0.4F } },
    });
}

// a node feeding two inputs is compiled once, and a constant material runs no instructions
BOOST_AUTO_TEST_CASE(moya_program_a_node_is_compiled_once_and_constants_run_nothing_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle grey = node(scene, "grey", "ND_constant_color3", { { "value", colour3(0.5F, 0.5F, 0.5F) } });
    const scene::Handle surface = node(scene, "surface", kOpenPbr, { { "specular_roughness", number(0.2F) } });
    connect(scene, grey, surface, "base_color");
    connect(scene, grey, surface, "coat_color");
    const scene::Handle fed = materialOf(scene, "fed", surface);
    const material::Compiled compiled = material::compile(material::Library::installed(), scene, fed);
    // the node, then a copy into each input
    BOOST_TEST(compiled.instructions.size() == 3U);

    scene::Scene constant(scene::kinds());
    const scene::Handle plain = materialOf(constant, "plain", node(constant, "surface", kOpenPbr, { { "base_color", colour3(0.5F, 0.5F, 0.5F) } }));
    const material::Compiled still = material::compile(material::Library::installed(), constant, plain);
    BOOST_TEST(still.material.program.count == 0U);
    BOOST_TEST(still.material.surface.baseColour.x == 0.5F);
}

// a node that reads its own output is reported, and gives its output's default
BOOST_AUTO_TEST_CASE(moya_program_a_node_reading_its_own_output_is_reported_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle looped = node(scene, "looped", "ND_add_color3", { { "in2", colour3(0.1F, 0.1F, 0.1F) } });
    connect(scene, looped, looped, "in1");
    const scene::Handle surface = node(scene, "surface", kOpenPbr, {});
    connect(scene, looped, surface, "base_color");
    const scene::Handle held = materialOf(scene, "looped", surface);
    const material::Compiled compiled = material::compile(material::Library::installed(), scene, held);
    BOOST_TEST(mentioning(compiled.reports, "reads its own output") == 1U);
    const OpenPbr layers = run(compiled);
    BOOST_TEST(layers.baseColour.x == 0.1F, boost::test_tools::tolerance(1.0e-6F));
}

// a program that needs more registers than a shading point holds is reported, and each input a
// node feeds gives that node's default
BOOST_AUTO_TEST_CASE(moya_program_a_program_too_large_is_reported_and_gives_defaults_test) {
    scene::Scene scene(scene::kinds());
    scene::Handle last = node(scene, "first", "ND_constant_color3", { { "value", colour3(0.1F, 0.1F, 0.1F) } });
    for (int i = 0; i < 64; ++i) {
        const scene::Handle next = node(scene, "add" + std::to_string(i), "ND_add_color3", { { "in2", colour3(0.01F, 0.01F, 0.01F) } });
        connect(scene, last, next, "in1");
        last = next;
    }
    const scene::Handle surface = node(scene, "surface", kOpenPbr, {});
    connect(scene, last, surface, "base_color");
    const scene::Handle held = materialOf(scene, "large", surface);
    const material::Compiled compiled = material::compile(material::Library::installed(), scene, held);
    BOOST_TEST(mentioning(compiled.reports, "more registers") == 1U);
    BOOST_TEST(compiled.values.size() <= program::kRegisters);
    BOOST_TEST(run(compiled).baseColour.x == 0.0F);
}

/** A square facing +z, placed with a material. **/
void placeSquare(scene::Scene & scene, const scene::Handle & held) {
    const scene::Handle mesh = scene.create("mesh", "square");
    scene.set(mesh, "P", scene::Value::points({ { -1.0F, -1.0F, 0.0F }, { 1.0F, -1.0F, 0.0F }, { 1.0F, 1.0F, 0.0F }, { -1.0F, 1.0F, 0.0F } }));
    scene.set(mesh, "vertices", scene::Value::integers({ 4 }));
    scene.set(mesh, "indices", scene::Value::integers({ 0, 1, 2, 3 }));
    const scene::Handle placed = scene.create("instance", "square");
    scene.set(placed, "prototype", scene::Value::handle(mesh));
    scene.set(placed, "material", scene::Value::handle(held));
    scene.commit();
    scene.apply();
}

// each colour constant, and the surface's parameters, are converted into ACEScg from the space
// the scene names for them
BOOST_AUTO_TEST_CASE(moya_program_a_colour_is_converted_from_its_space_test) {
    const std::string space = "sRGB Encoded Rec.709 (sRGB)";
    scene::Scene scene(scene::kinds());
    const scene::Handle mixed = node(scene, "mixed", "ND_mix_color3",
        { { "fg", scene::Value::colour(glm::vec3 { 0.5F, 0.2F, 0.1F }, space) }, { "bg", colour3(0.0F, 0.0F, 0.0F) }, { "mix", number(1.0F) } });
    const scene::Handle surface = node(scene, "surface", kOpenPbr, { { "coat_color", scene::Value::colour(glm::vec3 { 0.5F, 0.2F, 0.1F }, space) } });
    connect(scene, mixed, surface, "base_color");
    placeSquare(scene, materialOf(scene, "tagged", surface));

    const colour::Config config;
    const glm::vec3 expected = config.input(space).apply(glm::vec3 { 0.5F, 0.2F, 0.1F });
    const geometry::Prototypes prototypes = geometry::gather(scene, {});
    const geometry::ConvertedMaterials converted = geometry::ConvertedMaterials::of(prototypes, config);
    BOOST_TEST_REQUIRE(converted.materials.size() == 1U);
    const kernel::surface::Material & made = converted.materials.front();
    BOOST_TEST(made.surface.coatColour.x == expected.x, boost::test_tools::tolerance(1.0e-5F));
    BOOST_TEST(made.surface.coatColour.z == expected.z, boost::test_tools::tolerance(1.0e-5F));
    std::vector<program::Instruction> instructions = prototypes.instructions;
    std::vector<float> values = converted.programValues;
    const OpenPbr layers = program::evaluate(kernel::Array<program::Instruction> { instructions.data(), static_cast<std::uint32_t>(instructions.size()) },
        kernel::Array<float> { values.data(), static_cast<std::uint32_t>(values.size()) }, made.program);
    BOOST_TEST(layers.baseColour.x == expected.x, boost::test_tools::tolerance(1.0e-5F));
    BOOST_TEST(layers.baseColour.y == expected.y, boost::test_tools::tolerance(1.0e-5F));
}

/** The BSDF at the middle of a scene's square, seen from above, through the device's surfaces. **/
kernel::bsdf::Bsdf bsdfOf(const scene::Scene & scene) {
    const geometry::Prototypes prototypes = geometry::gather(scene, {});
    kernel::geometry::Ray ray;
    ray.origin = glm::vec3 { 0.1F, 0.2F, 3.0F };
    ray.direction = glm::vec3 { 0.0F, 0.0F, -1.0F };
    v3d::moya::device::CpuDevice device;
    const view::Traced traced = view::trace(device, prototypes, { ray }, view::Tracer::Hierarchy);
    const geometry::Surfaces surfaces(device, prototypes, colour::Config());
    const kernel::surface::Surfaces held = surfaces.view();
    const kernel::surface::ShadingPoint point = held.at(traced.nearest.front(), ray);
    BOOST_TEST_REQUIRE(point.found());
    const auto wavelengths = kernel::spectrum::SampledWavelengths::visible(0.37F);
    return kernel::bsdf::atSurface(held, point, colour::acescgTable(), material::ggxAlbedos(), wavelengths, glm::vec3 { 0.0F, 0.0F, 1.0F });
}

// a material whose base colour mixes two colours by a constant renders as one whose base colour
// is the mix
BOOST_AUTO_TEST_CASE(moya_program_a_mix_of_two_colours_renders_as_the_mix_test) {
    scene::Scene mixing(scene::kinds());
    const scene::Handle mixed = node(mixing, "mixed", "ND_mix_color3", { { "fg", colour3(0.9F, 0.1F, 0.1F) }, { "bg", colour3(0.1F, 0.1F, 0.9F) }, { "mix", number(0.25F) } });
    const scene::Handle surface = node(mixing, "surface", kOpenPbr, {});
    connect(mixing, mixed, surface, "base_color");
    placeSquare(mixing, materialOf(mixing, "mixed", surface));

    scene::Scene plain(scene::kinds());
    const scene::Handle constant = node(plain, "surface", kOpenPbr, { { "base_color", colour3(0.3F, 0.1F, 0.7F) } });
    placeSquare(plain, materialOf(plain, "plain", constant));

    // OpenPBR's default base colour, which a program that did not run would give
    scene::Scene unmixed(scene::kinds());
    placeSquare(unmixed, materialOf(unmixed, "grey", node(unmixed, "surface", kOpenPbr, {})));

    const kernel::bsdf::Bsdf got = bsdfOf(mixing);
    const kernel::bsdf::Bsdf expected = bsdfOf(plain);
    const kernel::bsdf::Bsdf grey = bsdfOf(unmixed);
    BOOST_TEST_REQUIRE(got.count == expected.count);
    // light reflected towards the camera from along the normal and from a slant
    const glm::vec3 outgoing { 0.0F, 0.0F, 1.0F };
    for (const glm::vec3 & incoming : { glm::vec3 { 0.0F, 0.0F, 1.0F }, glm::normalize(glm::vec3 { 0.5F, 0.2F, 1.0F }) }) {
        const kernel::spectrum::SampledSpectrum value = got.evaluate(outgoing, incoming);
        const kernel::spectrum::SampledSpectrum wanted = expected.evaluate(outgoing, incoming);
        const kernel::spectrum::SampledSpectrum greyed = grey.evaluate(outgoing, incoming);
        bool differs = false;
        for (int sample = 0; sample < kernel::spectrum::kSamples; ++sample) {
            BOOST_TEST(value[sample] == wanted[sample], boost::test_tools::tolerance(1.0e-5F));
            differs = differs || std::fabs(value[sample] - greyed[sample]) > 1.0e-3F;
        }
        BOOST_TEST(differs);
    }
}

};  // namespace
