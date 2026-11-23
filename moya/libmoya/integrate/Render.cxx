/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/integrate/Render.h>

#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/film/Channel.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/integrate/Profile.h>
#include <moya/libmoya/integrate/Session.h>
#include <moya/libmoya/integrate/Trails.h>
#include <moya/libmoya/integrate/internal/Options.h>
#include <moya/libmoya/kernel/integrate/Settings.h>
#include <moya/libmoya/kernel/sample/ZSobol.h>
#include <moya/libmoya/scene/Description.h>
#include <moya/libmoya/scene/Scene.h>

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <glm/vec2.hpp>

namespace v3d::moya::integrate {

namespace {

/** FNV-1a's 64-bit offset basis and prime. **/
constexpr std::uint64_t kFnvOffset = 0xCBF29CE484222325ULL;
constexpr std::uint64_t kFnvPrime = 0x100000001B3ULL;

/** The strategy an option names: multiple importance sampling unless it names another. **/
kernel::integrate::Strategy strategyNamed(const std::string & name) {
    if (name == "light") {
        return kernel::integrate::Strategy::Light;
    }
    return name == "bsdf" ? kernel::integrate::Strategy::Bsdf : kernel::integrate::Strategy::Mis;
}

};  // namespace

kernel::integrate::Settings settingsOf(const scene::Scene & scene) {
    using internal::count;
    using internal::option;
    return kernel::integrate::Settings { .maxDepth = count(scene, "maxDepth"),
        .diffuseDepth = count(scene, "diffuseDepth"),
        .glossyDepth = count(scene, "glossyDepth"),
        .specularDepth = count(scene, "specularDepth"),
        .transmissionDepth = count(scene, "transmissionDepth"),
        .volumeDepth = count(scene, "volumeDepth"),
        .subsurfaceDepth = count(scene, "subsurfaceDepth"),
        .rouletteDepth = count(scene, "rouletteDepth"),
        .strategy = strategyNamed(option(scene, "lightSampling").stringArray().front()),
        .picking = option(scene, "lightPicking").stringArray().front() == "power" ? kernel::integrate::Picking::Power : kernel::integrate::Picking::Tree };
}

std::uint64_t fingerprint(const scene::Scene & scene, const std::vector<film::Channel> & channels) {
    std::string text = scene::describe(scene);
    for (const film::Channel & channel : channels) {
        text += "channel " + channel.name + " " + std::to_string(channel.components) + " " + std::to_string(static_cast<std::uint32_t>(channel.combine)) + "\n";
    }
    text += "samplesPerPass " + std::to_string(kSamplesPerPass) + "\n";
    std::uint64_t hash = kFnvOffset;
    for (const char letter : text) {
        hash ^= static_cast<unsigned char>(letter);
        hash *= kFnvPrime;
    }
    return hash;
}

film::Snapshot render(device::CpuDevice & device, const scene::Scene & scene, const boost::shared_ptr<log::Logger> & logger,
    const colour::Config & config, const Checkpoints & checkpoints, Profile * profile, Trails * trails) {
    Session session(device, scene, nullptr, logger, config, checkpoints, profile, trails);
    while (session.advance()) {
    }
    session.finish();
    return session.picture();
}

film::Snapshot render(device::CpuDevice & device, const scene::Scene & scene, const geometry::Prototypes & prototypes,
    const boost::shared_ptr<log::Logger> & logger, const colour::Config & config, const Checkpoints & checkpoints, Profile * profile,
    Trails * trails) {
    Session session(device, scene, &prototypes, logger, config, checkpoints, profile, trails);
    while (session.advance()) {
    }
    session.finish();
    return session.picture();
}

std::uint32_t passesOf(const scene::Scene & scene) {
    const std::uint32_t samples = kernel::sample::ZSobol(std::max(internal::count(scene, "samples"), 1U), glm::uvec2(1U), 0, 0).samplesPerPixel();
    return (samples + kSamplesPerPass - 1U) / kSamplesPerPass;
}

};  // namespace v3d::moya::integrate
