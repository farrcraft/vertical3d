/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/geometry/Surfaces.h>

#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/colour/Transform.h>
#include <moya/libmoya/device/Buffer.h>
#include <moya/libmoya/device/Device.h>
#include <moya/libmoya/geometry/ConvertedMaterials.h>
#include <moya/libmoya/geometry/Geometry.h>
#include <moya/libmoya/geometry/Instance.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/Key.h>
#include <moya/libmoya/kernel/geometry/primitive/Quadric.h>
#include <moya/libmoya/kernel/surface/Geometry.h>
#include <moya/libmoya/kernel/surface/Instance.h>
#include <moya/libmoya/kernel/program/Instruction.h>
#include <moya/libmoya/kernel/surface/Material.h>
#include <moya/libmoya/kernel/surface/Primvar.h>
#include <moya/libmoya/kernel/surface/Surfaces.h>

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::moya::geometry {

namespace {

template <typename T>
boost::shared_ptr<device::Buffer> uploaded(device::Device & device, const std::vector<T> & values) {
    const std::size_t bytes = values.size() * sizeof(T);
    auto buffer = device.allocate(bytes);
    device.upload(*buffer, values.data(), bytes);
    return buffer;
}

/** Every primvar's values, with each colour converted into ACEScg from the space it names. **/
std::vector<float> convertedValues(const Prototypes & prototypes, const colour::Config & config) {
    std::vector<float> out = prototypes.primvarValues;
    std::map<std::string, colour::Transform> transforms;
    for (std::size_t i = 0; i < prototypes.primvars.size(); ++i) {
        const kernel::surface::Primvar & primvar = prototypes.primvars[i];
        if (!prototypes.primvarSpaces[i]) {
            continue;
        }
        const std::string & space = *prototypes.primvarSpaces[i];
        auto found = transforms.find(space);
        if (found == transforms.end()) {
            found = transforms.emplace(space, config.input(space)).first;
        }
        // a primvar's values run up to the next one's first
        const std::size_t end = i + 1 < prototypes.primvars.size() ? prototypes.primvars[i + 1].first : out.size();
        for (std::size_t at = primvar.first; at + 3 <= end; at += 3) {
            const glm::vec3 converted = found->second.apply(glm::vec3 { out[at], out[at + 1], out[at + 2] });
            out[at] = converted.x;
            out[at + 1] = converted.y;
            out[at + 2] = converted.z;
        }
    }
    return out;
}

};  // namespace

Surfaces::Surfaces(device::Device & device, const Prototypes & prototypes, const colour::Config & config) :
    open_(prototypes.open), close_(prototypes.close) {
    std::vector<kernel::surface::Geometry> geometries;
    geometries.reserve(prototypes.geometries.size());
    for (const Geometry & geometry : prototypes.geometries) {
        geometries.push_back(kernel::surface::Geometry { .form = geometry.form,
            .points = geometry.points,
            .widths = geometry.widths,
            .quadric = geometry.quadric,
            .controls = geometry.controls,
            .ends = geometry.ends,
            .faces = geometry.first,
            .primvars = geometry.primvars,
            .primvarCount = geometry.primvarCount });
    }
    std::vector<kernel::geometry::Key> keys;
    std::vector<kernel::surface::Instance> instances;
    std::uint32_t most = 0;
    for (const Instance & instance : prototypes.instances) {
        instances.push_back(kernel::surface::Instance { .transform = instance.transform,
            .keys = static_cast<std::uint32_t>(keys.size()),
            .keyCount = static_cast<std::uint32_t>(instance.keys.size()),
            .geometry = instance.kind == Prototype::Geometry ? instance.prototype : kernel::surface::kGroup,
            .material = instance.material });
        keys.insert(keys.end(), instance.keys.begin(), instance.keys.end());
        most = instance.id + 1U > most ? instance.id + 1U : most;
    }
    std::vector<std::uint32_t> byId(most, kernel::geometry::kNoPrimitive);
    for (std::size_t i = 0; i < prototypes.instances.size(); ++i) {
        byId[prototypes.instances[i].id] = static_cast<std::uint32_t>(i);
    }
    points_ = uploaded(device, prototypes.points);
    widths_ = uploaded(device, prototypes.widths);
    quadrics_ = uploaded(device, prototypes.quadrics);
    controls_ = uploaded(device, prototypes.controls);
    ends_ = uploaded(device, prototypes.ends);
    times_ = uploaded(device, prototypes.times);
    faces_ = uploaded(device, prototypes.faces);
    geometries_ = uploaded(device, geometries);
    keys_ = uploaded(device, keys);
    instances_ = uploaded(device, instances);
    byId_ = uploaded(device, byId);
    const ConvertedMaterials converted = ConvertedMaterials::of(prototypes, config);
    materials_ = uploaded(device, converted.materials);
    instructions_ = uploaded(device, prototypes.instructions);
    programValues_ = uploaded(device, converted.programValues);
    primvars_ = uploaded(device, prototypes.primvars);
    values_ = uploaded(device, convertedValues(prototypes, config));
}

kernel::surface::Surfaces Surfaces::view() const {
    return kernel::surface::Surfaces { .points = device::view<glm::vec3>(*points_),
        .widths = device::view<float>(*widths_),
        .quadrics = device::view<kernel::geometry::primitive::Quadric>(*quadrics_),
        .controls = device::view<glm::vec4>(*controls_),
        .ends = device::view<std::uint32_t>(*ends_),
        .times = device::view<float>(*times_),
        .faces = device::view<std::uint32_t>(*faces_),
        .geometries = device::view<kernel::surface::Geometry>(*geometries_),
        .keys = device::view<kernel::geometry::Key>(*keys_),
        .instances = device::view<kernel::surface::Instance>(*instances_),
        .byId = device::view<std::uint32_t>(*byId_),
        .primvars = device::view<kernel::surface::Primvar>(*primvars_),
        .materials = device::view<kernel::surface::Material>(*materials_),
        .instructions = device::view<kernel::program::Instruction>(*instructions_),
        .programValues = device::view<float>(*programValues_),
        .values = device::view<float>(*values_),
        .open = open_,
        .close = close_ };
}

};  // namespace v3d::moya::geometry
