/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "LightCodec.h"

#include <vertical3d/src/scene/item/Item.h>
#include <vertical3d/src/scene/item/Light.h>

#include <cstdint>
#include <string>
#include <string_view>

#include "Codec.h"
#include "Payload.h"

#include <boost/json.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <glm/vec3.hpp>

namespace v3d::editor::store {

/**
 **/
scene::item::Item::Kind LightCodec::kind() const noexcept {
    return scene::item::Item::Kind::Light;
}

/**
 **/
std::string_view LightCodec::name() const noexcept {
    return "light";
}

/**
 **/
std::int64_t LightCodec::version() const noexcept {
    return VERSION;
}

/**
 **/
boost::json::object LightCodec::payload(const scene::item::Item& item) const {
    const scene::item::Light::Emission& emission = static_cast<const scene::item::Light&>(item).emission();
    boost::json::object payload;
    switch (emission.type) {
        case scene::item::Light::Type::Point:
            payload["type"] = "point";
            break;
        case scene::item::Light::Type::Distant:
            payload["type"] = "distant";
            break;
        case scene::item::Light::Type::Spot:
            payload["type"] = "spot";
            break;
    }
    payload["colour"] = vector(emission.colour);
    payload["intensity"] = emission.intensity;
    payload["cone"] = emission.cone;
    payload["delta"] = emission.delta;
    return payload;
}

/**
 **/
boost::shared_ptr<scene::item::Item> LightCodec::read(const boost::json::object& payload, const Blobs& /*blobs*/, std::string* problem) const {
    if (!exactly(payload, { "type", "colour", "intensity", "cone", "delta" }, problem)) {
        return nullptr;
    }
    scene::item::Light::Emission emission;
    const std::string_view type = word(payload.at("type"));
    if (type == "point") {
        emission.type = scene::item::Light::Type::Point;
    } else if (type == "distant") {
        emission.type = scene::item::Light::Type::Distant;
    } else if (type == "spot") {
        emission.type = scene::item::Light::Type::Spot;
    } else {
        *problem = "its type is not point, distant or spot";
        return nullptr;
    }
    float colour[3] = { 0.0F, 0.0F, 0.0F };
    if (!numbers(payload.at("colour"), 3, colour) || !numbers(payload.at("intensity"), 1, &emission.intensity) ||
        !numbers(payload.at("cone"), 1, &emission.cone) || !numbers(payload.at("delta"), 1, &emission.delta)) {
        *problem = "a field of its emission is not a finite number";
        return nullptr;
    }
    emission.colour = glm::vec3(colour[0], colour[1], colour[2]);
    if (!emission.valid()) {
        *problem = "its emission is out of range";
        return nullptr;
    }
    const boost::shared_ptr<scene::item::Light> light = boost::make_shared<scene::item::Light>();
    light->emission(emission);
    return light;
}

};  // namespace v3d::editor::store
