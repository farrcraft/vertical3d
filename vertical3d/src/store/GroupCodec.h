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

namespace v3d::editor::store {

/**
 * A group adds nothing to what every item has, so its payload is empty.
 **/
class GroupCodec final : public Codec {
 public:
    static constexpr std::int64_t VERSION = 1;

    scene::Item::Kind kind() const noexcept override;
    std::string_view name() const noexcept override;
    std::int64_t version() const noexcept override;
    boost::json::object payload(const scene::Item& item) const override;
    boost::shared_ptr<scene::Item> read(const boost::json::object& payload, const Blobs& blobs, std::string* problem) const override;
};

};  // namespace v3d::editor::store
