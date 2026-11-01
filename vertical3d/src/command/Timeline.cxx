/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Timeline.h"

#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/scene/Timeline.h>

#include <algorithm>
#include <string>

#include <boost/shared_ptr.hpp>

namespace v3d::editor::command {

/**
 **/
Timeline::Timeline(const boost::shared_ptr<scene::Scene>& scene, const scene::Timeline& wanted, const std::string& what) :
    scene_(scene),
    before_(scene ? scene->timeline() : scene::Timeline()),
    after_(wanted),
    what_(what) {
}

/**
 **/
void Timeline::undo() {
    apply(before_);
}

/**
 **/
void Timeline::redo() {
    apply(after_);
}

/**
 **/
std::string Timeline::name() const {
    return what_;
}

/**
 **/
void Timeline::apply(const scene::Timeline& from) {
    if (!scene_) {
        return;
    }
    scene::Timeline now = scene_->timeline();
    now.rate = from.rate;
    now.start = from.start;
    now.end = from.end;
    now.autoKey = from.autoKey;
    const int frame = std::clamp(now.frame, now.start, now.end);
    const bool held = frame != now.frame;
    scene_->timeline(now);
    // a frame moved into the range is set as any other, which drops the pending edits
    if (held) {
        scene_->frame(frame);
    }
}

};  // namespace v3d::editor::command
