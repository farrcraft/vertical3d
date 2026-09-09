/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/asset/Manager.h>
#include <api/asset/Model.h>
#include <api/asset/Type.h>
#include <api/asset/loader/Gltf.h>
#include <api/image/Compare.h>
#include <api/image/Factory.h>

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

namespace {

// three primitives, the third of them not indexed and all three sharing one material -
// api/asset/tests/data/make_model_fixture.py generates it and says why it is shaped that way
const char* FIXTURE = "three_primitives.glb";

// one triangle whose base colour texture is a bufferView holding pixel.png, which is the
// case the named-texture fixture cannot cover - api/asset/tests/data/make_embedded_fixture.py
const char* EMBEDDED = "embedded_texture.glb";

boost::shared_ptr<v3d::log::Logger> logger() {
    return boost::make_shared<v3d::log::Logger>();
}

boost::shared_ptr<v3d::asset::Model> loadAsset(const char* name) {
    v3d::asset::Manager manager("data", logger());
    boost::shared_ptr<v3d::asset::Asset> asset = manager.load(name, v3d::asset::Type::ModelGltf);
    return boost::dynamic_pointer_cast<v3d::asset::Model>(asset);
}

boost::shared_ptr<v3d::type::Model> load(const char* name) {
    v3d::asset::Manager manager("data", logger());
    boost::shared_ptr<v3d::asset::Asset> asset = manager.load(name, v3d::asset::Type::ModelGltf);
    if (!asset) {
        return boost::shared_ptr<v3d::type::Model>();
    }
    boost::shared_ptr<v3d::asset::Model> model = boost::dynamic_pointer_cast<v3d::asset::Model>(asset);
    return model ? model->model() : boost::shared_ptr<v3d::type::Model>();
}

};  // namespace

BOOST_AUTO_TEST_CASE(gltf_merges_every_primitive_test) {
    boost::shared_ptr<v3d::type::Model> model = load(FIXTURE);

    BOOST_REQUIRE(model);
    // three triangles of three vertices, in one array and one index run
    BOOST_CHECK_EQUAL(model->vertices().size(), 9u);
    BOOST_CHECK_EQUAL(model->indices().size(), 9u);
}

BOOST_AUTO_TEST_CASE(gltf_rebases_the_indices_of_later_primitives_test) {
    boost::shared_ptr<v3d::type::Model> model = load(FIXTURE);
    BOOST_REQUIRE(model);

    // the first two primitives both index 0, 1, 2 in the file. Without the rebase the
    // second triangle would redraw the first one's vertices and the highest index would
    // be 2 rather than 8
    const std::vector<std::uint32_t> expected{ 0, 1, 2, 3, 4, 5, 6, 7, 8 };
    BOOST_CHECK_EQUAL(model->indices() == expected, true);
}

BOOST_AUTO_TEST_CASE(gltf_synthesises_indices_for_a_primitive_without_them_test) {
    boost::shared_ptr<v3d::type::Model> model = load(FIXTURE);
    BOOST_REQUIRE(model);

    // the third primitive is drawn straight out of its vertex array in the file. Its three
    // vertices are in the merge, and so is an index run reaching them - dropping either
    // would leave six of each
    BOOST_CHECK_EQUAL(model->vertices().size(), 9u);
    BOOST_CHECK_EQUAL(model->indices()[6], 6u);
    BOOST_CHECK_EQUAL(model->indices()[8], 8u);
}

BOOST_AUTO_TEST_CASE(gltf_every_index_addresses_a_real_vertex_test) {
    boost::shared_ptr<v3d::type::Model> model = load(FIXTURE);
    BOOST_REQUIRE(model);

    for (const std::uint32_t index : model->indices()) {
        BOOST_REQUIRE(index < model->vertices().size());
    }
}

