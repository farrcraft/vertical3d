/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/asset/Manager.h>
#include <api/asset/Worker.h>
#include <api/asset/media/Loaders.h>
#include <api/image/Image.h>
#include <api/image/reader/Png.h>
#include <api/log/Logger.h>
#include <api/render/realtime/DeviceContext.h>
#include <api/render/realtime/Frame.h>
#include <api/render/realtime/Handle.h>
#include <api/render/realtime/Pass.h>
#include <api/render/realtime/Textures.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/render/realtime/vulkan/frame/Capture.h>
#include <api/render/realtime/vulkan/frame/Recorder.h>
#include <api/render/realtime/vulkan/frame/RenderTarget.h>
#include <api/render/realtime/vulkan/pipeline/Resources.h>
#include <spdlog/sinks/ringbuffer_sink.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include "Headless.h"

using v3d::asset::Worker;
using v3d::render::realtime::Frame;
using v3d::render::realtime::Pass;
using v3d::render::realtime::TextureHandle;
using v3d::render::realtime::Textures;
using v3d::render::realtime::canvas::Canvas;
using v3d::render::realtime::vulkan::frame::Capture;
using v3d::render::realtime::vulkan::frame::Recorder;
using v3d::render::realtime::vulkan::frame::RenderTarget;

namespace {

const VkFormat colourFormat = VK_FORMAT_R8G8B8A8_UNORM;
const uint32_t width = 16;
const uint32_t height = 16;

/**
 * The context's textures, requesting from the asset suite's fixtures through a worker. The
 * build passes their directory in V3D_ASSET_FIXTURES. pixel.png there is two by two and blue.
 **/
boost::shared_ptr<Textures> requesting(v3d::test::Headless* headless, const boost::shared_ptr<Worker>& worker) {
    const boost::shared_ptr<v3d::asset::Manager> assets = boost::make_shared<v3d::asset::Manager>(V3D_ASSET_FIXTURES, headless->logger);
    v3d::asset::media::registerLoaders(*assets, headless->logger);
    const boost::shared_ptr<Textures> textures = headless->context->textures();
    textures->requestFrom(assets, worker, headless->logger);
    return textures;
}

/**
 * Poll a worker until every job's completion has run, or ten seconds pass.
 *
 * @return whether everything completed
 **/
bool drain(Worker* worker) {
    const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
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
 * Draw one frame of a quad over the whole target, textured with a handle, and read back the
 * texel at its centre as RGBA.
 **/
std::vector<unsigned char> drawn(v3d::test::Headless* headless, const TextureHandle& texture, const std::string& path) {
    const boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(headless->device, headless->context->ring(),
        width, height, colourFormat);

    Canvas canvas;
    canvas.resize(glm::uvec2(width, height));
    canvas.clear();
    canvas.rect(glm::vec2(0.0F, 0.0F), glm::vec2(static_cast<float>(width), static_cast<float>(height)),
        glm::vec2(0.0F, 0.0F), glm::vec2(1.0F, 1.0F), glm::vec4(1.0F, 1.0F, 1.0F, 1.0F), texture);

    Frame frame;
    const boost::shared_ptr<Pass> pass = frame.pass("colour");
    pass->clearColour(glm::vec4(0.0F, 0.0F, 0.0F, 1.0F));
    headless->context->quads()->submit(canvas, pass.get());

    Capture capture(headless->device, headless->logger);
    VkCommandBuffer commands = headless->context->ring()->begin();
    Recorder::Target described;
    described.image = target->image();
    described.view = target->view();
    described.extent = target->extent();
    described.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    Recorder::record(commands, frame, described, *headless->context->resources(), headless->context->frameUniforms().get());
    Capture::Source source;
    source.image = target->image();
    source.extent = target->extent();
    source.format = target->format();
    source.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    capture.record(commands, source);
    headless->submitAndWait(commands);

    BOOST_REQUIRE(capture.write(path));
    v3d::image::reader::Png png(headless->logger);
    const boost::shared_ptr<v3d::image::Image> picture = png.read(path);
    BOOST_REQUIRE(picture);
    const std::size_t centre = ((static_cast<std::size_t>(height / 2) * width) + (width / 2)) * 4;
    const unsigned char* at = picture->data() + centre;
    return std::vector<unsigned char>(at, at + 4);
}

/**
 * A sink holding the last lines logged, so a case can read what was reported.
 **/
std::shared_ptr<spdlog::sinks::ringbuffer_sink_mt> listen(const boost::shared_ptr<v3d::log::Logger>& logger) {
    const std::shared_ptr<spdlog::sinks::ringbuffer_sink_mt> sink = std::make_shared<spdlog::sinks::ringbuffer_sink_mt>(16);
    logger->get()->sinks().push_back(sink);
    return sink;
}

BOOST_AUTO_TEST_SUITE(texture_request_test)

/**
 * A requested texture can be drawn at once, and draws white until its image arrives. Once poll()
 * has uploaded it, the same handle draws its pixels in the next frame.
 **/
BOOST_AUTO_TEST_CASE(a_requested_texture_draws_white_then_its_pixels) {
    v3d::test::Headless headless(colourFormat, width, height);
    const boost::shared_ptr<Worker> worker = boost::make_shared<Worker>();
    const boost::shared_ptr<Textures> textures = requesting(&headless, worker);

    const TextureHandle requested = textures->request("pixel.png");
    BOOST_REQUIRE(requested.valid());
    BOOST_CHECK(headless.context->resources()->texture(requested) != nullptr);
    BOOST_CHECK_EQUAL(textures->slot(requested), textures->slot(textures->white()));
    const std::vector<unsigned char> before = drawn(&headless, requested, "data_out/texture_requested_before.png");
    BOOST_CHECK(before == (std::vector<unsigned char>{0xFF, 0xFF, 0xFF, 0xFF}));

    BOOST_REQUIRE(drain(worker.get()));
    BOOST_CHECK_EQUAL(textures->poll(), 1U);
    BOOST_CHECK(textures->slot(requested) != textures->slot(textures->white()));
    const std::vector<unsigned char> after = drawn(&headless, requested, "data_out/texture_requested_after.png");
    BOOST_CHECK(after == (std::vector<unsigned char>{0x00, 0x00, 0xFF, 0xFF}));

    BOOST_CHECK(textures->release(requested));
    headless.context->ring()->waitIdle();
    BOOST_CHECK(headless.silent());
}

/**
 * Asking for a path again gives the handle the first request was given, before its image has
 * arrived and after, and the image is read once.
 **/
BOOST_AUTO_TEST_CASE(a_path_requested_again_is_the_same_handle) {
    v3d::test::Headless headless(colourFormat, width, height);
    const boost::shared_ptr<Worker> worker = boost::make_shared<Worker>();
    const boost::shared_ptr<Textures> textures = requesting(&headless, worker);

    const TextureHandle first = textures->request("pixel.png");
    BOOST_CHECK(textures->request("pixel.png") == first);
    BOOST_REQUIRE(drain(worker.get()));
    BOOST_CHECK_EQUAL(textures->poll(), 1U);
    BOOST_CHECK(textures->request("pixel.png") == first);
    BOOST_CHECK_EQUAL(worker->outstanding(), 0U);

    headless.context->ring()->waitIdle();
    BOOST_CHECK(headless.silent());
}

/**
 * A request released before its image arrives is abandoned: poll() uploads nothing for it, and
 * the path can be requested again as a new handle.
 **/
BOOST_AUTO_TEST_CASE(a_request_released_before_it_arrives_is_abandoned) {
    v3d::test::Headless headless(colourFormat, width, height);
    const boost::shared_ptr<Worker> worker = boost::make_shared<Worker>();
    const boost::shared_ptr<Textures> textures = requesting(&headless, worker);
    const std::size_t before = headless.context->resources()->textureCount();

    const TextureHandle requested = textures->request("pixel.png");
    BOOST_CHECK(textures->release(requested));
    BOOST_CHECK(headless.context->resources()->texture(requested) == nullptr);
    BOOST_REQUIRE(drain(worker.get()));
    BOOST_CHECK_EQUAL(textures->poll(), 0U);
    BOOST_CHECK_EQUAL(textures->slot(requested), textures->slot(textures->white()));

    const TextureHandle again = textures->request("pixel.png");
    BOOST_CHECK(again != requested);
    BOOST_CHECK(textures->release(again));

    for (uint32_t turn = 0; turn <= headless.context->ring()->framesInFlight(); turn++) {
        headless.submit(headless.context->ring()->begin());
    }
    headless.context->ring()->waitIdle();
    BOOST_CHECK_EQUAL(headless.context->resources()->textureCount(), before);
    BOOST_CHECK(headless.silent());
}

/**
 * A file that cannot be read is reported, and its handle goes on drawing white.
 **/
BOOST_AUTO_TEST_CASE(a_missing_image_is_reported_and_draws_white) {
    v3d::test::Headless headless(colourFormat, width, height);
    const std::shared_ptr<spdlog::sinks::ringbuffer_sink_mt> sink = listen(headless.logger);
    const boost::shared_ptr<Worker> worker = boost::make_shared<Worker>();
    const boost::shared_ptr<Textures> textures = requesting(&headless, worker);

    const TextureHandle requested = textures->request("no_such_image.png");
    BOOST_REQUIRE(drain(worker.get()));
    BOOST_CHECK_EQUAL(textures->poll(), 0U);
    BOOST_CHECK_EQUAL(textures->slot(requested), textures->slot(textures->white()));
    BOOST_CHECK(headless.context->resources()->texture(requested) != nullptr);

    bool reported = false;
    for (const std::string& line : sink->last_formatted()) {
        reported = reported || line.find("The texture no_such_image.png could not be loaded") != std::string::npos;
    }
    BOOST_CHECK(reported);
    const std::vector<unsigned char> texel = drawn(&headless, requested, "data_out/texture_requested_missing.png");
    BOOST_CHECK(texel == (std::vector<unsigned char>{0xFF, 0xFF, 0xFF, 0xFF}));

    headless.context->ring()->waitIdle();
}

/**
 * A request that failed frees its path. Asking for the path again reads the file again as a new
 * handle, and the failed handle goes on drawing white until it is released.
 **/
BOOST_AUTO_TEST_CASE(a_failed_request_frees_its_path) {
    v3d::test::Headless headless(colourFormat, width, height);
    const boost::shared_ptr<Worker> worker = boost::make_shared<Worker>();
    const boost::shared_ptr<Textures> textures = requesting(&headless, worker);

    const TextureHandle failed = textures->request("no_such_image.png");
    BOOST_REQUIRE(drain(worker.get()));
    BOOST_CHECK_EQUAL(textures->poll(), 0U);

    const TextureHandle again = textures->request("no_such_image.png");
    BOOST_CHECK(again != failed);
    BOOST_CHECK_EQUAL(worker->outstanding(), 1U);
    BOOST_CHECK(headless.context->resources()->texture(failed) != nullptr);
    BOOST_CHECK_EQUAL(textures->slot(failed), textures->slot(textures->white()));
    BOOST_REQUIRE(drain(worker.get()));
    BOOST_CHECK_EQUAL(textures->poll(), 0U);

    BOOST_CHECK(textures->release(failed));
    BOOST_CHECK(textures->release(again));
    headless.context->ring()->waitIdle();
}

/**
 * A request needs somewhere to read from and something to decode on. Without them it is refused,
 * rather than giving a handle that never fills.
 **/
BOOST_AUTO_TEST_CASE(a_request_without_a_worker_is_refused) {
    const v3d::test::Headless headless(colourFormat, width, height);
    BOOST_CHECK_THROW(headless.context->textures()->request("pixel.png"), std::logic_error);
    BOOST_CHECK(headless.silent());
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
