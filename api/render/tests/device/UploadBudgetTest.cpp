/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/asset/Manager.h>
#include <api/asset/Worker.h>
#include <api/asset/Writer.h>
#include <api/asset/media/Loaders.h>
#include <api/engine/Statistics.h>
#include <api/image/Factory.h>
#include <api/image/Image.h>
#include <api/model/Model.h>
#include <api/render/realtime/DeviceContext.h>
#include <api/render/realtime/MeshRegistry.h>
#include <api/render/realtime/Textures.h>
#include <api/render/realtime/vulkan/frame/Ring.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Headless.h"

using v3d::asset::Worker;
using v3d::render::realtime::MeshRegistry;
using v3d::render::realtime::Textures;

namespace {

const VkFormat colourFormat = VK_FORMAT_R8G8B8A8_UNORM;
const uint32_t width = 16;
const uint32_t height = 16;

/**
 * The share of a 60 Hz frame one poll may take: an eighth, about 2.1 ms.
 **/
constexpr std::uint64_t pollShare = 1000000000ULL / 60ULL / 8ULL;

/**
 * A region's worth of content, as a world streams it: textures of 512 by 512, and meshes of a
 * 64 by 64 grid of vertices, about the detail of a prop or a stretch of terrain.
 **/
constexpr unsigned int regionTextures = 24;
constexpr uint32_t textureSide = 512;
constexpr unsigned int regionMeshes = 48;
constexpr unsigned int gridSide = 64;

/**
 * A grid of gridSide by gridSide vertices, in one part with no texture. The offset moves it, so
 * no two of a region's grids are the same model.
 **/
v3d::model::Model grid(float offset) {
    v3d::model::Model model;
    model.vertices().resize(static_cast<std::size_t>(gridSide) * gridSide);
    for (unsigned int row = 0; row < gridSide; row++) {
        for (unsigned int column = 0; column < gridSide; column++) {
            v3d::model::Model::Vertex& vertex = model.vertices()[(static_cast<std::size_t>(row) * gridSide) + column];
            vertex.position = glm::vec3(static_cast<float>(column) + offset, 0.0F, static_cast<float>(row));
            vertex.normal = glm::vec3(0.0F, 1.0F, 0.0F);
            vertex.uv = glm::vec2(static_cast<float>(column), static_cast<float>(row)) / static_cast<float>(gridSide - 1);
        }
    }
    for (unsigned int row = 0; row + 1 < gridSide; row++) {
        for (unsigned int column = 0; column + 1 < gridSide; column++) {
            const std::uint32_t corner = (row * gridSide) + column;
            model.indices().insert(model.indices().end(),
                {corner, corner + gridSide, corner + 1, corner + 1, corner + gridSide, corner + gridSide + 1});
        }
    }
    model.materials().push_back(v3d::model::Model::Material());
    model.parts().push_back({0, static_cast<std::uint32_t>(model.indices().size()), 0});
    return model;
}

/**
 * Write the region's textures as PNG files for the worker to read and decode.
 **/
void writeTextures(v3d::test::Headless* headless, const boost::filesystem::path& directory) {
    boost::filesystem::create_directories(directory);
    v3d::image::Factory factory(headless->logger);
    for (unsigned int index = 0; index < regionTextures; index++) {
        const boost::shared_ptr<v3d::image::Image> image =
            boost::make_shared<v3d::image::Image>(glm::uvec2(textureSide, textureSide), 32);
        const std::size_t bytes = static_cast<std::size_t>(textureSide) * textureSide * 4;
        for (std::size_t at = 0; at < bytes; at++) {
            (*image)[static_cast<unsigned int>(at)] = static_cast<unsigned char>((at * 7) + index);
        }
        const std::optional<std::string> encoded = factory.encode(image, "png");
        BOOST_REQUIRE(encoded);
        BOOST_REQUIRE(v3d::asset::writeFile(directory / ("region_" + std::to_string(index) + ".png"), *encoded));
    }
}

/**
 * Poll a worker until every job's completion has run, or a minute passes.
 **/
bool drain(Worker* worker) {
    const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + std::chrono::seconds(60);
    while (worker->outstanding() > 0) {
        if (std::chrono::steady_clock::now() > deadline) {
            return false;
        }
        worker->poll();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return true;
}

/**
 * @return how many of a list of times are longer than a poll's share of a frame
 **/
std::size_t over(const std::vector<std::uint64_t>& times) {
    return static_cast<std::size_t>(std::ranges::count_if(times, [](std::uint64_t took) { return took > pollShare; }));
}

BOOST_AUTO_TEST_SUITE(upload_budget_test)

/**
 * The default upload budget keeps one poll inside an eighth of a 60 Hz frame while a region
 * streams in.
 *
 * A region's textures are requested and decoded on the worker, then uploaded by polls at the
 * default budget, a frame between each, with every poll timed through engine::Statistics as
 * Engine::measure times one. A region's meshes are added in runs that fill the same budget,
 * because MeshRegistry::poll uploads a decoded model through add(), and each run is timed the
 * same way. The case reports every time it measured, and fails if more than one poll of either
 * kind takes more than its share. One may, because a poll that fills the allocator's last block
 * of device memory has it make a new one, which costs more than a poll's uploads do.
 *
 * It is disabled, because what it measures belongs to the machine it runs on and to a Release
 * build. Run it by hand:
 *
 *     out/build/x64-Release/api/render/tests/v3dtest_render_device.exe --run_test=upload_budget_test --log_level=message
 *
 * Passes before the change: it measures the budget as it stands, and fails only if the number
 * the change sets is too large for this machine.
 **/
BOOST_AUTO_TEST_CASE(a_poll_at_the_default_budget_fits_its_share_of_a_frame, *boost::unit_test::disabled()) {
    v3d::test::Headless headless(colourFormat, width, height);
    const boost::shared_ptr<v3d::render::realtime::vulkan::frame::Ring> ring = headless.context->ring();
    const boost::filesystem::path directory = boost::filesystem::path("data_out") / "upload_budget";
    writeTextures(&headless, directory);

    const boost::shared_ptr<v3d::asset::Manager> assets = boost::make_shared<v3d::asset::Manager>(directory.string(), headless.logger);
    v3d::asset::media::registerLoaders(*assets, headless.logger);
    const boost::shared_ptr<Worker> worker = boost::make_shared<Worker>();
    const boost::shared_ptr<Textures> textures = headless.context->textures();
    textures->requestFrom(assets, worker, headless.logger);
    for (unsigned int index = 0; index < regionTextures; index++) {
        textures->request("region_" + std::to_string(index) + ".png");
    }
    BOOST_REQUIRE(drain(worker.get()));

    v3d::engine::Statistics statistics;
    std::vector<std::uint64_t> texturePolls;
    std::size_t uploaded = 0;
    while (uploaded < regionTextures) {
        std::size_t count = 0;
        {
            const v3d::engine::Statistics::Scope scope = statistics.scope("textures");
            count = textures->poll();
        }
        statistics.frame(std::chrono::nanoseconds(0), 0);
        texturePolls.push_back(statistics.rows().front().last);
        BOOST_REQUIRE_GT(count, 0U);
        uploaded += count;
        headless.submit(ring->begin());
    }

    const boost::shared_ptr<MeshRegistry> meshes = boost::make_shared<MeshRegistry>(headless.logger, headless.context, assets);
    // built before anything is timed, as the worker decodes a model before a poll uploads it
    std::vector<v3d::model::Model> models;
    models.reserve(regionMeshes);
    for (unsigned int index = 0; index < regionMeshes; index++) {
        models.push_back(grid(static_cast<float>(index)));
    }
    std::vector<std::uint64_t> meshPolls;
    unsigned int added = 0;
    while (added < regionMeshes) {
        std::size_t spent = 0;
        {
            const v3d::engine::Statistics::Scope scope = statistics.scope("meshes");
            // a run of nothing yet is under any budget, so every run adds at least one
            while (added < regionMeshes && spent < MeshRegistry::DEFAULT_UPLOAD_BUDGET) {
                const v3d::model::Model& model = models[added];
                meshes->add("region_mesh_" + std::to_string(added), model);
                spent += model.vertexBytes() + (model.indices().size() * sizeof(std::uint32_t));
                added++;
            }
        }
        statistics.frame(std::chrono::nanoseconds(0), 0);
        for (const v3d::engine::Statistics::Row& row : statistics.rows()) {
            if (row.name == "meshes") {
                meshPolls.push_back(row.last);
            }
        }
        headless.submit(ring->begin());
    }
    ring->waitIdle();

    BOOST_TEST_MESSAGE("budget " << Textures::DEFAULT_UPLOAD_BUDGET << " bytes, share " << pollShare / 1000 << " us");
    for (const std::uint64_t took : texturePolls) {
        BOOST_TEST_MESSAGE("texture poll " << static_cast<double>(took) / 1000.0 << " us");
    }
    for (const std::uint64_t took : meshPolls) {
        BOOST_TEST_MESSAGE("mesh poll " << static_cast<double>(took) / 1000.0 << " us");
    }
    BOOST_CHECK_LE(over(texturePolls), 1U);
    BOOST_CHECK_LE(over(meshPolls), 1U);
    BOOST_CHECK(headless.silent());
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
