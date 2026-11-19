/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/volume/Volumes.h>

#include <moya/libmoya/device/Buffer.h>
#include <moya/libmoya/device/Device.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/kernel/volume/Block.h>
#include <moya/libmoya/kernel/volume/GridPlace.h>
#include <moya/libmoya/kernel/volume/Volume.h>
#include <moya/libmoya/kernel/volume/Volumes.h>
#include <moya/libmoya/volume/Grid.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::moya::volume {

template <typename T>
boost::shared_ptr<device::Buffer> Volumes::uploaded(device::Device & device, const std::vector<T> & values) {
    const std::size_t bytes = values.size() * sizeof(T);
    auto buffer = device.allocate(bytes);
    device.upload(*buffer, values.data(), bytes);
    return buffer;
}

Volumes::Volumes(device::Device & device, const geometry::Prototypes & prototypes) {
    std::vector<kernel::volume::GridPlace> places;
    std::vector<kernel::volume::Block> blocks;
    places.reserve(prototypes.grids.size());
    for (const boost::shared_ptr<const Grid> & grid : prototypes.grids) {
        places.push_back(kernel::volume::GridPlace { .first = static_cast<std::uint32_t>(blocks.size()), .count = static_cast<std::uint32_t>(grid->blocks.size()) });
        blocks.insert(blocks.end(), grid->blocks.begin(), grid->blocks.end());
    }
    volumes_ = uploaded(device, prototypes.volumes);
    grids_ = uploaded(device, places);
    blocks_ = uploaded(device, blocks);
}

kernel::volume::Volumes Volumes::view() const {
    return kernel::volume::Volumes { .volumes = device::view<kernel::volume::Volume>(*volumes_),
        .grids = device::view<kernel::volume::GridPlace>(*grids_),
        .blocks = device::view<kernel::volume::Block>(*blocks_) };
}

};  // namespace v3d::moya::volume
