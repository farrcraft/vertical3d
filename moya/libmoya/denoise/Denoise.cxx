/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/denoise/Denoise.h>

#include <api/log/Logger.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/output/Channel.h>
#include <moya/libmoya/output/Exr.h>
#include <moya/libmoya/output/Outputs.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <OpenImageDenoise/oidn.hpp>

namespace v3d::moya::denoise {

namespace {

/** The floats of a colour. **/
constexpr std::size_t kComponents = 3;

/** ACEScg's luminance weights, the Y row of its matrix into CIE XYZ. **/
constexpr double kRed = 0.2722287168;
constexpr double kGreen = 0.6740817658;
constexpr double kBlue = 0.0536895174;

/** The least luminance a pixel holds to count as holding light. **/
constexpr double kDark = 1.0e-8;

/** The offset of the first float of a raw channel of three components that reads a value. **/
std::optional<std::uint32_t> guideOf(const film::Snapshot & snapshot, const std::vector<output::Channel> & channels, const std::string & reading) {
    for (const output::Channel & channel : channels) {
        if (channel.source == output::Source::Raw && channel.sourceName == reading && channel.film.components == kComponents) {
            return snapshot.offset(channel.film.name);
        }
    }
    return std::nullopt;
}

/** A picture's three floats from an offset of each pixel, packed. **/
std::vector<float> pictureAt(const film::Snapshot & snapshot, std::uint32_t offset) {
    const std::size_t pixels = static_cast<std::size_t>(snapshot.resolution.x) * snapshot.resolution.y;
    std::vector<float> out(pixels * kComponents);
    for (std::size_t pixel = 0; pixel < pixels; ++pixel) {
        for (std::size_t i = 0; i < kComponents; ++i) {
            out[(pixel * kComponents) + i] = snapshot.values[(pixel * snapshot.stride) + offset + i];
        }
    }
    return out;
}

/** A packed picture written to an offset of each pixel. **/
void putAt(film::Snapshot & snapshot, std::uint32_t offset, const std::vector<float> & picture) {
    const std::size_t pixels = static_cast<std::size_t>(snapshot.resolution.x) * snapshot.resolution.y;
    for (std::size_t pixel = 0; pixel < pixels; ++pixel) {
        for (std::size_t i = 0; i < kComponents; ++i) {
            snapshot.values[(pixel * snapshot.stride) + offset + i] = picture[(pixel * kComponents) + i];
        }
    }
}

/** Whether the device has failed, which is then reported. **/
bool failed(oidn::DeviceRef & device, const std::string & what, const boost::shared_ptr<log::Logger> & logger) {
    const char * message = nullptr;
    if (device.getError(message) == oidn::Error::None) {
        return false;
    }
    if (logger) {
        logger->get()->error("OIDN could not denoise {}: {}, so it is written noisy", what, message == nullptr ? "no reason given" : message);
    }
    return true;
}

/** A guide filtered by the RT filter alone, in place, as OIDN's guides are cleaned. **/
void cleaned(oidn::DeviceRef & device, const char * name, std::vector<float> & guide, const glm::uvec2 & resolution) {
    oidn::FilterRef filter = device.newFilter("RT");
    filter.setImage(name, guide.data(), oidn::Format::Float3, resolution.x, resolution.y);
    filter.setImage("output", guide.data(), oidn::Format::Float3, resolution.x, resolution.y);
    filter.commit();
    filter.execute();
}

};  // namespace

float scaleOf(const std::vector<float> & values, std::size_t stride, std::size_t first, std::size_t pixels) {
    double logs = 0.0;
    std::size_t lit = 0;
    for (std::size_t pixel = 0; pixel < pixels; ++pixel) {
        const float * const colour = &values[(pixel * stride) + first];
        const double luminance = (kRed * colour[0]) + (kGreen * colour[1]) + (kBlue * colour[2]);
        if (luminance > kDark) {
            logs += std::log(luminance);
            ++lit;
        }
    }
    return lit == 0 ? 1.0F : static_cast<float>(kKey / std::exp(logs / static_cast<double>(lit)));
}

void denoise(film::Snapshot & snapshot, const std::vector<output::Channel> & channels, bool beauty, const boost::shared_ptr<log::Logger> & logger) {
    struct Target {
        std::string name;
        std::uint32_t from { 0 };
        std::uint32_t to { 0 };
    };
    std::vector<Target> targets;
    if (beauty) {
        targets.push_back(Target { .name = "the beauty", .from = 0, .to = snapshot.offset(output::denoisedOf(std::string(output::kBeauty))) });
    }
    for (const output::Channel & channel : channels) {
        if (channel.denoised) {
            targets.push_back(Target { .name = channel.film.name, .from = snapshot.offset(channel.film.name), .to = snapshot.offset(output::denoisedOf(channel.film.name)) });
        }
    }
    if (targets.empty()) {
        return;
    }
    const std::optional<std::uint32_t> albedoAt = guideOf(snapshot, channels, "albedo");
    const std::optional<std::uint32_t> normalAt = guideOf(snapshot, channels, "denoisingNormal");
    const glm::uvec2 resolution = snapshot.resolution;
    const std::size_t pixels = static_cast<std::size_t>(resolution.x) * resolution.y;
    oidn::DeviceRef device = oidn::newDevice(oidn::DeviceType::CPU);
    device.commit();
    std::vector<float> albedo = albedoAt ? pictureAt(snapshot, *albedoAt) : std::vector<float>();
    std::vector<float> normal = normalAt ? pictureAt(snapshot, *normalAt) : std::vector<float>();
    if (!albedo.empty()) {
        cleaned(device, "albedo", albedo, resolution);
    }
    if (!normal.empty()) {
        cleaned(device, "normal", normal, resolution);
    }
    const bool guided = !failed(device, "its guides", logger);
    for (const Target & target : targets) {
        const std::vector<float> noisy = pictureAt(snapshot, target.from);
        std::vector<float> output(noisy.size(), 0.0F);
        oidn::FilterRef filter = device.newFilter("RT");
        filter.setImage("color", const_cast<float *>(noisy.data()), oidn::Format::Float3, resolution.x, resolution.y);
        if (guided && !albedo.empty()) {
            filter.setImage("albedo", albedo.data(), oidn::Format::Float3, resolution.x, resolution.y);
            if (!normal.empty()) {
                filter.setImage("normal", normal.data(), oidn::Format::Float3, resolution.x, resolution.y);
            }
            filter.set("cleanAux", true);
        }
        filter.setImage("output", output.data(), oidn::Format::Float3, resolution.x, resolution.y);
        filter.set("hdr", true);
        filter.set("inputScale", scaleOf(snapshot.values, snapshot.stride, target.from, pixels));
        filter.commit();
        filter.execute();
        putAt(snapshot, target.to, failed(device, target.name, logger) ? noisy : output);
    }
}

};  // namespace v3d::moya::denoise
