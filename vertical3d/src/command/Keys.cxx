/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Keys.h"

#include <api/animation/Curve.h>
#include <vertical3d/src/scene/item/Item.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/scene/Timeline.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <map>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::editor::command {

namespace {

/**
 * Move a curve's keys on the frames by the offset.
 **/
bool moved(v3d::animation::Curve* curve, const std::vector<float>& frames, float offset) {
    return curve->shift(frames, offset) > 0;
}

/**
 * Delete a curve's keys on the frames.
 **/
bool removed(v3d::animation::Curve* curve, const std::vector<float>& frames, [[maybe_unused]] float offset) {
    bool any = false;
    for (const float frame : frames) {
        any = curve->remove(frame) || any;
    }
    return any;
}

/**
 * @return whether a curve has a key on a frame
 **/
bool keyed(const v3d::animation::Curve& curve, float frame) {
    return std::any_of(curve.keys().begin(), curve.keys().end(), [frame](const v3d::animation::Curve::Key& key) {
        return std::fabs(key.frame - frame) <= v3d::animation::Curve::SAME_FRAME;
    });
}

};  // namespace

/**
 **/
boost::shared_ptr<Keys> Keys::shift(const boost::shared_ptr<scene::Scene>& scene, const std::vector<scene::FrameKey>& keys, float offset) {
    boost::shared_ptr<Keys> made(new Keys(scene, keys, &moved, offset));
    made->what_ = "move keys";
    return made;
}

/**
 **/
boost::shared_ptr<Keys> Keys::remove(const boost::shared_ptr<scene::Scene>& scene, const std::vector<scene::FrameKey>& keys) {
    boost::shared_ptr<Keys> made(new Keys(scene, keys, &removed, 0.0F));
    made->what_ = "delete keys";
    return made;
}

/**
 **/
boost::shared_ptr<Keys> Keys::replace(const boost::shared_ptr<scene::Scene>& scene, unsigned int item, const std::string& path,
    const v3d::animation::Curve& curve) {
    boost::shared_ptr<Keys> made(new Keys(scene, {}, &moved, 0.0F));
    made->what_ = "edit curve";
    const boost::shared_ptr<scene::item::Item> found = scene ? scene->item(item) : boost::shared_ptr<scene::item::Item>();
    if (!found) {
        return made;
    }
    const scene::Curves curves = scene->curves(item);
    const auto had = curves.find(path);
    if (had != curves.end() && had->second != curve) {
        made->changes_.push_back({ .item = found, .path = path, .before = had->second, .after = curve });
        made->count_ = 1;
    }
    return made;
}

/**
 **/
Keys::Keys(const boost::shared_ptr<scene::Scene>& scene, const std::vector<scene::FrameKey>& keys, const Edit& edit, float offset) :
    scene_(scene) {
    if (!scene_) {
        return;
    }
    // the frames asked for, gathered by item, each item once in the order it was first asked for
    std::vector<unsigned int> order;
    std::map<unsigned int, std::vector<float>> frames;
    for (const scene::FrameKey& key : keys) {
        if (!frames.contains(key.item)) {
            order.push_back(key.item);
        }
        frames[key.item].push_back(key.frame);
    }
    for (const unsigned int id : order) {
        const boost::shared_ptr<scene::item::Item> item = scene_->item(id);
        if (!item) {
            continue;
        }
        const scene::Curves curves = scene_->curves(id);
        for (const float frame : frames[id]) {
            const auto onFrame = [frame](const auto& each) { return keyed(each.second, frame); };
            if (std::any_of(curves.begin(), curves.end(), onFrame)) {
                count_++;
            }
        }
        for (const auto& [path, curve] : curves) {
            Change change{ .item = item, .path = path, .before = curve, .after = curve };
            if (edit(&change.after, frames[id], offset)) {
                changes_.push_back(change);
            }
        }
    }
}

/**
 **/
void Keys::redo() {
    apply(true);
}

/**
 **/
void Keys::undo() {
    apply(false);
}

/**
 **/
std::string Keys::name() const {
    return what_;
}

/**
 **/
std::size_t Keys::count() const noexcept {
    return count_;
}

/**
 **/
bool Keys::empty() const noexcept {
    return changes_.empty();
}

/**
 **/
void Keys::apply(bool after) {
    if (!scene_) {
        return;
    }
    for (const Change& change : changes_) {
        scene_->curve(change.item->id(), change.path, after ? change.after : change.before);
    }
}

};  // namespace v3d::editor::command
