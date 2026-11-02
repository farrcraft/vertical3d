/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Codec.h"

#include <vertical3d/src/scene/item/Item.h>

#include <algorithm>
#include <string_view>
#include <vector>

#include "CameraCodec.h"
#include "CurveCodec.h"
#include "GroupCodec.h"
#include "LightCodec.h"
#include "MeshCodec.h"

namespace v3d::editor::store {

/**
 **/
bool Codec::buffered() const noexcept {
    return false;
}

/**
 **/
std::vector<Buffer> Codec::buffers(const scene::item::Item& /*item*/) const {
    return {};
}

/**
 **/
const Codec& codecOf(scene::item::Item::Kind kind) {
    static const MeshCodec mesh;
    static const CameraCodec camera;
    static const LightCodec light;
    static const GroupCodec group;
    static const CurveCodec curve;
    switch (kind) {
        case scene::item::Item::Kind::Mesh:
            return mesh;
        case scene::item::Item::Kind::Camera:
            return camera;
        case scene::item::Item::Kind::Light:
            return light;
        case scene::item::Item::Kind::Curve:
            return curve;
        case scene::item::Item::Kind::Group:
            break;
    }
    return group;
}

/**
 **/
const std::vector<const Codec*>& codecs() {
    static const std::vector<const Codec*> all{ &codecOf(scene::item::Item::Kind::Mesh), &codecOf(scene::item::Item::Kind::Camera),
        &codecOf(scene::item::Item::Kind::Light), &codecOf(scene::item::Item::Kind::Group), &codecOf(scene::item::Item::Kind::Curve) };
    return all;
}

/**
 **/
const Codec* codecNamed(std::string_view name) {
    const std::vector<const Codec*>& all = codecs();
    const auto found = std::ranges::find_if(all, [name](const Codec* codec) { return codec->name() == name; });
    return found == all.end() ? nullptr : *found;
}

};  // namespace v3d::editor::store
