/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <api/log/Logger.h>
#include <api/rib/Reader.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/kernel/surface/Material.h>
#include <moya/libmoya/ri/SceneClient.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Kinds.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>

#include <fstream>
#include <ios>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>


#include "Heard.h"

namespace {

namespace scene = v3d::moya::scene;

/** A scene read from a world of RIB requests through the client, and the warnings it heard. **/
class Read final {
 public:
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the requests, then what to listen for
    Read(const std::string & requests, const std::string & listen = {}) : scene_(scene::kinds()), logger_(boost::make_shared<v3d::log::Logger>()) {
        const v3d::moya::handling::Heard heard(logger_->get());
        v3d::moya::ri::SceneClient client(scene_, logger_);
        client.frame([this]() {
            scene_.apply();
        });
        std::istringstream stream("WorldBegin\n" + requests + "WorldEnd\n");
        v3d::rib::Reader reader(logger_);
        BOOST_REQUIRE(reader.read(stream, &client));
        heard_ = !listen.empty() && heard.said(listen);
    }

    const scene::Scene & scene() const {
        return scene_;
    }

    bool heard() const {
        return heard_;
    }

    /** The surface node of the material the only instance names. **/
    scene::Handle surface() const {
        const std::vector<scene::Handle> instances = scene_.objects("instance");
        BOOST_TEST_REQUIRE(instances.size() == 1U);
        const scene::Handle material = scene_.value(instances.front(), "material").handleArray().front();
        return scene_.value(material, "surface").handleArray().front();
    }

    std::string text(const scene::Handle & object, const std::string & parameter) const {
        return scene_.value(object, parameter).stringArray().front();
    }

    /** The connection that feeds an input of a node, or none. **/
    std::optional<scene::Handle> feeding(const scene::Handle & node, const std::string & input) const {
        for (const scene::Handle & connection : scene_.objects("connection")) {
            if (scene_.value(connection, "to").handleArray().front() == node && text(connection, "input") == input) {
                return connection;
            }
        }
        return std::nullopt;
    }

