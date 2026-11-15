/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/Executable.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/kernel/program/Instruction.h>
#include <moya/libmoya/kernel/program/Op.h>
#include <moya/libmoya/kernel/surface/Material.h>
#include <moya/libmoya/kernel/surface/OpenPbr.h>
#include <moya/libmoya/material/Compile.h>
#include <moya/libmoya/material/Library.h>
#include <moya/libmoya/scene/Declaration.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Kinds.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>
#include <MaterialXCore/Document.h>
#include <MaterialXCore/Node.h>
#include <MaterialXFormat/File.h>
#include <MaterialXFormat/Util.h>
#include <MaterialXFormat/XmlIo.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <ios>
#include <string>
#include <utility>
#include <vector>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/test/unit_test.hpp>

#include <glm/vec3.hpp>

namespace {

namespace kernel = v3d::moya::kernel;
namespace material = v3d::moya::material;
namespace scene = v3d::moya::scene;
namespace mx = MaterialX;
using v3d::moya::kernel::surface::OpenPbr;

constexpr const char * kOpenPbr = "ND_open_pbr_surface_surfaceshader";

/** A material compiled through the installed libraries, numbering primvars as the gather does. **/
material::Compiled compileOf(const scene::Scene & scene, const scene::Handle & held) {
    std::vector<std::string> primvars { "N", "Cs", "tangent", "st" };
    return material::compile(material::Library::installed(), scene, held, primvars);
}

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

void connect(scene::Scene & scene, const scene::Handle & from, const scene::Handle & to, const std::string & input) {
    const scene::Handle out = scene.create("connection", input);
    scene.set(out, "from", scene::Value::handle(from));
    scene.set(out, "to", scene::Value::handle(to));
    scene.set(out, "input", scene::Value::string(input));
}

scene::Handle materialOf(scene::Scene & scene, const std::string & name, const scene::Handle & surface) {
    const scene::Handle out = scene.create("material", name);
    scene.set(out, "surface", scene::Value::handle(surface));
    return out;
}

void apply(scene::Scene & scene) {
    scene.commit();
    scene.apply();
}

/** Each node of a document, and each of its inputs with its value or what feeds it, sorted. **/
std::vector<std::string> nodesOf(const mx::DocumentPtr & document) {
    std::vector<std::string> out;
    for (const mx::NodePtr & each : document->getNodes()) {
        out.push_back(each->getName() + " " + each->getCategory() + " " + each->getType());
        for (const mx::InputPtr & input : each->getInputs()) {
            out.push_back(each->getName() + "." + input->getName() + " " + input->getType() + " " + input->getValueString() + " <- " +
                input->getNodeName() + "." + input->getOutputString());
        }
    }
    std::sort(out.begin(), out.end());
    return out;
}

/** How many of the reports mention a text. **/
std::size_t mentioning(const std::vector<std::string> & reports, const std::string & text) {
    return static_cast<std::size_t>(std::count_if(reports.begin(), reports.end(), [&](const std::string & report) {
        return report.find(text) != std::string::npos;
    }));
}

// each input set is the constant the BSDF takes, and every other input is OpenPBR's default
BOOST_AUTO_TEST_CASE(moya_material_compile_constant_inputs_give_the_bsdf_they_name_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle surface = node(scene, "metal", kOpenPbr);
    setInput(scene, surface, "base_metalness", scene::Value::number(1.0F));
    setInput(scene, surface, "base_color", scene::Value::colour(glm::vec3 { 0.9F, 0.6F, 0.2F }));
    setInput(scene, surface, "specular_roughness", scene::Value::number(0.15F));
    setInput(scene, surface, "specular_roughness_anisotropy", scene::Value::number(0.4F));
    setInput(scene, surface, "coat_weight", scene::Value::number(0.5F));
    setInput(scene, surface, "coat_color", scene::Value::colour(glm::vec3 { 0.1F, 0.2F, 0.3F }));
    setInput(scene, surface, "emission_luminance", scene::Value::number(40.0F));
    setInput(scene, surface, "geometry_thin_walled", scene::Value::integer(1));
    const scene::Handle held = materialOf(scene, "gold", surface);
    apply(scene);

    const material::Compiled compiled = compileOf(scene, held);
    BOOST_TEST(compiled.reports.empty());
    BOOST_TEST(compiled.material.layered == 1U);
    const OpenPbr & layers = compiled.material.surface;
    BOOST_TEST(layers.baseMetalness == 1.0F);
    BOOST_TEST((layers.baseColour == glm::vec3 { 0.9F, 0.6F, 0.2F }));
    BOOST_TEST(layers.specularRoughness == 0.15F);
    BOOST_TEST(layers.specularAnisotropy == 0.4F);
    BOOST_TEST(layers.coatWeight == 0.5F);
    BOOST_TEST((layers.coatColour == glm::vec3 { 0.1F, 0.2F, 0.3F }));
    BOOST_TEST(layers.emissionLuminance == 40.0F);
    BOOST_TEST(layers.thinWalled == 1U);

    const OpenPbr defaults;
    BOOST_TEST(layers.baseWeight == defaults.baseWeight);
    BOOST_TEST(layers.specularIor == defaults.specularIor);
    BOOST_TEST(layers.coatIor == defaults.coatIor);
    BOOST_TEST(layers.fuzzRoughness == defaults.fuzzRoughness);
    BOOST_TEST(layers.thinFilmThickness == defaults.thinFilmThickness);
}

// a node graph is flattened as MaterialX flattens it, and the surface is not
BOOST_AUTO_TEST_CASE(moya_material_compile_a_node_graph_flattens_as_materialx_flattens_it_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle tiles = node(scene, "tiles", "ND_tiledimage_color3");
    setInput(scene, tiles, "file", scene::Value::string("grid.exr"));
    setInput(scene, tiles, "uvtiling", scene::Value::floats({ 2.0F, 3.0F }));
    const scene::Handle surface = node(scene, "plastic", kOpenPbr);
    setInput(scene, surface, "specular_roughness", scene::Value::number(0.2F));
    connect(scene, tiles, surface, "base_color");
    const scene::Handle held = materialOf(scene, "tiled", surface);
    apply(scene);

    const material::Library & library = material::Library::installed();
    const mx::DocumentPtr ours = mx::createDocument();
    mx::readFromXmlString(ours, material::flattened(library, scene, held));

    const mx::DocumentPtr theirs = mx::createDocument();
    mx::readFromXmlString(theirs,
        "<materialx version=\"1.39\">"
        "<tiledimage name=\"tiles\" type=\"color3\" nodedef=\"ND_tiledimage_color3\">"
        "<input name=\"file\" type=\"filename\" value=\"grid.exr\" />"
        "<input name=\"uvtiling\" type=\"vector2\" value=\"2, 3\" />"
        "</tiledimage>"
        "<open_pbr_surface name=\"plastic\" type=\"surfaceshader\" nodedef=\"ND_open_pbr_surface_surfaceshader\">"
        "<input name=\"specular_roughness\" type=\"float\" value=\"0.2\" />"
        "<input name=\"base_color\" type=\"color3\" nodename=\"tiles\" />"
        "</open_pbr_surface>"
        "<surfacematerial name=\"tiled\" type=\"material\" nodedef=\"ND_surfacematerial\">"
        "<input name=\"surfaceshader\" type=\"surfaceshader\" nodename=\"plastic\" />"
        "</surfacematerial>"
        "</materialx>");
    // MaterialX reads the libraries for itself, from the folders that define these nodes
    const mx::DocumentPtr libraries = mx::createDocument();
    mx::loadLibraries({ "stdlib", "pbrlib", "bxdf" }, mx::FileSearchPath(mx::FilePath((v3d::moya::executableDirectory() / material::Library::kDirectory).string())),
        libraries);
    theirs->setDataLibrary(libraries);
    theirs->flattenSubgraphs(mx::EMPTY_STRING, [](const mx::NodePtr & each) {
        return each->getType() != "surfaceshader" && each->getType() != "material";
    });

    BOOST_TEST(nodesOf(ours) == nodesOf(theirs), boost::test_tools::per_element());
    BOOST_TEST(!ours->getNode("tiles"));
    BOOST_TEST(ours->getNode("plastic"));
    BOOST_TEST(ours->getNodes().size() > 3U);
}

// a node the program does not compile gives its output's default, and is reported once however
// many inputs it feeds
BOOST_AUTO_TEST_CASE(moya_material_compile_a_node_not_compiled_gives_its_default_and_is_reported_once_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle image = node(scene, "grid", "ND_hextiledimage_color3");
    const scene::Handle surface = node(scene, "plastic", kOpenPbr);
    connect(scene, image, surface, "base_color");
    connect(scene, image, surface, "coat_color");
    const scene::Handle held = materialOf(scene, "gridded", surface);
    apply(scene);

    const material::Compiled compiled = compileOf(scene, held);
    BOOST_TEST(compiled.material.layered == 1U);
    BOOST_TEST((compiled.material.surface.baseColour == glm::vec3 { 0.0F }));
    BOOST_TEST((compiled.material.surface.coatColour == glm::vec3 { 0.0F }));
    BOOST_TEST(compiled.reports.size() == 1U);
    BOOST_TEST(mentioning(compiled.reports, "ND_hextiledimage_color3") == 1U);
}

// a heighttonormal node that feeds another one's height gives its default and is reported once,
// and the outer one shifts the point along each axis of its footprint and back
BOOST_AUTO_TEST_CASE(moya_material_compile_a_height_within_a_height_is_reported_test) {
    scene::Scene scene(scene::kinds());
    // each connection named for the node it feeds, as three of them feed an input named in
    const auto feed = [&scene](const scene::Handle & from, const scene::Handle & to, const std::string & input, const std::string & name) {
        const scene::Handle out = scene.create("connection", name + "." + input);
        scene.set(out, "from", scene::Value::handle(from));
        scene.set(out, "to", scene::Value::handle(to));
        scene.set(out, "input", scene::Value::string(input));
    };
    const scene::Handle inner = node(scene, "inner", "ND_heighttonormal_vector3");
    const scene::Handle tilt = node(scene, "tilt", "ND_extract_vector3");
    feed(inner, tilt, "in", "tilt");
    const scene::Handle outer = node(scene, "outer", "ND_heighttonormal_vector3");
    feed(tilt, outer, "in", "outer");
    const scene::Handle map = node(scene, "map", "ND_normalmap_float");
    feed(outer, map, "in", "map");
    const scene::Handle surface = node(scene, "plastic", kOpenPbr);
    feed(map, surface, "geometry_normal", "plastic");
    const scene::Handle held = materialOf(scene, "bumped", surface);
    apply(scene);

    const material::Compiled compiled = compileOf(scene, held);
    BOOST_TEST(compiled.reports.size() == 1U);
    BOOST_TEST(mentioning(compiled.reports, "node inner of ND_heighttonormal_vector3") == 1U);
    std::vector<std::uint32_t> shifts;
    for (const kernel::program::Instruction & instruction : compiled.instructions) {
        if (instruction.op == kernel::program::Op::Shift) {
            shifts.push_back(instruction.in[0]);
        }
    }
    BOOST_TEST(shifts == (std::vector<std::uint32_t> { 1U, 2U, 0U }), boost::test_tools::per_element());
}

// a node that names no definition is left out and reported once, and the inputs it feeds keep
// their own values
BOOST_AUTO_TEST_CASE(moya_material_compile_an_unknown_node_is_reported_once_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle unknown = node(scene, "mystery", "ND_no_such_node");
    const scene::Handle surface = node(scene, "plastic", kOpenPbr);
    setInput(scene, surface, "base_color", scene::Value::colour(glm::vec3 { 0.25F }));
    connect(scene, unknown, surface, "base_color");
    connect(scene, unknown, surface, "specular_color");
    const scene::Handle held = materialOf(scene, "unknown", surface);
    apply(scene);

    const material::Compiled compiled = compileOf(scene, held);
    BOOST_TEST(compiled.material.layered == 1U);
    BOOST_TEST((compiled.material.surface.baseColour == glm::vec3 { 0.25F }));
    BOOST_TEST((compiled.material.surface.specularColour == OpenPbr().specularColour));
    BOOST_TEST(compiled.reports.size() == 1U);
    BOOST_TEST(mentioning(compiled.reports, "ND_no_such_node") == 1U);
}

// a surface shader moya neither renders nor translates is reported, and the material takes
// OpenPBR's defaults
BOOST_AUTO_TEST_CASE(moya_material_compile_another_surface_shader_takes_openpbrs_defaults_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle surface = node(scene, "principled", "ND_disney_principled");
    const scene::Handle held = materialOf(scene, "principled", surface);
    apply(scene);

    const material::Compiled compiled = compileOf(scene, held);
    BOOST_TEST(compiled.material.layered == 1U);
    BOOST_TEST(compiled.material.program.count == 0U);
    BOOST_TEST((compiled.material.surface.baseColour == kernel::surface::OpenPbr {}.baseColour));
    BOOST_TEST(mentioning(compiled.reports, "ND_disney_principled") == 1U);
}

// an input that is not the definition's, or not of its type, is left out and reported
BOOST_AUTO_TEST_CASE(moya_material_compile_an_input_the_definition_does_not_take_is_reported_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle surface = node(scene, "plastic", kOpenPbr);
    setInput(scene, surface, "no_such_input", scene::Value::number(1.0F));
    setInput(scene, surface, "base_weight", scene::Value::string("heavy"));
    const scene::Handle held = materialOf(scene, "plastic", surface);
    apply(scene);

    const material::Compiled compiled = compileOf(scene, held);
    BOOST_TEST(compiled.material.layered == 1U);
    BOOST_TEST(compiled.material.surface.baseWeight == OpenPbr().baseWeight);
    BOOST_TEST(mentioning(compiled.reports, "no_such_input") == 1U);
    BOOST_TEST(mentioning(compiled.reports, "base_weight") == 1U);
}

// gathering the scene compiles each MaterialX material an instance names, and reports what it
// left out with the scene's other reports
BOOST_AUTO_TEST_CASE(moya_material_compile_a_gathered_instance_takes_its_compiled_material_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle surface = node(scene, "glass", kOpenPbr);
    setInput(scene, surface, "transmission_weight", scene::Value::number(1.0F));
    connect(scene, node(scene, "mystery", "ND_no_such_node"), surface, "base_color");
    const scene::Handle held = materialOf(scene, "glass", surface);
    const scene::Handle mesh = scene.create("mesh", "square");
    scene.set(mesh, "P", scene::Value::points({ { -1.0F, -1.0F, 0.0F }, { 1.0F, -1.0F, 0.0F }, { 1.0F, 1.0F, 0.0F }, { -1.0F, 1.0F, 0.0F } }));
    scene.set(mesh, "vertices", scene::Value::integers({ 4 }));
    scene.set(mesh, "indices", scene::Value::integers({ 0, 1, 2, 3 }));
    const scene::Handle placed = scene.create("instance", "square");
    scene.set(placed, "prototype", scene::Value::handle(mesh));
    scene.set(placed, "material", scene::Value::handle(held));
    apply(scene);

    const v3d::moya::geometry::Prototypes prototypes = v3d::moya::geometry::gather(scene, {});
    BOOST_TEST_REQUIRE(prototypes.materials.size() == 1U);
    BOOST_TEST(prototypes.materials.front().layered == 1U);
    BOOST_TEST(prototypes.materials.front().surface.transmissionWeight == 1.0F);
    BOOST_TEST(mentioning(prototypes.reports, "ND_no_such_node") == 1U);
}

/** An RI surface of a shader, with parameters as the scene client gives them, and its material. **/
scene::Handle riMaterial(scene::Scene & scene, const std::string & shader, const std::vector<std::pair<std::string, scene::Value>> & parameters) {
    const scene::Handle surface = scene.create("shader", shader);
    scene.set(surface, "shader", scene::Value::string(shader));
    scene.set(surface, "language", scene::Value::string("ri"));
    for (const auto & [name, value] : parameters) {
        setInput(scene, surface, name, value);
    }
    return materialOf(scene, shader, surface);
}

/** RI's roughness as OpenPBR's: the root of the Beckmann alpha of an exponent 1 / roughness. **/
float openPbrRoughness(float roughness) {
    return std::pow((2.0F * roughness) / (1.0F + (2.0F * roughness)), 0.25F);
}

// each of RI's built-in surfaces compiles onto OpenPBR, with its parameters as their definitions
// read them and nothing reported
BOOST_AUTO_TEST_CASE(moya_material_compile_ris_built_in_surfaces_compile_onto_openpbr_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle matte = riMaterial(scene, "matte", { { "Kd", scene::Value::floats({ 0.8F }) }, { "Os", scene::Value::colour(glm::vec3 { 0.5F }) } });
    const scene::Handle plastic = riMaterial(scene, "plastic", { { "Ks", scene::Value::floats({ 0.7F }) }, { "Ka", scene::Value::floats({ 0.2F }) } });
    const scene::Handle metal = riMaterial(scene, "metal", { { "roughness", scene::Value::floats({ 0.2F }) } });
    const scene::Handle shiny = riMaterial(scene, "shinymetal", {});
    const scene::Handle glass = riMaterial(scene, "glass", { { "eta", scene::Value::floats({ 1.33F }) }, { "Kt", scene::Value::floats({ 0.9F }) } });
    const scene::Handle constant = riMaterial(scene, "constant", { { "Cs", scene::Value::colour(glm::vec3 { 1.0F, 0.5F, 0.0F }) } });
    apply(scene);

    const auto layersOf = [&scene](const scene::Handle & held) {
        const material::Compiled compiled = compileOf(scene, held);
        BOOST_TEST(compiled.reports.empty(), scene.name(held) << " reports " << (compiled.reports.empty() ? std::string() : compiled.reports.front()));
        BOOST_TEST(compiled.material.layered == 1U);
        return compiled.material.surface;
    };
    const OpenPbr matted = layersOf(matte);
    BOOST_TEST(matted.baseWeight == 0.8F);
    BOOST_TEST(matted.specularWeight == 0.0F);
    BOOST_TEST(matted.geometryOpacity == 0.5F, boost::test_tools::tolerance(1.0e-5F));

    const OpenPbr plastics = layersOf(plastic);
    BOOST_TEST(plastics.baseWeight == 0.5F);
    BOOST_TEST(plastics.specularWeight == 0.7F);
    BOOST_TEST(plastics.specularRoughness == openPbrRoughness(0.1F), boost::test_tools::tolerance(1.0e-5F));
    BOOST_TEST(plastics.geometryOpacity == 1.0F, boost::test_tools::tolerance(1.0e-5F));

    const OpenPbr metals = layersOf(metal);
    BOOST_TEST(metals.baseMetalness == 1.0F);
    BOOST_TEST(metals.baseWeight == 1.0F);
    BOOST_TEST(metals.specularRoughness == openPbrRoughness(0.2F), boost::test_tools::tolerance(1.0e-5F));

    // a mirror of the same weight as the glossy lobe halves the roughness
    const OpenPbr shinier = layersOf(shiny);
    BOOST_TEST(shinier.baseMetalness == 1.0F);
    BOOST_TEST(shinier.baseWeight == 1.0F);
    BOOST_TEST(shinier.specularRoughness == 0.5F * openPbrRoughness(0.1F), boost::test_tools::tolerance(1.0e-5F));

    const OpenPbr glassy = layersOf(glass);
    BOOST_TEST(glassy.transmissionWeight == 0.9F);
    BOOST_TEST(glassy.specularIor == 1.33F);
    BOOST_TEST(glassy.specularRoughness == 0.0F);
    BOOST_TEST(glassy.specularWeight == 1.0F);

    const OpenPbr emitting = layersOf(constant);
    BOOST_TEST(emitting.emissionLuminance == 1.0F);
    BOOST_TEST((emitting.emissionColour == glm::vec3 { 1.0F, 0.5F, 0.0F }));
    BOOST_TEST(emitting.baseWeight == 0.0F);
    BOOST_TEST(emitting.specularWeight == 0.0F);
}

// RI's default surface is RI's matte, and an RI shader moya does not define is reported once and
// shades as RI's matte, without a report for each of its own parameters
BOOST_AUTO_TEST_CASE(moya_material_compile_an_undefined_ri_surface_shades_as_matte_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle fallback = riMaterial(scene, "defaultsurface", {});
    const scene::Handle bumpy = riMaterial(scene, "bumpy", { { "amplitude", scene::Value::floats({ 2.0F }) } });
    apply(scene);

    const material::Compiled defaulted = compileOf(scene, fallback);
    BOOST_TEST(defaulted.reports.empty());
    BOOST_TEST(defaulted.material.layered == 1U);
    BOOST_TEST(defaulted.material.surface.specularWeight == 0.0F);

    const material::Compiled standIn = compileOf(scene, bumpy);
    BOOST_TEST(standIn.reports.size() == 1U);
    BOOST_TEST(mentioning(standIn.reports, "shades as RI's matte") == 1U);
    BOOST_TEST(standIn.material.layered == 1U);
    BOOST_TEST(standIn.material.surface.specularWeight == 0.0F);
}

/**
 * A MaterialX document of an RI shader of one parameter, Kd, whose translation onto OpenPBR gives
 * a base of half its Kd and no specular layer.
 **/
std::string halvingShader(const std::string & shader) {
    const std::string definition = "ND_ri_" + shader;
    return "<?xml version=\"1.0\"?>\n<materialx version=\"1.39\">\n"
           "  <nodedef name=\"" + definition + "_surfaceshader\" node=\"ri_" + shader + "\" nodegroup=\"pbr\">\n"
           "    <input name=\"Kd\" type=\"float\" value=\"1\" />\n    <output name=\"out\" type=\"surfaceshader\" />\n  </nodedef>\n"
           "  <nodedef name=\"" + definition + "_to_open_pbr_surface\" node=\"ri_" + shader + "_to_open_pbr_surface\" nodegroup=\"translation\">\n"
           "    <input name=\"Kd\" type=\"float\" value=\"1\" />\n    <output name=\"base_weight_out\" type=\"float\" />\n"
           "    <output name=\"specular_weight_out\" type=\"float\" />\n  </nodedef>\n"
           "  <nodegraph name=\"NG_ri_" + shader + "_to_open_pbr_surface\" nodedef=\"" + definition + "_to_open_pbr_surface\">\n"
           "    <multiply name=\"halved\" type=\"float\">\n      <input name=\"in1\" type=\"float\" interfacename=\"Kd\" />\n"
           "      <input name=\"in2\" type=\"float\" value=\"0.5\" />\n    </multiply>\n"
           "    <dot name=\"none\" type=\"float\">\n      <input name=\"in\" type=\"float\" value=\"0\" />\n    </dot>\n"
           "    <output name=\"base_weight_out\" type=\"float\" nodename=\"halved\" />\n"
           "    <output name=\"specular_weight_out\" type=\"float\" nodename=\"none\" />\n  </nodegraph>\n</materialx>\n";
}

// a document of an RI shader's name on the shader search path defines it, in place of moya's own
// definition of a built-in, or as a shader moya does not define. Without a search path, the
// built-in is moya's
BOOST_AUTO_TEST_CASE(moya_material_compile_a_shader_document_on_the_search_path_defines_it_test) {
    const boost::filesystem::path directory = boost::filesystem::temp_directory_path() / "v3d_ri_shaders";
    boost::filesystem::create_directories(directory);
    for (const char * shader : { "plastic", "velvet" }) {
        std::ofstream((directory / (std::string(shader) + ".mtlx")).string(), std::ios::binary) << halvingShader(shader);
    }
    const auto layersOf = [&directory](const std::string & shader, bool searched) {
        scene::Scene scene(scene::kinds());
        const scene::Handle options = scene.create("options", "options");
        if (searched) {
            scene.set(options, "shaderSearchPath", scene::Value::string("nowhere;" + directory.string()));
        }
        const scene::Handle held = riMaterial(scene, shader, { { "Kd", scene::Value::floats({ 0.8F }) } });
        apply(scene);
        const material::Compiled compiled = compileOf(scene, held);
        BOOST_TEST(compiled.reports.empty(), shader << " reports " << (compiled.reports.empty() ? std::string() : compiled.reports.front()));
        return compiled.material.surface;
    };
    BOOST_TEST(layersOf("plastic", true).baseWeight == 0.4F, boost::test_tools::tolerance(1.0e-6F));
    BOOST_TEST(layersOf("plastic", true).specularWeight == 0.0F);
    BOOST_TEST(layersOf("velvet", true).baseWeight == 0.4F, boost::test_tools::tolerance(1.0e-6F));
    BOOST_TEST(layersOf("plastic", false).baseWeight == 0.8F);
    BOOST_TEST(layersOf("plastic", false).specularWeight == 0.5F);
    boost::system::error_code ignored;
    boost::filesystem::remove_all(directory, ignored);
}

};  // namespace
