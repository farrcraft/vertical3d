/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "DebugOverlay.h"

#include <sstream>
#include <string>
#include <vector>

#include "Scene.h"
#include "Version.h"
#include "game/Player.h"

DebugOverlay::DebugOverlay(const boost::shared_ptr<Scene>& scene) :
    scene_(scene),
    enabled_(false),
    cursor_(0),
    total_(0) {
    durations_.fill(0);
}

void DebugOverlay::enable(bool status) {
    enabled_ = status;
    if (enabled_) {
        durations_.fill(0);
        cursor_ = 0;
        total_ = 0;
        update(0);
    }
}

bool DebugOverlay::enabled() const {
    return enabled_;
}

const std::vector<std::string>& DebugOverlay::lines() const {
    return lines_;
}

unsigned int DebugOverlay::average(unsigned int delta) {
    total_ -= durations_[cursor_];
    total_ += delta;
    durations_[cursor_] = delta;
    cursor_ = (cursor_ + 1) % samples;
    return total_ / static_cast<unsigned int>(samples);
}

void DebugOverlay::update(unsigned int delta) {
    const unsigned int frame = average(delta);
    const glm::vec3 position = scene_->player()->position();

    lines_.clear();

    std::stringstream version;
    version << "Voxel " << VOXEL_VERSION << " - " << frame << " ms";
    lines_.push_back(version.str());

    std::stringstream where;
    where.precision(1);
    where << std::fixed << "x " << position.x << "  y " << position.y << "  z " << position.z;
    lines_.push_back(where.str());
}
