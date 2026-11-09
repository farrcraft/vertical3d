/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/camera/Camera.h>
#include <moya/libmoya/kernel/film/Pixels.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/Placed.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/geometry/Traversal.h>
#include <moya/libmoya/kernel/Launch.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>
#include <moya/libmoya/kernel/spectrum/Wavelengths.h>
#include <moya/libmoya/kernel/view/HitChannels.h>

#include <cstdint>
#include <limits>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

/** Pictures of the scene that light nothing, for looking at what a traversal finds. **/
namespace v3d::moya::kernel::view {

/**
 * What the hit view's kernel function reads: the film, the camera, a traversal, and the placed
 * surfaces, its triangles, points and quadrics, which give a hit's normal and face whichever
 * traversal found it.
 **/
template <geometry::Traversal T>
struct HitView {
    film::Pixels pixels;
    HitChannels channels;
    camera::Camera camera;
    T traversal;
    geometry::Placed surfaces;

    /** Where the rays are drawn over the shutter, from 0 at its opening to 1 at its close. **/
    float time { 0.0F };
};

/**
 * One pixel of the hit view: a ray through the pixel's centre and the lens's, and what it hit.
 *
 * The beauty is black. The channels hold the hit's depth, its geometric normal, its innermost
 * instance, the face of its mesh, and a coverage of one. A pixel whose ray met nothing holds an
 * infinite depth, a zero normal, an instance and a face of -1, and no coverage.
 **/
template <geometry::Traversal T>
MOYA_KERNEL void hits(const HitView<T> & view, Item item) {
    const glm::vec2 centre = glm::vec2 { static_cast<float>(item.x), static_cast<float>(item.y) } + 0.5F;
    const geometry::Ray ray = view.camera.ray(centre, glm::vec2 { 0.5F }, view.time);
    const geometry::Hit hit = view.traversal.nearest(ray);

    view.pixels.add(item, 1.0F, spectrum::SampledSpectrum(0.0F), spectrum::SampledWavelengths::visible(0.5F));
    if (!hit.found()) {
        view.pixels.addChannel(item, view.channels.depth, 1.0F, std::numeric_limits<float>::infinity());
        view.pixels.addChannel(item, view.channels.instance, 1.0F, -1.0F);
        view.pixels.addChannel(item, view.channels.face, 1.0F, -1.0F);
        return;
    }
    const std::uint32_t face = view.surfaces.face(hit);
    view.pixels.addChannel(item, view.channels.depth, 1.0F, hit.distance);
    view.pixels.addChannel(item, view.channels.normal, 1.0F, view.surfaces.normal(hit, ray));
    view.pixels.addChannel(item, view.channels.instance, 1.0F, hit.depth == 0U ? -1.0F : static_cast<float>(hit.instances[hit.depth - 1U]));
    view.pixels.addChannel(item, view.channels.face, 1.0F, face == geometry::kNoPrimitive ? -1.0F : static_cast<float>(face));
    view.pixels.addChannel(item, view.channels.coverage, 1.0F, 1.0F);
}

};  // namespace v3d::moya::kernel::view
