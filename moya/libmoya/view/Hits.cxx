/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/view/Hits.h>

#include <moya/libmoya/camera/Camera.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/film/Film.h>
#include <moya/libmoya/geometry/World.h>
#include <moya/libmoya/kernel/camera/Camera.h>
#include <moya/libmoya/kernel/film/Pixels.h>
#include <moya/libmoya/kernel/geometry/BruteForce.h>
#include <moya/libmoya/kernel/view/HitView.h>
#include <moya/libmoya/output/Exr.h>
#include <moya/libmoya/scene/Scene.h>

#include <cstdint>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <glm/vec2.hpp>

namespace v3d::moya::view {

film::Snapshot hits(device::CpuDevice & device, const scene::Scene & scene, const boost::shared_ptr<log::Logger> & logger, float time) {
    const kernel::camera::Camera camera = camera::fromScene(scene);
    const geometry::World world(device, geometry::flatten(scene, logger));
    const glm::uvec2 resolution { static_cast<std::uint32_t>(camera.resolution.x), static_cast<std::uint32_t>(camera.resolution.y) };
    film::Film picture(device, resolution,
        { { .name = kDepth, .components = 1 }, { .name = kNormal, .components = 3 }, { .name = kInstance, .components = 1 },
            { .name = kFace, .components = 1 }, { .name = kCoverage, .components = 1 } });
    const kernel::view::HitChannels channels { .depth = picture.offset(kDepth), .normal = picture.offset(kNormal),
        .instance = picture.offset(kInstance), .face = picture.offset(kFace), .coverage = picture.offset(kCoverage) };
    picture.pass([&](const kernel::film::Pixels & pixels) {
        const kernel::view::HitView<kernel::geometry::BruteForce> params { .pixels = pixels, .channels = channels, .camera = camera,
            .traversal = { .placed = world.view() }, .surfaces = world.view(), .time = time };
        device.launch(resolution.x, resolution.y, params, &kernel::view::hits<kernel::geometry::BruteForce>);
    });
    return picture.snapshot();
}

void writeHits(const std::string & path, const film::Snapshot & snapshot) {
    output::writeExr(path, snapshot,
        { { .name = "", .outputs = { std::string(output::kBeauty), kDepth, kNormal, kInstance, kFace, kCoverage },
            .precision = output::Precision::Float } },
        {});
}

};  // namespace v3d::moya::view
