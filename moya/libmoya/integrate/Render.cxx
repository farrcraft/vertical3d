/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/integrate/Render.h>

#include <api/log/Logger.h>
#include <api/rib/Sampling.h>
#include <moya/libmoya/accel/Assembly.h>
#include <moya/libmoya/accel/Box.h>
#include <moya/libmoya/accel/Sweep.h>
#include <moya/libmoya/camera/Camera.h>
#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/colour/RgbTables.h>
#include <moya/libmoya/denoise/Denoise.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/device/cpu/Module.h>
#include <moya/libmoya/device/cpu/ProfileParams.h>
#include <moya/libmoya/film/Channel.h>
#include <moya/libmoya/film/Film.h>
#include <moya/libmoya/film/Filter.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/film/State.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/geometry/Surfaces.h>
#include <moya/libmoya/integrate/Profile.h>
#include <moya/libmoya/integrate/Trails.h>
#include <moya/libmoya/integrate/internal/Coverage.h>
#include <moya/libmoya/integrate/internal/Crop.h>
#include <moya/libmoya/integrate/internal/Deep.h>
#include <moya/libmoya/integrate/internal/FirstHits.h>
#include <moya/libmoya/integrate/internal/Kept.h>
#include <moya/libmoya/integrate/internal/LightPaths.h>
#include <moya/libmoya/integrate/internal/Records.h>
#include <moya/libmoya/integrate/internal/Tallies.h>
#include <moya/libmoya/integrate/internal/TrailSteps.h>
#include <moya/libmoya/kernel/camera/Camera.h>
#include <moya/libmoya/kernel/film/Pixels.h>
#include <moya/libmoya/kernel/integrate/Settings.h>
#include <moya/libmoya/kernel/Launch.h>
#include <moya/libmoya/kernel/sample/ZSobol.h>
#include <moya/libmoya/light/Lights.h>
#include <moya/libmoya/light/Shading.h>
#include <moya/libmoya/material/Albedos.h>
#include <moya/libmoya/output/Channel.h>
#include <moya/libmoya/output/Checkpoint.h>
#include <moya/libmoya/output/Output.h>
#include <moya/libmoya/output/Outputs.h>
#include <moya/libmoya/scene/Declaration.h>
#include <moya/libmoya/scene/Description.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Schema.h>
#include <moya/libmoya/scene/Value.h>
#include <moya/libmoya/view/Tracing.h>
#include <moya/libmoya/texture/Options.h>
#include <moya/libmoya/texture/Textures.h>
#include <moya/libmoya/volume/Volumes.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::integrate {

