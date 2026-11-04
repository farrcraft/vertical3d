/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Show.h"

#include <vertical3d/src/scene/item/Item.h>
#include <vertical3d/src/scene/Scene.h>

#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::editor::command {

/**
 **/
Show::Show(const boost::shared_ptr<scene::Scene>& scene, const std::vector<Change>& changes, const std::string& name) :
    scene_(scene),
    name_(name) {
    for (const Change& change : changes) {
        if (change.item && change.item->shown() != change.shown) {
            entries_.push_back({ .item = change.item, .before = change.item->shown(), .after = change.shown });
        }
    }
}

/**
 **/
void Show::redo() {
    for (const Entry& entry : entries_) {
        entry.item->shown(entry.after);
    }
    deselected_.clear();
    if (!scene_) {
        return;
    }
    for (const boost::shared_ptr<scene::item::Item>& item : scene_->selection()) {
        if (!scene_->shown(item->id()).views) {
            item->selected(false);
            deselected_.push_back(item);
        }
    }
}

/**
 **/
void Show::undo() {
    for (const Entry& entry : entries_) {
        entry.item->shown(entry.before);
    }
    for (const boost::shared_ptr<scene::item::Item>& item : deselected_) {
        item->selected(true);
    }
}

/**
 **/
std::string Show::name() const {
    return name_;
}

/**
 **/
bool Show::empty() const noexcept {
    return entries_.empty();
}

};  // namespace v3d::editor::command
