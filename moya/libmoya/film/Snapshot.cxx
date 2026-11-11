/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/film/Snapshot.h>

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace v3d::moya::film {

float Snapshot::at(std::uint32_t x, std::uint32_t y, std::uint32_t offset) const {
    return values.at((((static_cast<std::size_t>(y) * resolution.x) + x) * stride) + offset);
}

float Snapshot::error(std::uint32_t x, std::uint32_t y) const {
    return errors.at((static_cast<std::size_t>(y) * resolution.x) + x);
}

std::uint32_t Snapshot::count(std::uint32_t x, std::uint32_t y) const {
    return samples.at((static_cast<std::size_t>(y) * resolution.x) + x);
}

std::uint32_t Snapshot::offset(const std::string & name) const {
    for (std::size_t i = 0; i < channels.size(); ++i) {
        if (channels[i].name == name) {
            return offsets.at(i);
        }
    }
    throw std::out_of_range("the snapshot has no channel named " + name);
}

};  // namespace v3d::moya::film