namespace {

/** FNV-1a's 64-bit offset basis and prime. **/
constexpr std::uint64_t kFnvOffset = 0xCBF29CE484222325ULL;
constexpr std::uint64_t kFnvPrime = 0x100000001B3ULL;

/** A parameter of the scene's options, or its fallback while the scene has none. **/
const scene::Value & option(const scene::Scene & scene, const char * name) {
    const std::vector<scene::Handle> options = scene.objects("options");
    if (!options.empty()) {
        return scene.value(options.front(), name);
    }
    for (const scene::Declaration & declaration : scene.schema().kind(*scene.schema().find("options")).parameters) {
        if (declaration.name == name) {
            return declaration.fallback;
        }
    }
    throw std::invalid_argument(std::string("the options have no parameter ") + name);
}

/** An integer option, held to zero or more. **/
std::uint32_t count(const scene::Scene & scene, const char * name) {
    return static_cast<std::uint32_t>(std::max(option(scene, name).integerArray().front(), 0));
}

/** The strategy an option names: multiple importance sampling unless it names another. **/
kernel::integrate::Strategy strategyNamed(const std::string & name) {
    if (name == "light") {
        return kernel::integrate::Strategy::Light;
    }
    return name == "bsdf" ? kernel::integrate::Strategy::Bsdf : kernel::integrate::Strategy::Mis;
}

/** The pixel filter the scene's options name, of the width they give. **/
film::Filter filterOf(device::CpuDevice & device, const scene::Scene & scene) {
    rib::Filter kind = rib::Filter::Gaussian;
    rib::filterNamed(option(scene, "filter").stringArray().front(), &kind);
    const std::vector<float> & width = option(scene, "filterWidth").floatArray();
    return film::Filter(device, kind, glm::vec2 { width[0], width[1] });
}

/** Whether any of a scene's outputs is a deep file. **/
bool deepened(const scene::Scene & scene) {
    const std::vector<output::Output> outputs = output::outputsOf(scene);
    return std::any_of(outputs.begin(), outputs.end(), [](const output::Output & each) { return each.format == output::Format::DeepExr; });
}

/** Restores a film and what it keeps from a checkpoint, and gives the passes it held. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the path, then the fingerprint
std::uint32_t resumed(const std::string & path, const std::vector<film::Channel> & channels, std::uint64_t print, film::Film & picture, internal::Kept & kept) {
    const film::State state = output::readCheckpoint(path, channels, print);
    picture.restore(state);
    kept.coverage.restore(state);
    kept.deep.restore(state);
    return state.passes;
}

/**
 * Fills a profile with the seconds the passes took, the seconds adding what they covered took, and,
 * for a counted render, its tallies.
 **/
void profiled(Profile * profile, double seconds, const internal::Kept & kept, const std::optional<internal::Tallies> & tallies) {
    if (profile == nullptr) {
        return;
    }
    profile->seconds = seconds;
    profile->coverage = kept.coverageSeconds;
    profile->deep = kept.deepSeconds;
    if (tallies) {
        tallies->into(*profile);
    }
}

/**
 * A film's snapshot, with each Cryptomatte layer's ranks and manifest, its deep samples, and the
 * denoised copies the scene asks for.
 **/
film::Snapshot finished(const film::Film & picture, const internal::Kept & kept, const scene::Scene & scene, const std::vector<output::Channel> & channels,
    const boost::shared_ptr<log::Logger> & logger) {
    film::Snapshot out = picture.snapshot();
    kept.coverage.into(out);
    kept.deep.into(out);
    denoise::denoise(out, channels, output::denoisesBeauty(scene), logger);
    return out;
}

/** A film's state for a checkpoint, with every pixel's coverage and deep list. **/
film::State stateOf(const film::Film & picture, const internal::Kept & kept) {
    film::State out = picture.state();
    kept.coverage.into(out);
    kept.deep.into(out);
    return out;
}

/** The radius of a sphere about the box that bounds the world over the shutter, or zero. **/
float reach(const accel::LinearBox & bounds) {
    accel::Box all = bounds.start;
    all.grow(bounds.end);
    return all.empty() ? 0.0F : 0.5F * glm::length(all.upper - all.lower);
}

};  // namespace

