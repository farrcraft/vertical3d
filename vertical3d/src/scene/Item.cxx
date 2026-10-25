/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Item.h"

#include <cstdint>

namespace v3d::editor {

/**
 **/
Item::Item() :
    selected_(false),
    documentId_(0) {
}

/**
 **/
Item::~Item() {
}

/**
 **/
std::int64_t Item::documentId() const noexcept {
    return documentId_;
}

/**
 **/
void Item::documentId(std::int64_t id) noexcept {
    documentId_ = id;
}

/**
 **/
std::uint64_t Item::revision() const noexcept {
    return changes() + changes_;
}

/**
 **/
void Item::changed() noexcept {
    changes_++;
}

/**
 **/
bool Item::selected() const noexcept {
    return selected_;
}

/**
 **/
void Item::selected(bool selected) noexcept {
    selected_ = selected;
}

};  // namespace v3d::editor
