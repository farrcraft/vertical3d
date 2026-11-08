/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/output/Png.h>

#include <api/asset/Writer.h>
#include <api/image/Factory.h>
#include <api/image/Image.h>
#include <api/log/Logger.h>
#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/film/Film.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

#include <glm/vec2.hpp>

namespace v3d::moya::output {

namespace {

/** The largest value of an 8-bit channel. **/
constexpr float kLargest = 255.0F;

};  // namespace

std::vector<std::uint8_t> displayed(const film::Snapshot & snapshot, const colour::Transform & display) {
    const std::size_t pixels = static_cast<std::size_t>(snapshot.resolution.x) * snapshot.resolution.y;
    std::vector<float> beauty(pixels * 3);
    for (std::size_t pixel = 0; pixel < pixels; ++pixel) {
        for (std::size_t i = 0; i < 3; ++i) {
            beauty[(pixel * 3) + i] = snapshot.values.at((pixel * snapshot.stride) + i);
        }
    }
    display.apply(beauty.data(), pixels, 3);

    std::vector<std::uint8_t> out(beauty.size());
    for (std::size_t i = 0; i < beauty.size(); ++i) {
        const float value = std::isnan(beauty[i]) ? 0.0F : std::clamp(beauty[i], 0.0F, 1.0F);
        out[i] = static_cast<std::uint8_t>(std::lround(value * kLargest));  // checked: clamped to [0, 1] first
    }
    return out;
}

void writePng(const std::string & path, const film::Snapshot & snapshot, const colour::Transform & display,
    const boost::shared_ptr<log::Logger> & logger) {
    const std::vector<std::uint8_t> bytes = displayed(snapshot, display);
    const auto image = boost::make_shared<image::Image>(glm::uvec2(snapshot.resolution.x, snapshot.resolution.y), 24);
    if (!bytes.empty()) {
        std::memcpy(image->data(), bytes.data(), bytes.size());
    }
    image::Factory factory(logger);
    const std::optional<std::string> encoded = factory.encode(image, "png");
    if (!encoded) {
        throw std::runtime_error("the picture for " + path + " did not encode as a PNG");
    }
    if (!asset::writeFile(path, *encoded)) {
        throw std::runtime_error("cannot write " + path);
    }
}

};  // namespace v3d::moya::output
