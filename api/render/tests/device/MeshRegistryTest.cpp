/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/asset/Manager.h>
#include <api/asset/media/Loaders.h>
#include <api/image/Image.h>
#include <api/image/reader/Png.h>
#include <api/render/realtime/DrawItem.h>
#include <api/render/realtime/Frame.h>
#include <api/render/realtime/MeshRegistry.h>
#include <api/render/realtime/Pass.h>
#include <api/render/realtime/vulkan/frame/Capture.h>
#include <api/render/realtime/vulkan/frame/Recorder.h>
#include <api/render/realtime/vulkan/frame/RenderTarget.h>
#include <api/render/realtime/vulkan/pipeline/Builder.h>
#include <api/type/Model.h>

#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

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
using v3d::render::realtime::vulkan::frame::Capture;
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

const uint32_t coverShader[] =
#include "shaders/scene.vert.inc"
;  // NOLINT(whitespace/semicolon)

const uint32_t albedoShader[] =
#include "shaders/albedo.frag.inc"
;  // NOLINT(whitespace/semicolon)

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
 * A path asked for twice is one upload and one handle, so a hundred props drawn from one file
 * cost one mesh.
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
    BOOST_CHECK(entry->parts[0].texture == headless.context->textures()->white());
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
    BOOST_CHECK(entry->parts[0].texture != headless.context->textures()->white());
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
    BOOST_CHECK(crateEntry.texture != headless.context->textures()->white());
    BOOST_CHECK(crateEntry.texture == barrelEntry.texture);
    BOOST_CHECK(crateEntry.material == barrelEntry.material);

    BOOST_CHECK(meshes->release(crate));
    BOOST_CHECK(meshes->resolve(crate) == nullptr);
    BOOST_CHECK(headless.context->resources()->material(barrelEntry.material) != nullptr);

    // the last user's release releases the albedo. Its material resolves for the frame already
    // queued, and to nothing once that frame is submitted
    BOOST_CHECK(meshes->release(barrel));
    BOOST_CHECK(headless.context->resources()->texture(barrelEntry.texture) == nullptr);
    BOOST_CHECK(headless.context->resources()->material(barrelEntry.material) != nullptr);
    headless.submit(headless.context->ring()->begin());
    BOOST_CHECK(headless.context->resources()->material(barrelEntry.material) == nullptr);
    headless.context->ring()->waitIdle();
    BOOST_CHECK(headless.silent());
}

/**
 * A context with no colour format, so that no 2D pipeline can be built against it, still
 * uploads a texture and registers a textured mesh. Textures belong to the device context, not
 * to the quad renderer, so uploading one compiles no pipeline.
 **/
BOOST_AUTO_TEST_CASE(a_context_that_draws_nothing_still_loads_textures) {
    v3d::test::Headless headless(VK_FORMAT_UNDEFINED, width, height);
    const unsigned char pixel[4] = {0x10, 0x20, 0x30, 0xFF};
    const v3d::render::realtime::TextureHandle texture = headless.context->textures()->texture(pixel, 1, 1, 4);
    BOOST_CHECK(headless.context->resources()->texture(texture) != nullptr);
    BOOST_CHECK(headless.context->textures()->material(texture).valid());

    const boost::shared_ptr<MeshRegistry> meshes = registry(&headless);
    const MeshHandle crate = meshes->add("crate", triangle("pixel.png"));
    BOOST_REQUIRE(meshes->resolve(crate) != nullptr);
    BOOST_CHECK(meshes->resolve(crate)->parts.at(0).texture != headless.context->textures()->white());
    BOOST_CHECK(!headless.context->hasQuads());
    BOOST_CHECK(headless.silent());
}

/**
 * A file whose surfaces differ is one entry in parts, each with its own range and base colour,
 * in the order the model holds them.
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
    BOOST_CHECK(entry.parts[0].texture != headless.context->textures()->white());
    BOOST_CHECK(entry.parts[1].texture == headless.context->textures()->white());

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
 * frame, as with a texture: it is destroyed once the frames in flight have finished. The case
 * passes if the validation layer reports no errors.
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

/**
 * A mesh released after an item drawing it is queued, and before the frame is begun, is still
 * drawn by that frame. It is destroyed only once that frame has finished. The case passes if the
 * validation layer reports no errors.
 **/
