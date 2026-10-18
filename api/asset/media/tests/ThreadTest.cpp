/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/asset/Manager.h>
#include <api/asset/media/Loaders.h>
#include <api/asset/media/kind/Image.h>
#include <api/asset/media/kind/Model.h>
#include <api/image/Compare.h>
#include <api/image/Factory.h>
#include <api/image/Image.h>
#include <api/log/Logger.h>
#include <api/type/Model.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <latch>
#include <string>
#include <thread>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

namespace {

/**
 * The directory the generated pictures are written to and loaded from. The committed fixtures
 * are copied beside it, so one manager rooted here reaches both.
 **/
const char* const ROOT = "data_out/threads";

/**
 * A picture large enough that decoding it takes long enough for threads to overlap.
 **/
boost::shared_ptr<v3d::image::Image> gradient() {
    const uint32_t size = 256;
    boost::shared_ptr<v3d::image::Image> image = boost::make_shared<v3d::image::Image>(glm::uvec2(size, size), 24);
    unsigned char* pixels = image->data();
    for (uint32_t y = 0; y < size; y++) {
        for (uint32_t x = 0; x < size; x++) {
            unsigned char* pixel = pixels + (((static_cast<std::size_t>(y) * size) + x) * 3);
            pixel[0] = static_cast<unsigned char>(x);
            pixel[1] = static_cast<unsigned char>(y);
            pixel[2] = static_cast<unsigned char>((x * y) & 0xFFU);
        }
    }
    return image;
}

/**
 * A manager rooted at ROOT, with the media loaders registered, after writing a PNG and a JPEG
 * there and copying in the glTF fixtures.
 **/
boost::shared_ptr<v3d::asset::Manager> prepare(const std::vector<std::string>& models) {
    const boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    boost::filesystem::create_directories(ROOT);

    v3d::image::Factory factory(logger);
    const boost::shared_ptr<v3d::image::Image> picture = gradient();
    BOOST_REQUIRE(factory.write(std::string(ROOT) + "/gradient.png", picture));
    BOOST_REQUIRE(factory.write(std::string(ROOT) + "/gradient.jpg", picture));
    for (const std::string& model : models) {
        boost::filesystem::copy_file(boost::filesystem::path("data") / model, boost::filesystem::path(ROOT) / model,
            boost::filesystem::copy_options::overwrite_existing);
    }

    boost::shared_ptr<v3d::asset::Manager> assets = boost::make_shared<v3d::asset::Manager>(ROOT, logger);
    v3d::asset::media::registerLoaders(*assets, logger);
    return assets;
}

/**
 * Whether two loads of one picture decoded to the same pixels.
 **/
bool same(const boost::shared_ptr<v3d::asset::Asset>& expected, const boost::shared_ptr<v3d::asset::Asset>& actual) {
    const auto expectedImage = boost::dynamic_pointer_cast<v3d::asset::media::kind::Image>(expected);
    const auto actualImage = boost::dynamic_pointer_cast<v3d::asset::media::kind::Image>(actual);
    if (expectedImage && actualImage) {
        return expectedImage->image() && actualImage->image() && v3d::image::compare(*expectedImage->image(), *actualImage->image(), 0).match;
    }

    const auto expectedModel = boost::dynamic_pointer_cast<v3d::asset::media::kind::Model>(expected);
    const auto actualModel = boost::dynamic_pointer_cast<v3d::asset::media::kind::Model>(actual);
    if (!expectedModel || !actualModel || !expectedModel->model() || !actualModel->model()) {
        return false;
    }
    const v3d::type::Model& left = *expectedModel->model();
    const v3d::type::Model& right = *actualModel->model();
    if (left.vertices().size() != right.vertices().size() || left.indices() != right.indices() ||
        left.parts().size() != right.parts().size() || left.materials().size() != right.materials().size()) {
        return false;
    }
    if (std::memcmp(left.vertices().data(), right.vertices().data(),
        left.vertices().size() * sizeof(v3d::type::Model::Vertex)) != 0) {
        return false;
    }
    for (std::size_t material = 0; material < left.materials().size(); material++) {
        const boost::shared_ptr<v3d::image::Image> expectedTexture = expectedModel->baseColourImage(material);
        const boost::shared_ptr<v3d::image::Image> actualTexture = actualModel->baseColourImage(material);
        if (static_cast<bool>(expectedTexture) != static_cast<bool>(actualTexture)) {
            return false;
        }
        if (expectedTexture && !v3d::image::compare(*expectedTexture, *actualTexture, 0).match) {
            return false;
        }
    }
    return true;
}

/**
 * One manager loads pictures and models on several threads at once, and every load matches
 * the same load made on one thread. This is what lets a worker thread load through the
 * manager an app already has.
 *
 * Covers the base: 7270a33e holds the loaders as they are. Checked by decoding every PNG into
 * one static buffer shared by all calls, which failed this case on six runs of six.
 **/
BOOST_AUTO_TEST_CASE(thread_one_manager_loads_on_many_threads_test) {
    const std::vector<std::string> models = { "two_surfaces.glb", "embedded_texture.glb", "data_uri_texture.gltf" };
    const boost::shared_ptr<v3d::asset::Manager> assets = prepare(models);

    std::vector<std::string> names = { "gradient.png", "gradient.jpg" };
    names.insert(names.end(), models.begin(), models.end());

    std::vector<boost::shared_ptr<v3d::asset::Asset>> expected;
    for (const std::string& name : names) {
        expected.push_back(assets->loadTypeFromExt(name));
        BOOST_REQUIRE_MESSAGE(expected.back(), name << " loads on one thread");
    }

    const std::size_t threads = 8;
    const std::size_t rounds = 10;
    std::latch start(static_cast<std::ptrdiff_t>(threads));
    std::atomic<std::size_t> mismatches{ 0 };
    std::vector<std::thread> workers;
    workers.reserve(threads);
    for (std::size_t worker = 0; worker < threads; worker++) {
        workers.emplace_back([&, worker]() {
            start.arrive_and_wait();
            for (std::size_t round = 0; round < rounds; round++) {
                // each thread starts at a different file, so different decoders run at once
                const std::size_t which = (worker + round) % names.size();
                if (!same(expected[which], assets->loadTypeFromExt(names[which]))) {
                    mismatches++;
                }
            }
        });
    }
    for (std::thread& worker : workers) {
        worker.join();
    }

    BOOST_TEST(mismatches.load() == 0U);
}

};  // namespace
