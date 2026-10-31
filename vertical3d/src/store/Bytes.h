/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vertical3d/src/store/Blob.h>

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <glm/vec3.hpp>

namespace v3d::editor::store {

static_assert(std::endian::native == std::endian::little, "scene blobs are little-endian");

/**
 * Append a number to a blob, in the machine's order, which is little-endian.
 **/
template <typename T>
void put(std::vector<std::byte>* bytes, T value) {
    const std::array<std::byte, sizeof(T)> raw = std::bit_cast<std::array<std::byte, sizeof(T)>>(value);
    bytes->insert(bytes->end(), raw.begin(), raw.end());
}

/**
 * Append a vector's three numbers to a blob.
 **/
void put(std::vector<std::byte>* bytes, const glm::vec3& value);

/**
 * @return whether a blob holds exactly its count of entries of a size, and no more than a mesh
 *         index can number. A blob that does not names its role in the problem
 **/
bool sized(const Blob& blob, std::size_t size, const char* role, std::string* problem);

/**
 * @return whether all three numbers of a vector are finite
 **/
bool finite(const glm::vec3& value);

};  // namespace v3d::editor::store