 private:
    scene::Scene scene_;
    boost::shared_ptr<v3d::log::Logger> logger_;
    bool heard_ { false };
};

// a Bxdf makes the material's surface node, of the definition its type names, and a Pattern it
// references feeds the input the reference names, from the output the reference names
BOOST_AUTO_TEST_CASE(moya_scenematerials_a_bxdf_and_a_pattern_make_a_network_test) {
    const Read read("Pattern \"ND_constant_color3\" \"tint\" \"color value\" [0.9 0.6 0.2]\n"
                    "Bxdf \"ND_open_pbr_surface_surfaceshader\" \"gold\" \"reference color base_color\" [\"tint:out\"] \"float base_metalness\" [1]\n"
                    "Sphere 1 -1 1 360\n");
    const scene::Handle surface = read.surface();
    BOOST_TEST(read.text(surface, "language") == "materialx");
    BOOST_TEST(read.text(surface, "shader") == "ND_open_pbr_surface_surfaceshader");
    BOOST_TEST(read.scene().value(surface, "base_metalness").floatArray().front() == 1.0F);
    const std::optional<scene::Handle> connection = read.feeding(surface, "base_color");
    BOOST_TEST_REQUIRE(connection.has_value());
    BOOST_TEST(read.text(*connection, "output") == "out");
    const scene::Handle tint = read.scene().value(*connection, "from").handleArray().front();
    BOOST_TEST(read.text(tint, "shader") == "ND_constant_color3");
    BOOST_TEST((read.scene().value(tint, "value").floatArray() == std::vector<float> { 0.9F, 0.6F, 0.2F }));

    // the network renders: the gathered material is OpenPBR's, a metal
    const v3d::moya::geometry::Prototypes prototypes = v3d::moya::geometry::gather(read.scene(), {});
    BOOST_TEST_REQUIRE(prototypes.materials.size() == 1U);
    BOOST_TEST(prototypes.materials.front().layered == 1U);
    BOOST_TEST(prototypes.materials.front().surface.baseMetalness == 1.0F);
}

// a reference to a handle no request before it has is heard, and its input keeps its value
BOOST_AUTO_TEST_CASE(moya_scenematerials_a_reference_to_no_node_is_heard_test) {
    const Read read("Bxdf \"ND_open_pbr_surface_surfaceshader\" \"surface\" \"reference color base_color\" [\"missing:out\"]\nSphere 1 -1 1 360\n", "which no Pattern");
    BOOST_TEST(read.heard());
    BOOST_TEST(!read.feeding(read.surface(), "base_color").has_value());
}

// a Surface after a Bxdf shades what follows it, and a Bxdf after a Surface does
BOOST_AUTO_TEST_CASE(moya_scenematerials_the_later_of_surface_and_bxdf_shades_test) {
    const Read surfaced("Bxdf \"ND_open_pbr_surface_surfaceshader\" \"surface\"\nSurface \"plastic\"\nSphere 1 -1 1 360\n");
    BOOST_TEST(surfaced.text(surfaced.surface(), "language") == "ri");
    BOOST_TEST(surfaced.text(surfaced.surface(), "shader") == "plastic");
    const Read networked("Surface \"plastic\"\nBxdf \"ND_open_pbr_surface_surfaceshader\" \"surface\"\nSphere 1 -1 1 360\n");
    BOOST_TEST(networked.text(networked.surface(), "language") == "materialx");
}

// a Bxdf of type materialx gives the scene the nodes of the document's material, through its node
// graph, and its other parameters override the surface node's inputs
BOOST_AUTO_TEST_CASE(moya_scenematerials_a_materialx_bxdf_reads_a_document_test) {
    const boost::filesystem::path file = boost::filesystem::temp_directory_path() / "v3d_scene_materials.mtlx";
    {
        std::ofstream stream(file.string(), std::ios::binary);
        stream << "<?xml version=\"1.0\"?>\n<materialx version=\"1.39\">\n"
                  "  <nodegraph name=\"pattern\">\n"
                  "    <constant name=\"tint\" type=\"color3\"><input name=\"value\" type=\"color3\" value=\"0.2, 0.4, 0.6\" /></constant>\n"
                  "    <output name=\"colour\" type=\"color3\" nodename=\"tint\" />\n"
                  "  </nodegraph>\n"
                  "  <open_pbr_surface name=\"shading\" type=\"surfaceshader\">\n"
                  "    <input name=\"base_color\" type=\"color3\" nodegraph=\"pattern\" output=\"colour\" />\n"
                  "    <input name=\"specular_roughness\" type=\"float\" value=\"0.1\" />\n"
                  "  </open_pbr_surface>\n"
                  "  <surfacematerial name=\"other\" type=\"material\"><input name=\"surfaceshader\" type=\"surfaceshader\" nodename=\"shading\" /></surfacematerial>\n"
                  "  <surfacematerial name=\"painted\" type=\"material\"><input name=\"surfaceshader\" type=\"surfaceshader\" nodename=\"shading\" /></surfacematerial>\n"
                  "</materialx>\n";
    }
    const Read read("Bxdf \"materialx\" \"painted\" \"string file\" [\"" + file.generic_string() +
        "\"] \"string material\" [\"painted\"] \"float specular_roughness\" [0.7]\nSphere 1 -1 1 360\n");
    boost::system::error_code ignored;
    boost::filesystem::remove(file, ignored);
    const scene::Handle surface = read.surface();
    BOOST_TEST(read.text(surface, "shader") == "ND_open_pbr_surface_surfaceshader");
    BOOST_TEST(read.scene().value(surface, "specular_roughness").floatArray().front() == 0.7F);
    const std::optional<scene::Handle> connection = read.feeding(surface, "base_color");
    BOOST_TEST_REQUIRE(connection.has_value());
    const scene::Handle tint = read.scene().value(*connection, "from").handleArray().front();
    BOOST_TEST(read.text(tint, "shader") == "ND_constant_color3");
    BOOST_TEST((read.scene().value(tint, "value").floatArray() == std::vector<float> { 0.2F, 0.4F, 0.6F }));
}

// a document's material with a displacement shader gives the material that displacement, in
// place of the attribute state's Displacement, and one without keeps it
BOOST_AUTO_TEST_CASE(moya_scenematerials_a_document_gives_its_displacement_test) {
    const Read displaced("Displacement \"bumpy\"\nBxdf \"materialx\" \"rough\" \"string file\" [\"data/materials-patterns.mtlx\"] \"string material\" [\"rough\"]\n"
                         "Sphere 1 -1 1 360\n");
    const std::vector<scene::Handle> instances = displaced.scene().objects("instance");
    BOOST_TEST_REQUIRE(instances.size() == 1U);
    const scene::Handle material = displaced.scene().value(instances.front(), "material").handleArray().front();
    const scene::Handle displacement = displaced.scene().value(material, "displacement").handleArray().front();
    BOOST_TEST(displaced.text(displacement, "language") == "materialx");
    BOOST_TEST(displaced.text(displacement, "shader") == "ND_displacement_float");
    BOOST_TEST(displaced.feeding(displacement, "displacement").has_value());

    const Read kept("Displacement \"bumpy\"\nBxdf \"materialx\" \"bumped\" \"string file\" [\"data/materials-patterns.mtlx\"] \"string material\" [\"bumped\"]\n"
                    "Sphere 1 -1 1 360\n");
    const scene::Handle held = kept.scene().value(kept.scene().objects("instance").front(), "material").handleArray().front();
    const scene::Handle ri = kept.scene().value(held, "displacement").handleArray().front();
    BOOST_TEST(kept.text(ri, "language") == "ri");
    BOOST_TEST(kept.text(ri, "shader") == "bumpy");
}

// a document that cannot be read is heard, and the surface is RI's in force
BOOST_AUTO_TEST_CASE(moya_scenematerials_a_document_that_cannot_be_read_is_heard_test) {
    const Read read("Bxdf \"materialx\" \"none\" \"string file\" [\"no_such_document.mtlx\"]\nSphere 1 -1 1 360\n", "gives no material");
    BOOST_TEST(read.heard());
    BOOST_TEST(read.text(read.surface(), "language") == "ri");
}

};  // namespace
