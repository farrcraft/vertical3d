/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/brep/BRep.h>
#include <vertical3d/src/scene/Item.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "Codec.h"

#include <boost/json.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 * A mesh's payload is empty, and its topology is three blobs of little-endian arrays:
 *
 * - vertices: each its point, three floats
 * - edges: each half edge's vertex, face, pair and next, four 32 bit indices
 * - faces: each its normal, three floats, then its first half edge, a 32 bit index
 *
 * A read builds the mesh index for index, and refuses one whose references reach past what it
 * holds.
 **/
class MeshCodec final : public Codec {
 public:
    static constexpr std::int64_t VERSION = 1;

    Item::Kind kind() const noexcept override;
    std::string_view name() const noexcept override;
    std::int64_t version() const noexcept override;
    bool buffered() const noexcept override;
    std::vector<Buffer> buffers(const Item& item) const override;
    boost::json::object payload(const Item& item) const override;
    boost::shared_ptr<Item> read(const boost::json::object& payload, const Blobs& blobs, std::string* problem) const override;

 private:
    /**
     * The size of one entry of each of a mesh's three blobs. A vertex is its point. A half edge is
     * its vertex, face, pair and next. A face is its normal and its first half edge.
     **/
    static constexpr std::size_t VERTEX_SIZE = 3 * sizeof(float);
    static constexpr std::size_t EDGE_SIZE = 4 * sizeof(std::uint32_t);
    static constexpr std::size_t FACE_SIZE = (3 * sizeof(float)) + sizeof(std::uint32_t);

    /**
     * Build a mesh from its three blobs, index for index.
     **/
    static bool decode(const Blobs& blobs, v3d::brep::BRep* mesh, std::string* problem);
};

};  // namespace v3d::editor
