/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/device/Buffer.h>
#include <moya/libmoya/device/Device.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/kernel/light/Lights.h>
#include <moya/libmoya/scene/Scene.h>

#include <cstdint>

#include <boost/shared_ptr.hpp>

namespace v3d::log {
class Logger;
};  // namespace v3d::log

/** The scene's lights, made ready for the integrator. Host code. **/
namespace v3d::moya::light {

/**
 * The scene's lights, in a device's memory as the integrator reads them: the lights with a shape
 * that the prototypes place, then the points and the distant lights in the order of their indices.
 * Each light's emission is UsdLux's: its intensity times two to its exposure, times its colour as
 * an illuminant, converted into ACEScg from the space it names. With a colour temperature, the
 * colour tints a Planckian radiator at that temperature of a luminance of one.
 *
 * - **A light with a shape** emits that as a radiance. With normalize, it is divided by the
 *   light's area in the world, as it stands when the shutter opens, so its power is the same at
 *   any size. A light of no area is not divided. A rect or disk light emits from the face its -z
 *   axis points to, and a sphere or cylinder light from its outer face.
 * - **A point** is a sphere light of no radius, or one treated as a point. It emits the radiant
 *   intensity of the sphere it stands for: its radiance times pi times its radius squared. With
 *   normalize, that is a quarter of its emission. One of no radius without normalize emits
 *   nothing.
 * - **A mesh light** emits a radiance from the triangles of the geometry it names, wherever a
 *   chain of instances from the world places that geometry, from the side each triangle's winding
 *   faces. Each chain is a light of its own. With normalize, its radiance is divided by the area in
 *   the world of the triangles that chain places, as they stand when the shutter opens. Its
 *   transform and its cone are not read. A geometry that more than one mesh light names emits by
 *   the first, in the order of their indices, and one of no triangles emits nothing.
 * - **A distant light** shines along its -z axis. Of no angle, its emission is the irradiance of
 *   a plane that faces it. With an angle, its emission is a radiance over the cone of that
 *   diameter, held to [0, 360] degrees. With normalize, the radiance is divided by pi sin^2 of the
 *   half angle, or pi (2 - sin^2) past a quarter turn, so the irradiance stays the same at any
 *   angle.
 *
 * Each light is drawn in proportion to its power, which leaves its cone out. A light with a shape
 * has its radiance's luminance times its area times pi, and a point its intensity's times four
 * pi. A distant light has its irradiance's luminance times the area of a disk of the reach. A
 * mesh light has its radiance's luminance times its area times pi, and its triangle is then drawn
 * in proportion to its area.
 *
 * The light tree is built over the lights with a place, with a subtree over the triangles of each
 * geometry a mesh light makes emit. Each light is bounded over the shutter by its power, its box
 * and the directions it emits along. It depends on the scene alone, so every run builds the same
 * tree.
 *
 * Light linking decides which lights light each shading point, as the scene's includeRoot and
 * instances say. Each set of lights that some shading point is lit by has a top tree of its own,
 * and the mesh lights' subtrees are shared among them. A light whose shadowIncludeRoot is 0 casts
 * shadows only from the instances its shadowInstances names.
 *
 * A cone angle is held to [0, 180] degrees and a softness to [0, 1].
 *
 * It reads the device's memory, so the device outlives it.
 **/
class Lights final {
 public:
    /**
     * reach is the radius of a sphere that bounds the world, which a distant light's power is
     * measured over. The logger, which may be null, reports a mesh light that emits nothing.
     *
     * @throws std::runtime_error, from OpenColorIO, when a light's colour names a space the
     *         configuration cannot convert into ACEScg
     **/
    Lights(device::Device & device, const scene::Scene & scene, const geometry::Prototypes & prototypes, const colour::Config & config, float reach,
        const boost::shared_ptr<log::Logger> & logger);

    /** The lights, as kernel code reads them. **/
    kernel::light::Lights view() const;

 private:
    boost::shared_ptr<device::Buffer> lights_;
    boost::shared_ptr<device::Buffer> keys_;
    boost::shared_ptr<device::Buffer> distant_;
    boost::shared_ptr<device::Buffer> byId_;
    boost::shared_ptr<device::Buffer> cdf_;
    boost::shared_ptr<device::Buffer> triangles_;
    boost::shared_ptr<device::Buffer> meshes_;
    boost::shared_ptr<device::Buffer> images_;
    boost::shared_ptr<device::Buffer> texels_;
    boost::shared_ptr<device::Buffer> sums_;
    boost::shared_ptr<device::Buffer> domes_;
    boost::shared_ptr<device::Buffer> nodes_;
    boost::shared_ptr<device::Buffer> roots_;
    boost::shared_ptr<device::Buffer> leaves_;
    boost::shared_ptr<device::Buffer> triangleLeaves_;
    boost::shared_ptr<device::Buffer> infinite_;
    boost::shared_ptr<device::Buffer> infiniteFirst_;
    boost::shared_ptr<device::Buffer> linkFirst_;
    boost::shared_ptr<device::Buffer> linkSteps_;
    boost::shared_ptr<device::Buffer> lit_;
    boost::shared_ptr<device::Buffer> casters_;
    boost::shared_ptr<device::Buffer> profiles_;
    boost::shared_ptr<device::Buffer> photometry_;
    std::uint32_t count_ { 0 };
};

};  // namespace v3d::moya::light
