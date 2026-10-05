/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Statistics.h"

#include <algorithm>
#include <chrono>
#include <string>
#include <utility>
#include <vector>

namespace v3d::engine {

Statistics::Statistics() :
    clock_([]() {
        return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
    }) {
}

void Statistics::frame(std::uint64_t frame, unsigned int steps) noexcept {
    last_ = frame;
    steps_ = steps;
    // the slot being overwritten leaves the window, so the running total loses it rather
    // than the mean summing the array again every frame
    total_ -= recent_[next_];
    total_ += frame;
    recent_[next_] = frame;
    // a name not timed this frame counts as zero for it, so its mean stays correct
    for (Named& named : named_) {
        named.total -= named.recent[next_];
        named.total += named.pending;
        named.recent[next_] = named.pending;
        named.last = named.pending;
        named.pending = 0;
    }
    next_ = (next_ + 1) % window;
    frames_++;
}

Statistics::Scope Statistics::scope(std::string_view name) {
    return Scope(this, row(name));
}

void Statistics::add(std::string_view name, std::uint64_t nanoseconds) {
    named_[row(name)].pending += nanoseconds;
}

std::size_t Statistics::row(std::string_view name) {
    for (std::size_t index = 0; index < named_.size(); ++index) {
        if (named_[index].name == name) {
            return index;
        }
    }
    Named fresh;
    fresh.name = std::string(name);
    named_.push_back(std::move(fresh));
    return named_.size() - 1;
}

std::vector<Statistics::Row> Statistics::rows() const {
    std::vector<Row> found;
    found.reserve(named_.size());
    const std::uint64_t counted = std::min<std::uint64_t>(frames_, window);
    for (const Named& named : named_) {
        found.push_back(Row{ named.name, named.last, counted == 0 ? 0 : named.total / counted });
    }
    return found;
}

void Statistics::clock(const Clock& clock) {
    clock_ = clock;
}

Statistics::Scope::Scope(Statistics* statistics, std::size_t row) :
statistics_(statistics),
row_(row),
start_(statistics->clock_()) {
}

Statistics::Scope::Scope(Scope&& other) noexcept :
statistics_(other.statistics_),
row_(other.row_),
start_(other.start_) {
    other.statistics_ = nullptr;
}

Statistics::Scope::~Scope() {
    if (statistics_ != nullptr) {
        statistics_->named_[row_].pending += statistics_->clock_() - start_;
    }
}

std::uint64_t Statistics::last() const noexcept {
    return last_;
}

std::uint64_t Statistics::mean() const noexcept {
    if (frames_ == 0) {
        return 0;
    }
    return total_ / std::min<std::uint64_t>(frames_, window);
}

unsigned int Statistics::steps() const noexcept {
    return steps_;
}

std::uint64_t Statistics::frames() const noexcept {
    return frames_;
}

};  // namespace v3d::engine
