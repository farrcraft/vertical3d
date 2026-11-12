/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/material/Library.h>

#include <stdexcept>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/test/unit_test.hpp>

namespace {

namespace material = v3d::moya::material;

constexpr const char * kOpenPbr = "ND_open_pbr_surface_surfaceshader";

BOOST_AUTO_TEST_CASE(moya_material_library_the_libraries_beside_the_suite_define_the_standard_nodes_test) {
    const material::Library library;
    BOOST_TEST(library.defines("ND_add_float"));
    BOOST_TEST(library.defines("ND_image_color3"));
    BOOST_TEST(library.defines("ND_dielectric_bsdf"));
    BOOST_TEST(library.defines(kOpenPbr));
    BOOST_TEST(!library.defines("ND_no_such_node"));
}

// moya renders OpenPBR Surface 1.1.1, the version MaterialX defines as the default
BOOST_AUTO_TEST_CASE(moya_material_library_open_pbr_surface_is_version_1_1_1_test) {
    BOOST_TEST(material::Library().version(kOpenPbr) == "1.1.1");
}

BOOST_AUTO_TEST_CASE(moya_material_library_open_pbr_surface_resolves_to_its_node_graph_test) {
    BOOST_TEST(material::Library().graph(kOpenPbr) == "NG_open_pbr_surface_surfaceshader");
}

// the shader generators' implementations are not installed, so a node implemented in code has no
// graph, and an unknown one has none either
BOOST_AUTO_TEST_CASE(moya_material_library_a_node_implemented_in_code_has_no_graph_test) {
    const material::Library library;
    BOOST_TEST(library.graph("ND_add_float").empty());
    BOOST_TEST(library.graph("ND_no_such_node").empty());
    BOOST_TEST(library.version("ND_no_such_node").empty());
}

BOOST_AUTO_TEST_CASE(moya_material_library_a_missing_directory_is_refused_test) {
    const boost::filesystem::path missing = boost::filesystem::temp_directory_path() / "v3d_moya_no_materialx_libraries";
    boost::filesystem::remove_all(missing);
    BOOST_CHECK_THROW(const material::Library library(missing), std::runtime_error);
}

};  // namespace
