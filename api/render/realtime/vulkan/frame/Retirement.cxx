/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Retirement.h"

#include <utility>

namespace v3d::render::realtime::vulkan::frame {

/**
 **/
Retirement::Retirement(uint32_t framesInFlight) :
    framesInFlight_(framesInFlight > 0 ? framesInFlight : 1) {
}

/**
 **/
Retirement::~Retirement() {
    flush();
}

/**
 **/
void Retirement::retire(uint64_t begun, std::function<void()> destroy) {
    entries_.push_back(Entry{begun, std::move(destroy)});
}

/**
 **/
void Retirement::collect(uint64_t begun) {
    // entries are retired in the order frames are counted, so the ones due are at the front
    while (!entries_.empty() && entries_.front().begun + framesInFlight_ <= begun) {
        std::function<void()> destroy = std::move(entries_.front().destroy);
        entries_.pop_front();
        destroy();
    }
}

/**
 **/
void Retirement::flush() {
    while (!entries_.empty()) {
        std::function<void()> destroy = std::move(entries_.front().destroy);
        entries_.pop_front();
        destroy();
    }
}

/**
 **/
std::size_t Retirement::pending() const noexcept {
    return entries_.size();
}

};  // namespace v3d::render::realtime::vulkan::frame
