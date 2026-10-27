/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "CameraCodec.h"

#include <vertical3d/src/scene/Camera.h>
#include <vertical3d/src/scene/Item.h>

#include <cstdint>
#include <string>
#include <string_view>

#include "Codec.h"
#include "Payload.h"

#include <boost/json.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>

namespace v3d::editor {

/**
 **/
Item::Kind CameraCodec::kind() const noexcept {
    return Item::Kind::Camera;
}

/**
 **/
std::string_view CameraCodec::name() const noexcept {
    return "camera";
}

/**
 **/
std::int64_t CameraCodec::version() const noexcept {
    return VERSION;
}

/**
 **/
boost::json::object CameraCodec::payload(const Item& item) const {
    const Camera::Lens& lens = static_cast<const Camera&>(item).lens();
    boost::json::object payload;
    payload["projection"] = lens.projection == Camera::Projection::Orthographic ? "orthographic" : "perspective";
    payload["fov"] = lens.fov;
    payload["extent"] = lens.extent;
    payload["clipping"] = boost::json::array{ lens.clipping.x, lens.clipping.y };
    return payload;
}

/**
 **/
boost::shared_ptr<Item> CameraCodec::read(const boost::json::object& payload, const Blobs& /*blobs*/, std::string* problem) const {
    if (!exactly(payload, { "projection", "fov", "extent", "clipping" }, problem)) {
        return nullptr;
    }
    Camera::Lens lens;
    const std::string_view projection = word(payload.at("projection"));
    if (projection == "perspective") {
        lens.projection = Camera::Projection::Perspective;
    } else if (projection == "orthographic") {
        lens.projection = Camera::Projection::Orthographic;
    } else {
        *problem = "its projection is neither perspective nor orthographic";
        return nullptr;
    }
    float clipping[2] = { 0.0F, 0.0F };
    if (!numbers(payload.at("fov"), 1, &lens.fov) || !numbers(payload.at("extent"), 1, &lens.extent) ||
        !numbers(payload.at("clipping"), 2, clipping)) {
        *problem = "a field of its lens is not a finite number";
        return nullptr;
    }
    lens.clipping = glm::vec2(clipping[0], clipping[1]);
    if (!lens.valid()) {
        *problem = "its lens is out of range";
        return nullptr;
    }
    const boost::shared_ptr<Camera> camera = boost::make_shared<Camera>();
    camera->lens(lens);
    return camera;
}

};  // namespace v3d::editor