BOOST_AUTO_TEST_CASE(gltf_reads_the_vertex_attributes_test) {
    boost::shared_ptr<v3d::type::Model> model = load(FIXTURE);
    BOOST_REQUIRE(model);

    // the three triangles are stepped along x, so the position says which primitive a
    // vertex came from and in what order they were appended
    BOOST_CHECK_CLOSE(model->vertices()[0].position.x, 0.0f, 0.01f);
    BOOST_CHECK_CLOSE(model->vertices()[3].position.x, 2.0f, 0.01f);
    BOOST_CHECK_CLOSE(model->vertices()[6].position.x, 4.0f, 0.01f);

    // every fixture normal points down +z, and the uvs are the same three per triangle
    BOOST_CHECK_CLOSE(model->vertices()[0].normal.z, 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(model->vertices()[8].normal.z, 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(model->vertices()[1].uv.x, 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(model->vertices()[2].uv.y, 1.0f, 0.01f);
}

BOOST_AUTO_TEST_CASE(gltf_reads_the_base_colour_test) {
    boost::shared_ptr<v3d::type::Model> model = load(FIXTURE);
    BOOST_REQUIRE(model);

    BOOST_CHECK_CLOSE(model->material().baseColour.r, 0.25f, 0.01f);
    BOOST_CHECK_CLOSE(model->material().baseColour.g, 0.5f, 0.01f);
    BOOST_CHECK_CLOSE(model->material().baseColour.b, 0.75f, 0.01f);
    BOOST_CHECK_CLOSE(model->material().baseColour.a, 1.0f, 0.01f);
}

BOOST_AUTO_TEST_CASE(gltf_names_the_texture_rather_than_decoding_it_test) {
    boost::shared_ptr<v3d::type::Model> model = load(FIXTURE);
    BOOST_REQUIRE(model);

    // the material names its image and the app resolves it, which is ADR-0020's shape. The
    // fixture ships no albedo.png at all, and loading it still succeeds
    BOOST_CHECK_EQUAL(model->material().baseColourTexture, "albedo.png");
}

/**
 * An image the file carries has no name to hand over, so it arrives decoded - and decoded
 * to exactly what the png reader makes of the same bytes on disk.
 **/
BOOST_AUTO_TEST_CASE(gltf_decodes_a_texture_the_file_carries_test) {
    boost::shared_ptr<v3d::asset::Model> asset = loadAsset(EMBEDDED);
    BOOST_REQUIRE(asset);
    BOOST_REQUIRE(asset->model());

    // there is no name, because there is no file to name
    BOOST_CHECK(asset->model()->material().baseColourTexture.empty());

    boost::shared_ptr<v3d::image::Image> embedded = asset->baseColourImage();
    BOOST_REQUIRE(embedded);

    v3d::image::Factory factory(logger());
    boost::shared_ptr<v3d::image::Image> onDisk = factory.read("data/pixel.png");
    BOOST_REQUIRE(onDisk);

    const v3d::image::Difference difference = v3d::image::compare(*embedded, *onDisk, 0);
    BOOST_CHECK_MESSAGE(difference.match, difference.description());
}

/**
 * And a model whose texture was named carries no pixels, so an app can tell the two apart
 * by asking rather than by knowing which packaging it loaded.
 **/
BOOST_AUTO_TEST_CASE(gltf_a_named_texture_carries_no_pixels_test) {
    boost::shared_ptr<v3d::asset::Model> asset = loadAsset(FIXTURE);
    BOOST_REQUIRE(asset);

    BOOST_CHECK(!asset->baseColourImage());
}

BOOST_AUTO_TEST_CASE(gltf_a_missing_file_is_no_asset_test) {
    // a loader that failed hands back nothing rather than an asset holding nothing, which
    // is indistinguishable from a loaded one until a consumer dereferences it
    BOOST_CHECK_EQUAL(static_cast<bool>(load("does_not_exist.glb")), false);
}

BOOST_AUTO_TEST_CASE(gltf_a_file_that_is_not_gltf_is_no_asset_test) {
    BOOST_CHECK_EQUAL(static_cast<bool>(load("plain.txt")), false);
}

BOOST_AUTO_TEST_CASE(gltf_resolves_by_extension_test) {
    v3d::asset::Manager manager("data", logger());

    // .glb and .gltf both reach the model loader, so an app naming a file gets one without
    // naming the type
    boost::shared_ptr<v3d::asset::Asset> asset = manager.loadTypeFromExt(FIXTURE);
    BOOST_CHECK_EQUAL(static_cast<bool>(boost::dynamic_pointer_cast<v3d::asset::Model>(asset)), true);
}

BOOST_AUTO_TEST_CASE(gltf_vertex_bytes_is_the_array_size_test) {
    boost::shared_ptr<v3d::type::Model> model = load(FIXTURE);
    BOOST_REQUIRE(model);

    // what a device buffer is made from: eight floats a vertex, and the loader fills them
    // all whether or not the file carried normals and uvs
    BOOST_CHECK_EQUAL(model->vertexBytes(), model->vertices().size() * 8u * sizeof(float));
    BOOST_CHECK_EQUAL(model->empty(), false);
}