BOOST_AUTO_TEST_CASE(a_mesh_released_after_its_items_are_queued_outlives_its_frame) {
    v3d::test::Headless headless(colourFormat, width, height);
    const boost::shared_ptr<MeshRegistry> meshes = registry(&headless);
    const MeshHandle handle = meshes->add("triangle", triangle(""));

    boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(),
        width, height, VK_FORMAT_UNDEFINED, true, true);

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

    // one frame submitted first, so the ring has begun a frame before the release
    headless.submitAndWait(headless.context->ring()->begin());

    Frame frame;
    boost::shared_ptr<Pass> pass = frame.pass("depth");
    pass->target(target);
    pass->depth(true);
    DrawItem item;
    item.pipeline = pipeline;
    meshes->resolve(handle)->mesh->describe(&item);
    pass->submit(item);

    BOOST_CHECK(meshes->release(handle));

    VkCommandBuffer commands = headless.context->ring()->begin();
    Recorder::record(commands, frame, Recorder::Target(), *headless.context->resources());
    headless.submit(commands);
    for (uint32_t turn = 0; turn <= headless.context->ring()->framesInFlight(); turn++) {
        headless.submit(headless.context->ring()->begin());
    }
    headless.context->ring()->waitIdle();

    BOOST_CHECK(headless.silent());
}

/**
 * A textured mesh released after an item drawing it is queued, and before the frame is begun,
 * is drawn with its albedo by that frame. The item names its material by handle, and the frame
 * resolves it when it is recorded, so the material has to outlive the release as the mesh does.
 *
 * The triangle covers the target and the albedo is one flat colour at the ends of each channel's
 * range, so every texel of the picture is that colour on any conformant implementation.
 **/
BOOST_AUTO_TEST_CASE(a_textured_mesh_released_after_its_items_are_queued_draws_its_albedo) {
    v3d::test::Headless headless(colourFormat, width, height);
    const boost::shared_ptr<MeshRegistry> meshes = registry(&headless);

    const boost::shared_ptr<v3d::image::Image> albedo = boost::make_shared<v3d::image::Image>(4, 4, 32);
    for (std::size_t texel = 0; texel < std::size_t{16}; texel++) {
        unsigned char* at = albedo->data() + texel * 4;
        at[0] = 0xFF;
        at[1] = 0x00;
        at[2] = 0xFF;
        at[3] = 0xFF;
    }
    const MeshHandle handle = meshes->add("triangle", triangle(""), {albedo});
    const MeshRegistry::Entry* entry = meshes->resolve(handle);
    BOOST_REQUIRE(entry != nullptr);
    BOOST_REQUIRE_EQUAL(entry->parts.size(), 1U);
    BOOST_REQUIRE(entry->parts[0].texture != headless.context->textures()->white());

    boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(),
        width, height, colourFormat);

    // the vertex stage covers the target from the index alone, so the mesh's three indices
    // draw a triangle over every pixel whatever its positions are
    Builder builder(headless.device);
    builder.name("mesh-albedo")
        .shader(VK_SHADER_STAGE_VERTEX_BIT, coverShader, sizeof(coverShader))
        .shader(VK_SHADER_STAGE_FRAGMENT_BIT, albedoShader, sizeof(albedoShader))
        .cull(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
        .blend(false)
        .set(headless.context->frameUniforms()->layout())
        .set(headless.context->textures()->layout())
        .colourFormat(colourFormat);
    const PipelineHandle pipeline = headless.context->resources()->add(builder.build(headless.context->pipelineCache()));

    // one frame submitted first, so the ring has begun a frame before the release
    headless.submitAndWait(headless.context->ring()->begin());

    Frame frame;
    boost::shared_ptr<Pass> pass = frame.pass("colour");
    pass->clearColour(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    DrawItem item;
    item.pipeline = pipeline;
    item.material = entry->parts[0].material;
    entry->mesh->describe(&item);
    pass->submit(item);

    // the albedo has no other user, so this releases its texture and material as well
    BOOST_CHECK(meshes->release(handle));

    VkCommandBuffer commands = headless.context->ring()->begin();
    Recorder::Target described;
    described.image = target->image();
    described.view = target->view();
    described.extent = target->extent();
    described.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    Recorder::record(commands, frame, described, *headless.context->resources(), headless.context->frameUniforms().get());

    Capture capture(headless.device, headless.logger);
    Capture::Source source;
    source.image = target->image();
    source.extent = target->extent();
    source.format = target->format();
    source.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    capture.record(commands, source);
    headless.submit(commands);

    for (uint32_t turn = 0; turn <= headless.context->ring()->framesInFlight(); turn++) {
        headless.submit(headless.context->ring()->begin());
    }
    headless.context->ring()->waitIdle();

    BOOST_CHECK(headless.silent());

    BOOST_REQUIRE(capture.write("data_out/mesh_released_after_queueing.png"));
    v3d::image::reader::Png png(headless.logger);
    const boost::shared_ptr<v3d::image::Image> picture = png.read("data_out/mesh_released_after_queueing.png");
    BOOST_REQUIRE(picture);
    const unsigned char* pixels = picture->data();
    for (uint32_t texel = 0; texel < width * height; texel++) {
        const unsigned char* at = pixels + static_cast<std::size_t>(texel) * 4;
        if (at[0] != 0xFF || at[1] != 0x00 || at[2] != 0xFF || at[3] != 0xFF) {
            BOOST_ERROR("texel " << texel << " is not the albedo of the released mesh");
            return;
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
