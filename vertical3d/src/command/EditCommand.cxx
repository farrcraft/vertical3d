/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "EditCommand.h"

#include <vertical3d/src/scene/Camera.h>
#include <vertical3d/src/scene/Curve.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Light.h>
#include <vertical3d/src/scene/Properties.h>

#include <string>

#include "Placement.h"

#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 **/
EditCommand::Values EditCommand::of(const Item& item) {
    Values made{ .name = item.name(), .pose = pose(item) };
    if (item.kind() == Item::Kind::Curve) {
        made.style = static_cast<const Curve&>(item).style();
    }
    return made;
}

/**
 **/
void EditCommand::apply(Item* item, const Values& values) {
    item->name(values.name);
    item->placement(values.pose.placement);
    switch (item->kind()) {
        case Item::Kind::Camera:
            static_cast<Camera*>(item)->lens(values.pose.lens);
            break;
        case Item::Kind::Light:
            static_cast<Light*>(item)->emission(values.pose.emission);
            break;
        case Item::Kind::Curve:
            static_cast<Curve*>(item)->style(values.style);
            break;
        case Item::Kind::Mesh:
        case Item::Kind::Group:
            break;
    }
}

/**
 **/
bool EditCommand::same(const Values& first, const Values& second) {
    const Camera::Lens& left = first.pose.lens;
    const Camera::Lens& right = second.pose.lens;
    const Light::Emission& lit = first.pose.emission;
    const Light::Emission& other = second.pose.emission;
    return first.name == second.name && v3d::editor::same(first.pose.placement, second.pose.placement) &&
        left.projection == right.projection && left.fov == right.fov && left.extent == right.extent &&
        left.clipping == right.clipping && lit.type == other.type && lit.colour == other.colour &&
        lit.intensity == other.intensity && lit.cone == other.cone && lit.delta == other.delta &&
        first.style == second.style;
}

/**
 **/
EditCommand::EditCommand(const boost::shared_ptr<Item>& item, const Values& before, const std::string& what) :
    item_(item),
    before_(before),
    after_(item ? of(*item) : before),
    what_(what) {
}

/**
 **/
void EditCommand::undo() {
    if (item_) {
        apply(item_.get(), before_);
    }
}

/**
 **/
void EditCommand::redo() {
    if (item_) {
        apply(item_.get(), after_);
    }
}

/**
 **/
std::string EditCommand::name() const {
    return what_;
}

};  // namespace v3d::editor
