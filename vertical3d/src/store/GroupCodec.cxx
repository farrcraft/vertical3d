/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "GroupCodec.h"

#include <vertical3d/src/scene/Group.h>
#include <vertical3d/src/scene/Item.h>

#include <cstdint>
#include <string>
#include <string_view>

#include "Codec.h"
#include "Payload.h"

#include <boost/json.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 **/
Item::Kind GroupCodec::kind() const noexcept {
    return Item::Kind::Group;
}

/**
 **/
std::string_view GroupCodec::name() const noexcept {
    return "group";
}

/**
 **/
std::int64_t GroupCodec::version() const noexcept {
    return VERSION;
}

/**
 **/
boost::json::object GroupCodec::payload(const Item& /*item*/) const {
    return boost::json::object();
}

/**
 **/
boost::shared_ptr<Item> GroupCodec::read(const boost::json::object& payload, const Blobs& /*blobs*/, std::string* problem) const {
    return exactly(payload, {}, problem) ? boost::make_shared<Group>() : nullptr;
}

};  // namespace v3d::editor