kernel::integrate::Settings settingsOf(const scene::Scene & scene) {
    return kernel::integrate::Settings { .maxDepth = count(scene, "maxDepth"),
        .diffuseDepth = count(scene, "diffuseDepth"),
        .glossyDepth = count(scene, "glossyDepth"),
        .specularDepth = count(scene, "specularDepth"),
        .transmissionDepth = count(scene, "transmissionDepth"),
        .volumeDepth = count(scene, "volumeDepth"),
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
    return render(device, scene, geometry::gather(scene, logger), logger, config, checkpoints, profile, trails);
}

film::Snapshot render(device::CpuDevice & device, const scene::Scene & scene, const geometry::Prototypes & prototypes,
    const boost::shared_ptr<log::Logger> & logger, const colour::Config & config, const Checkpoints & checkpoints, Profile * profile,
    Trails * trails) {
    const kernel::camera::Camera camera = camera::fromScene(scene);
    const glm::uvec2 image { static_cast<std::uint32_t>(camera.resolution.x), static_cast<std::uint32_t>(camera.resolution.y) };
    const internal::Window window = internal::windowOf(option(scene, "crop").floatArray(), image, logger);
    // the film holds the window's pixels alone
    const glm::uvec2 resolution = window.size;
    const accel::Assembly assembly = accel::assemble(device, prototypes);
    const view::Tracing tracing(device, prototypes, assembly);
    const geometry::Surfaces surfaces(device, prototypes, config);
    const volume::Volumes volumes(device, prototypes);
    const texture::Textures textures(device, prototypes.images, texture::optionsOf(scene), config, logger);
    const light::Shading shading { .surfaces = surfaces.view(), .textures = textures.view(), .volumes = volumes.view() };
    const light::Lights lights(device, scene, prototypes, config, shading, reach(assembly.bounds), logger);

    const film::Filter filter = filterOf(device, scene);

    const kernel::sample::ZSobol sampler(std::max(count(scene, "samples"), 1U), image, 0, count(scene, "frame"), window.origin);
    const kernel::integrate::Settings settings = settingsOf(scene);
    const kernel::Function<device::cpu::PathParams> entry = device.module().paths;
    const float threshold = option(scene, "noiseThreshold").floatArray().front();
    const std::uint32_t least = count(scene, "minSamples");
    const std::vector<film::Channel> channels = output::filmChannelsOf(scene);
    film::Film picture(device, resolution, channels);
    const std::vector<output::Channel> outputs = output::channelsOf(scene);
    const internal::FirstHits firstHits(device, outputs, picture, prototypes.primvarNames, logger);
    const internal::LightPaths lightPaths(device, outputs, picture, lights.labels(), logger);
    internal::Kept kept { .coverage = internal::Coverage(resolution, outputs, scene, prototypes),
        .deep = internal::Deep(resolution, deepened(scene), option(scene, "deepTolerance").floatArray().front(), scene, prototypes) };
    const internal::Records records(device, resolution, kSamplesPerPass, !kept.coverage.none() || !kept.deep.none());
    const std::uint64_t print = checkpoints.path.empty() ? 0U : fingerprint(scene, channels);
    std::uint32_t done = checkpoints.resume ? resumed(checkpoints.path, channels, print, picture, kept) : 0U;
    std::uint32_t written = done;
    std::chrono::steady_clock::time_point last = std::chrono::steady_clock::now();
    std::optional<internal::Tallies> tallies;
    if (profile != nullptr && profile->counted) {
        tallies.emplace(device, resolution);
    }
    const internal::TrailSteps trail(device, resolution, trails != nullptr);
    double passing = 0.0;
    const std::uint32_t start =
        static_cast<std::uint32_t>(std::min<std::uint64_t>(std::uint64_t { done } * kSamplesPerPass, sampler.samplesPerPixel()));
    for (std::uint32_t first = start; first < sampler.samplesPerPixel(); first += kSamplesPerPass) {
        if (checkpoints.stop && checkpoints.stop(done)) {
            break;
        }
        const std::uint32_t samples = std::min(kSamplesPerPass, sampler.samplesPerPixel() - first);
        // a pass only adds samples that the noise check after the pass before it left sampling.
        // A resumed render repeats the check the render it continues made before this pass
        if (first != 0 && threshold > 0.0F && first >= least && picture.converge(threshold) == 0U) {
            break;
        }
        const std::chrono::steady_clock::time_point began = std::chrono::steady_clock::now();
        picture.pass([&](const kernel::film::Pixels & pixels) {
            const device::cpu::PathParams params { .pixels = pixels,
                .filter = filter.view(),
                .camera = camera,
                .sampler = sampler,
                .traversal = tracing.hierarchy(),
                .textures = textures.view(),
                .surfaces = surfaces.view(),
                .volumes = volumes.view(),
                .lights = lights.view(),
                .table = colour::acescgTable(),
                .albedos = material::ggxAlbedos(),
                .settings = settings,
                .first = first,
                .count = samples,
                .firstHits = firstHits.view(),
                .features = firstHits.features(),
                .trail = trail.view(),
                .expressions = lightPaths.view(),
                .covered = records.view() };
            if (tallies) {
                device.launch(resolution.x, resolution.y, device::cpu::ProfileParams { .paths = params, .tallies = tallies->view() }, device.module().profiled);
                return;
            }
            device.launch(resolution.x, resolution.y, params, entry);
        });
        kept.add(records.taken(samples), samples);
        ++done;
        const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        passing += std::chrono::duration<double>(now - began).count();
        if (!checkpoints.path.empty() && std::chrono::duration<double>(now - last).count() >= checkpoints.interval) {
            output::writeCheckpoint(checkpoints.path, stateOf(picture, kept), print);
            written = done;
            last = now;
        }
    }
    if (!checkpoints.path.empty() && written != done) {
        output::writeCheckpoint(checkpoints.path, stateOf(picture, kept), print);
    }
    profiled(profile, passing, kept, tallies);
    if (trails != nullptr) {
        trail.into(*trails, lights.labels());
    }
    textures.report();
    film::Snapshot out = finished(picture, kept, scene, outputs, logger);
    out.origin = window.origin;
    out.image = image;
    return out;
}

std::uint32_t passesOf(const scene::Scene & scene) {
    const std::uint32_t samples = kernel::sample::ZSobol(std::max(count(scene, "samples"), 1U), glm::uvec2(1U), 0, 0).samplesPerPixel();
    return (samples + kSamplesPerPass - 1U) / kSamplesPerPass;
}

};  // namespace v3d::moya::integrate
