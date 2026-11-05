/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Edit.h"

#include <vertical3d/src/scene/item/Camera.h>
#include <vertical3d/src/scene/item/Curve.h>
#include <vertical3d/src/scene/item/Item.h>
#include <vertical3d/src/scene/item/Light.h>
#include <vertical3d/src/scene/Properties.h>

#include <string>

#include "Placement.h"

#include <boost/shared_ptr.hpp>

namespace v3d::editor::command {

/**
 **/
Edit::Values Edit::of(const scene::item::Item& item) {
    Values made{ .name = item.name(), .pose = scene::pose(item) };
    if (item.kind() == scene::item::Item::Kind::Curve) {
        made.style = static_cast<const scene::item::Curve&>(item).style();
    }
    return made;
}

/**
 **/
void Edit::apply(scene::item::Item* item, const Values& values) {
    item->name(values.name);
    item->placement(values.pose.placement);
    switch (item->kind()) {
        case scene::item::Item::Kind::Camera:
            static_cast<scene::item::Camera*>(item)->lens(values.pose.lens);
            break;
        case scene::item::Item::Kind::Light:
            static_cast<scene::item::Light*>(item)->emission(values.pose.emission);
            break;
        case scene::item::Item::Kind::Curve:
            static_cast<scene::item::Curve*>(item)->style(values.style);
            break;
        case scene::item::Item::Kind::Mesh:
        case scene::item::Item::Kind::Group:
            break;
    }
}

/**
 **/
bool Edit::same(const Values& first, const Values& second) {
    const scene::item::Camera::Lens& left = first.pose.lens;
    const scene::item::Camera::Lens& right = second.pose.lens;
    const scene::item::Light::Emission& lit = first.pose.emission;
    const scene::item::Light::Emission& other = second.pose.emission;
    return first.name == second.name && v3d::editor::command::same(first.pose.placement, second.pose.placement) &&
        left.projection == right.projection && left.fov == right.fov && left.extent == right.extent &&
        left.clipping == right.clipping && lit.type == other.type && lit.colour == other.colour &&
        lit.intensity == other.intensity && lit.cone == other.cone && lit.delta == other.delta &&
        first.style == second.style;
}

/**
 **/
Edit::Edit(const boost::shared_ptr<scene::item::Item>& item, const Values& before, const std::string& what) :
    item_(item),
    before_(before),
    after_(item ? of(*item) : before),
    what_(what) {
}

/**
 **/
void Edit::undo() {
    if (item_) {
        apply(item_.get(), before_);
    }
}

/**
 **/
void Edit::redo() {
    if (item_) {
        apply(item_.get(), after_);
    }
}

/**
 **/
std::string Edit::name() const {
    return what_;
}

};  // namespace v3d::editor::command
