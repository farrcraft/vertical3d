/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/asset/Manager.h>
#include <api/asset/media/Loaders.h>
#include <api/render/realtime/DrawItem.h>
#include <api/render/realtime/Frame.h>
#include <api/render/realtime/MeshRegistry.h>
#include <api/render/realtime/Pass.h>
#include <api/render/realtime/vulkan/frame/Recorder.h>
#include <api/render/realtime/vulkan/frame/RenderTarget.h>
#include <api/render/realtime/vulkan/pipeline/Builder.h>
#include <api/type/Model.h>

#include <stdexcept>
#include <string>

#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "Headless.h"

using v3d::render::realtime::DrawItem;
using v3d::render::realtime::Frame;
using v3d::render::realtime::MeshHandle;
using v3d::render::realtime::MeshRegistry;
using v3d::render::realtime::Pass;
using v3d::render::realtime::PipelineHandle;
using v3d::render::realtime::vulkan::frame::Recorder;
using v3d::render::realtime::vulkan::frame::RenderTarget;
using v3d::render::realtime::vulkan::pipeline::Builder;

namespace {

const VkFormat colourFormat = VK_FORMAT_R8G8B8A8_UNORM;
const uint32_t width = 16;
const uint32_t height = 16;

const uint32_t vertexShader[] =
#include "shaders/depth.vert.inc"
;  // NOLINT(whitespace/semicolon) - the initialiser it terminates is the include above

/**
 * A registry loading from the asset suite's fixtures, which the build names rather than
 * copying - see V3D_ASSET_FIXTURES.
 **/
boost::shared_ptr<MeshRegistry> registry(v3d::test::Headless* headless) {
    const boost::shared_ptr<v3d::asset::Manager> assets = boost::make_shared<v3d::asset::Manager>(V3D_ASSET_FIXTURES, headless->logger);
    v3d::asset::media::registerLoaders(*assets, headless->logger);
    return boost::make_shared<MeshRegistry>(headless->logger, headless->context, assets);
}

/**
 * Two triangles in code, a part each, naming a texture apiece - empty for none.
 **/
v3d::type::Model twoParts(const std::string& first, const std::string& second) {
    v3d::type::Model model;
    const glm::vec3 corners[6] = {{0, 0, 0.5f}, {1, 0, 0.5f}, {0, 1, 0.5f}, {1, 0, 0.5f}, {1, 1, 0.5f}, {0, 1, 0.5f}};
    for (const glm::vec3& corner : corners) {
        v3d::type::Model::Vertex vertex;
        vertex.position = corner;
        model.vertices().push_back(vertex);
    }
    model.indices() = {0, 1, 2, 3, 4, 5};
    v3d::type::Model::Material material;
    material.baseColourTexture = first;
    model.materials().push_back(material);
    material.baseColourTexture = second;
    model.materials().push_back(material);
    model.parts().push_back({0, 3, 0});
    model.parts().push_back({3, 3, 1});
    return model;
}

/**
 * One triangle in code, naming a texture beside the fixtures.
 **/
v3d::type::Model triangle(const std::string& texture) {
    v3d::type::Model model;
    model.vertices().resize(3);
    model.vertices()[0].position = glm::vec3(-0.5f, -0.5f, 0.5f);
    model.vertices()[1].position = glm::vec3(0.5f, -0.5f, 0.5f);
    model.vertices()[2].position = glm::vec3(0.0f, 0.5f, 0.5f);
    model.indices() = {0, 1, 2};
    v3d::type::Model::Material material;
    material.baseColourTexture = texture;
    model.materials().push_back(material);
    model.parts().push_back({0, 3, 0});
    return model;
}

};  // namespace

BOOST_AUTO_TEST_SUITE(mesh_registry_test)

/**
 * A path asked for twice is one upload and one handle - which is what lets a hundred props
 * drawn from one file cost one mesh.
 **/
BOOST_AUTO_TEST_CASE(a_path_loaded_twice_is_one_upload) {
    v3d::test::Headless headless(colourFormat, width, height);
    const boost::shared_ptr<MeshRegistry> meshes = registry(&headless);

    const MeshHandle first = meshes->load("three_primitives.glb");
    const MeshHandle second = meshes->load("three_primitives.glb");

    BOOST_CHECK(first == second);
    BOOST_CHECK_EQUAL(meshes->count(), 1U);
    const MeshRegistry::Entry* entry = meshes->resolve(first);
    BOOST_REQUIRE(entry != nullptr);
    BOOST_CHECK(entry->mesh->indexCount() > 0);
    BOOST_REQUIRE_EQUAL(entry->parts.size(), 1U);
    // the fixture names an albedo.png that is not beside it, so it is drawn white
    BOOST_CHECK(entry->parts[0].texture == headless.context->quads()->white());
    BOOST_CHECK(headless.silent());
}

