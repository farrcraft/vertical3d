/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/denoise/Denoiser.h>

#include <api/log/Logger.h>
#include <moya/libmoya/denoise/Denoise.h>
#include <moya/libmoya/denoise/internal/Device.h>
#include <moya/libmoya/denoise/internal/Guides.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/output/Channel.h>
#include <moya/libmoya/output/Exr.h>
#include <moya/libmoya/output/Outputs.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

#include <glm/vec2.hpp>

#include <OpenImageDenoise/oidn.hpp>

namespace v3d::moya::denoise {

namespace {

using internal::Guides;

/** The floats of a colour. **/
constexpr std::size_t kComponents = 3;

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

/** The guides of a snapshot, cleaned when asked. **/
Guides guidesOf(oidn::DeviceRef & device, const film::Snapshot & snapshot, const std::vector<output::Channel> & channels, bool clean,
    const boost::shared_ptr<log::Logger> & logger) {
    Guides out;
    const std::optional<std::uint32_t> albedoAt = output::guideOf(snapshot, channels, "albedo");
    const std::optional<std::uint32_t> normalAt = output::guideOf(snapshot, channels, "denoisingNormal");
    if (albedoAt) {
        out.albedo = pictureAt(snapshot, *albedoAt);
    }
    if (normalAt) {
        out.normal = pictureAt(snapshot, *normalAt);
    }
    if (clean) {
        if (!out.albedo.empty()) {
            cleaned(device, "albedo", out.albedo, snapshot.resolution);
        }
        if (!out.normal.empty()) {
            cleaned(device, "normal", out.normal, snapshot.resolution);
        }
        out.usable = !failed(device, "its guides", logger);
        out.clean = true;
    }
    return out;
}

/** One picture denoised, at OIDN's fast quality when fast, or the noisy one when it fails. **/
std::vector<float> filtered(oidn::DeviceRef & device, const std::vector<float> & noisy, const Guides & guides, bool fast, const glm::uvec2 & resolution,
    const std::string & what, const boost::shared_ptr<log::Logger> & logger) {
    std::vector<float> output(noisy.size(), 0.0F);
    oidn::FilterRef filter = device.newFilter("RT");
    filter.setImage("color", const_cast<float *>(noisy.data()), oidn::Format::Float3, resolution.x, resolution.y);
    if (guides.usable && !guides.albedo.empty()) {
        filter.setImage("albedo", const_cast<float *>(guides.albedo.data()), oidn::Format::Float3, resolution.x, resolution.y);
        if (!guides.normal.empty()) {
            filter.setImage("normal", const_cast<float *>(guides.normal.data()), oidn::Format::Float3, resolution.x, resolution.y);
        }
        filter.set("cleanAux", guides.clean);
    }
    filter.setImage("output", output.data(), oidn::Format::Float3, resolution.x, resolution.y);
    filter.set("hdr", true);
    filter.set("inputScale", scaleOf(noisy, kComponents, 0, noisy.size() / kComponents));
    if (fast) {
        filter.set("quality", oidn::Quality::Fast);
    }
    filter.commit();
    filter.execute();
    return failed(device, what, logger) ? noisy : output;
}

};  // namespace

Denoiser::Denoiser(const boost::shared_ptr<log::Logger> & logger) : device_(boost::make_shared<internal::Device>()), logger_(logger) {
    device_->device = oidn::newDevice(oidn::DeviceType::CPU);
    device_->device.commit();
}

void Denoiser::into(film::Snapshot & snapshot, const std::vector<output::Channel> & channels, bool beauty) {
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
    const Guides guides = guidesOf(device_->device, snapshot, channels, true, logger_);
    for (const Target & target : targets) {
        putAt(snapshot, target.to, filtered(device_->device, pictureAt(snapshot, target.from), guides, false, snapshot.resolution, target.name, logger_));
    }
}

std::vector<float> Denoiser::beauty(const film::Snapshot & snapshot, const std::vector<output::Channel> & channels, bool preview) {
    const Guides guides = guidesOf(device_->device, snapshot, channels, !preview, logger_);
    return filtered(device_->device, pictureAt(snapshot, 0), guides, preview, snapshot.resolution, "the beauty", logger_);
}

};  // namespace v3d::moya::denoise
