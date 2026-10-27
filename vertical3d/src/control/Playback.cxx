/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
**/

#include "Playback.h"

#include <vertical3d/src/document/Document.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/scene/Timeline.h>

#include <algorithm>
#include <cmath>
#include <vector>

namespace v3d::editor {

namespace {

/**
 * A keyed frame as a whole frame within the timeline's range, rounded up or down.
 **/
int wholeFrame(float frame, const Timeline& timeline, bool up) {
    const float held = std::clamp(frame, static_cast<float>(timeline.start), static_cast<float>(timeline.end));
    return static_cast<int>(up ? std::ceil(held) : std::floor(held));  // checked: a key's frame is finite, and the clamp holds it in the range
}

};  // namespace

Playback::Playback(const boost::shared_ptr<Document>& document) :
    document_(document),
    playing_(false),
    played_(0.0F) {
}

void Playback::play() {
    playing_ = true;
    played_ = 0.0F;
}

void Playback::stop() {
    playing_ = false;
}

bool Playback::playing() const noexcept {
    return playing_;
}

void Playback::simulate(float step) {
    if (!playing_) {
        return;
    }
    const Timeline& timeline = document_->scene()->timeline();
    played_ += step;
    const float length = 1.0F / timeline.rate;
    if (played_ < length) {
        return;
    }
    const int frames = static_cast<int>(played_ / length);
    played_ -= static_cast<float>(frames) * length;
    const int span = timeline.end - timeline.start + 1;
    document_->goTo(timeline.start + ((timeline.frame - timeline.start + frames) % span));
}

void Playback::step(int frames) {
    document_->goTo(document_->scene()->timeline().frame + frames);
}

void Playback::first() {
    document_->goTo(document_->scene()->timeline().start);
}

void Playback::last() {
    document_->goTo(document_->scene()->timeline().end);
}

std::vector<float> Playback::keyed(const Items& items) const {
    const Scene& scene = *document_->scene();
    return scene.keyedFrames(items.empty() ? scene.items() : items);
}

void Playback::nextKey(const Items& items, bool forwards) {
    const Timeline& timeline = document_->scene()->timeline();
    const float frame = static_cast<float>(timeline.frame);
    const std::vector<float> frames = keyed(items);
    if (forwards) {
        const auto after = std::upper_bound(frames.begin(), frames.end(), frame);
        if (after != frames.end()) {
            document_->goTo(wholeFrame(*after, timeline, true));
        }
        return;
    }
    const auto before = std::lower_bound(frames.begin(), frames.end(), frame);
    if (before != frames.begin()) {
        document_->goTo(wholeFrame(*(before - 1), timeline, false));
    }
}

void Playback::firstKey(const Items& items) {
    const std::vector<float> frames = keyed(items);
    if (!frames.empty()) {
        document_->goTo(wholeFrame(frames.front(), document_->scene()->timeline(), true));
    }
}

void Playback::lastKey(const Items& items) {
    const std::vector<float> frames = keyed(items);
    if (!frames.empty()) {
        document_->goTo(wholeFrame(frames.back(), document_->scene()->timeline(), false));
    }
}

};  // namespace v3d::editor