/**
 * An image a .glb carries in its own buffer is uploaded as the entry's albedo.
 **/
BOOST_AUTO_TEST_CASE(an_embedded_albedo_is_uploaded) {
    v3d::test::Headless headless(colourFormat, width, height);
    const boost::shared_ptr<MeshRegistry> meshes = registry(&headless);

    const MeshRegistry::Entry* entry = meshes->resolve(meshes->load("embedded_texture.glb"));
    BOOST_REQUIRE(entry != nullptr);
    BOOST_REQUIRE_EQUAL(entry->parts.size(), 1U);
    BOOST_CHECK(entry->parts[0].texture != headless.context->quads()->white());
    BOOST_CHECK(headless.context->resources()->material(entry->parts[0].material) != nullptr);
    BOOST_CHECK(headless.silent());
}

/**
 * Two models naming one image share one texture and one material, and the albedo outlives
 * the first of them to be released.
 **/
BOOST_AUTO_TEST_CASE(two_models_naming_one_image_share_its_material) {
    v3d::test::Headless headless(colourFormat, width, height);
    const boost::shared_ptr<MeshRegistry> meshes = registry(&headless);

    const MeshHandle crate = meshes->add("crate", triangle("pixel.png"));
    const MeshHandle barrel = meshes->add("barrel", triangle("pixel.png"));
    BOOST_REQUIRE(crate != barrel);

    const MeshRegistry::Part crateEntry = meshes->resolve(crate)->parts.at(0);
    const MeshRegistry::Part barrelEntry = meshes->resolve(barrel)->parts.at(0);
    BOOST_CHECK(crateEntry.texture != headless.context->quads()->white());
    BOOST_CHECK(crateEntry.texture == barrelEntry.texture);
    BOOST_CHECK(crateEntry.material == barrelEntry.material);

    BOOST_CHECK(meshes->release(crate));
    BOOST_CHECK(meshes->resolve(crate) == nullptr);
    BOOST_CHECK(headless.context->resources()->material(barrelEntry.material) != nullptr);

    BOOST_CHECK(meshes->release(barrel));
    BOOST_CHECK(headless.context->resources()->material(barrelEntry.material) == nullptr);
    BOOST_CHECK(headless.context->resources()->texture(barrelEntry.texture) == nullptr);
    BOOST_CHECK(headless.silent());
}

/**
 * A file whose surfaces differ is one entry in parts, each with its own range and base colour,
 * in the order the model holds them - ADR-0069.
 **/
