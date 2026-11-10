/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/integrate/Render.h>

#include <api/render/offline/film/Sampling.h>
#include <moya/libmoya/accel/Assembly.h>
#include <moya/libmoya/accel/Box.h>
#include <moya/libmoya/accel/Sweep.h>
#include <moya/libmoya/camera/Camera.h>
#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/colour/RgbTables.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/device/cpu/Module.h>
#include <moya/libmoya/film/Film.h>
#include <moya/libmoya/film/Filter.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/geometry/Surfaces.h>
#include <moya/libmoya/kernel/camera/Camera.h>
#include <moya/libmoya/kernel/film/Pixels.h>
#include <moya/libmoya/kernel/integrate/Settings.h>
#include <moya/libmoya/kernel/Launch.h>
#include <moya/libmoya/kernel/sample/ZSobol.h>
#include <moya/libmoya/light/Lights.h>
#include <moya/libmoya/scene/Declaration.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Schema.h>
#include <moya/libmoya/scene/Value.h>
#include <moya/libmoya/view/Tracing.h>

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::integrate {

namespace {

/** A parameter of the scene's options, or its fallback while the scene has none. **/
const scene::Value & option(const scene::Scene & scene, const std::string & name) {
    const std::vector<scene::Handle> options = scene.objects("options");
    if (!options.empty()) {
        return scene.value(options.front(), name);
    }
    for (const scene::Declaration & declaration : scene.schema().kind(*scene.schema().find("options")).parameters) {
        if (declaration.name == name) {
            return declaration.fallback;
        }
    }
    throw std::invalid_argument("the options have no parameter " + name);
}

/** An integer option, held to zero or more. **/
std::uint32_t count(const scene::Scene & scene, const std::string & name) {
    return static_cast<std::uint32_t>(std::max(option(scene, name).integerArray().front(), 0));
}

/** The strategy an option names: multiple importance sampling unless it names another. **/
kernel::integrate::Strategy strategyNamed(const std::string & name) {
    if (name == "light") {
        return kernel::integrate::Strategy::Light;
    }
    return name == "bsdf" ? kernel::integrate::Strategy::Bsdf : kernel::integrate::Strategy::Mis;
}

/** The radius of a sphere about the box that bounds the world over the shutter, or zero. **/
float reach(const accel::LinearBox & bounds) {
    accel::Box all = bounds.start;
    all.grow(bounds.end);
    return all.empty() ? 0.0F : 0.5F * glm::length(all.upper - all.lower);
}

};  // namespace

kernel::integrate::Settings settingsOf(const scene::Scene & scene, const colour::Config & config) {
    const scene::Value & environment = option(scene, "environment");
    const std::vector<float> & floats = environment.floatArray();
    return kernel::integrate::Settings { .maxDepth = count(scene, "maxDepth"),
        .diffuseDepth = count(scene, "diffuseDepth"),
        .glossyDepth = count(scene, "glossyDepth"),
        .specularDepth = count(scene, "specularDepth"),
        .transmissionDepth = count(scene, "transmissionDepth"),
        .rouletteDepth = count(scene, "rouletteDepth"),
        .environment = config.input(environment.space()).apply(glm::vec3 { floats[0], floats[1], floats[2] }),
        .strategy = strategyNamed(option(scene, "lightSampling").stringArray().front()) };
}

film::Snapshot render(device::CpuDevice & device, const scene::Scene & scene, const boost::shared_ptr<log::Logger> & logger,
    const colour::Config & config) {
    const kernel::camera::Camera camera = camera::fromScene(scene);
    const glm::uvec2 resolution { static_cast<std::uint32_t>(camera.resolution.x), static_cast<std::uint32_t>(camera.resolution.y) };
    const geometry::Prototypes prototypes = geometry::gather(scene, logger);
    const accel::Assembly assembly = accel::assemble(device, prototypes);
    const view::Tracing tracing(device, prototypes, assembly);
    const geometry::Surfaces surfaces(device, prototypes, config);
    const light::Lights lights(device, scene, prototypes, config, reach(assembly.bounds));

    render::offline::film::Filter kind = render::offline::film::Filter::Gaussian;
    render::offline::film::filterNamed(option(scene, "filter").stringArray().front(), &kind);
    const std::vector<float> & width = option(scene, "filterWidth").floatArray();
    const film::Filter filter(device, kind, glm::vec2 { width[0], width[1] });

    const kernel::sample::ZSobol sampler(std::max(count(scene, "samples"), 1U), resolution, 0, count(scene, "frame"));
    const kernel::integrate::Settings settings = settingsOf(scene, config);
    const kernel::Function<device::cpu::PathParams> entry = device.module().paths;
    film::Film picture(device, resolution, {});
    for (std::uint32_t first = 0; first < sampler.samplesPerPixel(); first += kSamplesPerPass) {
        const std::uint32_t samples = std::min(kSamplesPerPass, sampler.samplesPerPixel() - first);
        picture.pass([&](const kernel::film::Pixels & pixels) {
            const device::cpu::PathParams params { .pixels = pixels,
                .filter = filter.view(),
                .camera = camera,
                .sampler = sampler,
                .traversal = tracing.hierarchy(),
                .surfaces = surfaces.view(),
                .lights = lights.view(),
                .table = colour::acescgTable(),
                .settings = settings,
                .first = first,
                .count = samples };
            device.launch(resolution.x, resolution.y, params, entry);
        });
    }
    return picture.snapshot();
}

};  // namespace v3d::moya::integrate
