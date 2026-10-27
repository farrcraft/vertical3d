/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Codec.h"

#include <vertical3d/src/scene/Item.h>

#include <algorithm>
#include <string_view>
#include <vector>

#include "CameraCodec.h"
#include "GroupCodec.h"
#include "LightCodec.h"
#include "MeshCodec.h"

namespace v3d::editor {

/**
 **/
bool Codec::buffered() const noexcept {
    return false;
}

/**
 **/
std::vector<Buffer> Codec::buffers(const Item& /*item*/) const {
    return {};
}

/**
 **/
const Codec& codecOf(Item::Kind kind) {
    static const MeshCodec mesh;
    static const CameraCodec camera;
    static const LightCodec light;
    static const GroupCodec group;
    switch (kind) {
        case Item::Kind::Mesh:
            return mesh;
        case Item::Kind::Camera:
            return camera;
        case Item::Kind::Light:
            return light;
        case Item::Kind::Group:
            break;
    }
    return group;
}

/**
 **/
const std::vector<const Codec*>& codecs() {
    static const std::vector<const Codec*> all{ &codecOf(Item::Kind::Mesh), &codecOf(Item::Kind::Camera),
        &codecOf(Item::Kind::Light), &codecOf(Item::Kind::Group) };
    return all;
}

/**
 **/
const Codec* codecNamed(std::string_view name) {
    const std::vector<const Codec*>& all = codecs();
    const auto found = std::ranges::find_if(all, [name](const Codec* codec) { return codec->name() == name; });
    return found == all.end() ? nullptr : *found;
}

};  // namespace v3d::editor
