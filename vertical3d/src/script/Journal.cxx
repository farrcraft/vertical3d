/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Journal.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>

namespace v3d::editor::script {

/**
 **/
Journal::Journal(std::size_t capacity) :
    capacity_(std::max<std::size_t>(capacity, 1)) {
}

/**
 **/
std::uint64_t Journal::add(Kind kind, const std::string& text) {
    entries_.push_back({ .number = ++made_, .kind = kind, .text = text });
    while (entries_.size() > capacity_) {
        entries_.pop_front();
    }
    ++revision_;
    return made_;
}

/**
 **/
void Journal::rewrite(std::uint64_t number, const std::string& text) {
    if (entries_.empty() || number < entries_.front().number || number > made_) {
        return;
    }
    entries_[static_cast<std::size_t>(number - entries_.front().number)].text = text;
    ++revision_;
}

/**
 **/
std::uint64_t Journal::last() const noexcept {
    return made_;
}

/**
 **/
const std::deque<Journal::Entry>& Journal::entries() const noexcept {
    return entries_;
}

/**
 **/
std::uint64_t Journal::revision() const noexcept {
    return revision_;
}

};  // namespace v3d::editor::script
