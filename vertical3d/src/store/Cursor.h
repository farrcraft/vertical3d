/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <array>
#include <bit>
#include <cstddef>
#include <cstring>
#include <span>

#include <glm/vec3.hpp>

namespace v3d::editor {

/**
 * Reads a blob's numbers in the order they were written. The caller has checked that the blob
 * holds every number it reads.
 **/
class Cursor final {
 public:
    explicit Cursor(std::span<const std::byte> bytes);

    template <typename T>
    T next() {
        std::array<std::byte, sizeof(T)> raw{};
        std::memcpy(raw.data(), bytes_.subspan(offset_, sizeof(T)).data(), sizeof(T));
        offset_ += sizeof(T);
        return std::bit_cast<T>(raw);
    }

    glm::vec3 vector();

 private:
    std::span<const std::byte> bytes_;
    std::size_t offset_ = 0;
};

};  // namespace v3d::editor
