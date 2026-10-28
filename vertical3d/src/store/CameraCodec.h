/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vertical3d/src/scene/Item.h>

#include <cstdint>
#include <string>
#include <string_view>

#include "Codec.h"

#include <boost/json.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 * A camera's payload is its lens: its projection, its angle of view, its orthographic extent and
 * its two clipping distances. A lens a read finds out of range is refused.
 **/
class CameraCodec final : public Codec {
 public:
    static constexpr std::int64_t VERSION = 1;

    Item::Kind kind() const noexcept override;
    std::string_view name() const noexcept override;
    std::int64_t version() const noexcept override;
    boost::json::object payload(const Item& item) const override;
    boost::shared_ptr<Item> read(const boost::json::object& payload, const Blobs& blobs, std::string* problem) const override;
};

};  // namespace v3d::editor