BOOST_AUTO_TEST_CASE(a_file_in_parts_is_an_entry_in_parts) {
    v3d::test::Headless headless(colourFormat, width, height);
    const boost::shared_ptr<MeshRegistry> meshes = registry(&headless);

    const MeshRegistry::Entry* entry = meshes->resolve(meshes->load("two_surfaces.glb"));
    BOOST_REQUIRE(entry != nullptr);
    BOOST_REQUIRE_EQUAL(entry->parts.size(), 2U);
    BOOST_CHECK_EQUAL(entry->parts[0].firstIndex, 0U);
    BOOST_CHECK_EQUAL(entry->parts[0].indexCount, 6U);
    BOOST_CHECK(entry->parts[0].baseColour == glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
    BOOST_CHECK_EQUAL(entry->parts[1].firstIndex, 6U);
    BOOST_CHECK_EQUAL(entry->parts[1].indexCount, 3U);
    BOOST_CHECK(entry->parts[1].baseColour == glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
    BOOST_CHECK(headless.silent());
}

/**
 * Each part has its own albedo: one part naming an image draws it while the other draws white,
 * and releasing the entry releases the image.
 **/
BOOST_AUTO_TEST_CASE(each_part_has_its_own_albedo) {
    v3d::test::Headless headless(colourFormat, width, height);
    const boost::shared_ptr<MeshRegistry> meshes = registry(&headless);

    const MeshHandle handle = meshes->add("half textured", twoParts("pixel.png", ""));
    const MeshRegistry::Entry entry = *meshes->resolve(handle);
    BOOST_REQUIRE_EQUAL(entry.parts.size(), 2U);
    BOOST_CHECK(entry.parts[0].texture != headless.context->quads()->white());
    BOOST_CHECK(entry.parts[1].texture == headless.context->quads()->white());

    BOOST_CHECK(meshes->release(handle));
    BOOST_CHECK(headless.context->resources()->texture(entry.parts[0].texture) == nullptr);
    // the white texture is the quads' own, and no part releases it
    BOOST_CHECK(headless.context->resources()->texture(entry.parts[1].texture) != nullptr);
    BOOST_CHECK(headless.silent());
}

/**
 * Two parts naming one image share it, and it is released once, when the entry is - not when
 * the first of the two parts is counted off.
 **/
BOOST_AUTO_TEST_CASE(two_parts_naming_one_image_share_it) {
    v3d::test::Headless headless(colourFormat, width, height);
    const boost::shared_ptr<MeshRegistry> meshes = registry(&headless);

    const MeshHandle twice = meshes->add("twice", twoParts("pixel.png", "pixel.png"));
    const MeshHandle once = meshes->add("once", triangle("pixel.png"));
    const MeshRegistry::Entry entry = *meshes->resolve(twice);
    BOOST_REQUIRE_EQUAL(entry.parts.size(), 2U);
    BOOST_CHECK(entry.parts[0].texture == entry.parts[1].texture);
    BOOST_CHECK(entry.parts[0].material == entry.parts[1].material);

    BOOST_CHECK(meshes->release(twice));
    BOOST_CHECK(headless.context->resources()->texture(entry.parts[0].texture) != nullptr);
    BOOST_CHECK(meshes->release(once));
    BOOST_CHECK(headless.context->resources()->texture(entry.parts[0].texture) == nullptr);
    BOOST_CHECK(headless.silent());
}

/**
 * A part outside the model's indices or its materials is refused, rather than drawing another
 * part's triangles.
 **/
BOOST_AUTO_TEST_CASE(a_part_outside_the_model_is_refused) {
    v3d::test::Headless headless(colourFormat, width, height);
    const boost::shared_ptr<MeshRegistry> meshes = registry(&headless);

    v3d::type::Model past = twoParts("", "");
    past.parts()[1].indexCount = 4;
    BOOST_CHECK_THROW(meshes->add("past the indices", past), std::runtime_error);

    v3d::type::Model unknown = twoParts("", "");
    unknown.parts()[1].material = 2;
    BOOST_CHECK_THROW(meshes->add("an unknown material", unknown), std::runtime_error);

    v3d::type::Model none = twoParts("", "");
    none.parts().clear();
    BOOST_CHECK_THROW(meshes->add("no parts", none), std::runtime_error);

    BOOST_CHECK_EQUAL(meshes->count(), 0U);
    BOOST_CHECK(headless.silent());
}

/**
 * A mesh released while the frame drawing it is still in flight is not destroyed under that
 * frame - the same rule ADR-0061 gives a texture. A silent log is the assertion.
 **/
BOOST_AUTO_TEST_CASE(a_mesh_released_in_flight_outlives_its_frame) {
    v3d::test::Headless headless(colourFormat, width, height);
    const boost::shared_ptr<MeshRegistry> meshes = registry(&headless);
    const MeshHandle handle = meshes->add("triangle", triangle(""));

    boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(),
        width, height, VK_FORMAT_UNDEFINED, true, true);

    // a depth-only draw of the model's positions, which is all the stride needs to agree on
    Builder builder(headless.device);
    builder.name("mesh-depth")
        .shader(VK_SHADER_STAGE_VERTEX_BIT, vertexShader, sizeof(vertexShader))
        .vertexBinding(0, sizeof(v3d::type::Model::Vertex))
        .vertexAttribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0)
        .cull(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
        .depth(true, true)
        .depthFormat(target->depthFormat())
        .colourFormats({});
    const PipelineHandle pipeline = headless.context->resources()->add(builder.build(headless.context->pipelineCache()));

    Frame frame;
    boost::shared_ptr<Pass> pass = frame.pass("depth");
    pass->target(target);
    pass->depth(true);
    DrawItem item;
    item.pipeline = pipeline;
    meshes->resolve(handle)->mesh->describe(&item);
    pass->submit(item);

    VkCommandBuffer commands = headless.context->ring()->begin();
    Recorder::record(commands, frame, Recorder::Target(), *headless.context->resources());
    headless.submit(commands);

    BOOST_CHECK(meshes->release(handle));
    for (uint32_t turn = 0; turn <= headless.context->ring()->framesInFlight(); turn++) {
        headless.submit(headless.context->ring()->begin());
    }
    headless.context->ring()->waitIdle();

    BOOST_CHECK(headless.silent());
}

BOOST_AUTO_TEST_SUITE_END()
