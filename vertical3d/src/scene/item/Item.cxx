/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Item.h"

#include <api/type/Transform.h>

#include <atomic>
#include <cstdint>
#include <string>

#include <glm/ext/quaternion_float.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace v3d::editor::scene::item {

/**
 **/
Item::Item() :
    id_(nextId()),
    selected_(false),
    documentId_(0) {
}

/**
 **/
Item::~Item() {
}

/**
 **/
unsigned int Item::nextId() noexcept {
    static std::atomic<unsigned int> last{ 0 };  // process-wide: a run-unique id counter, so ids stay unique whichever scene holds an item
    return ++last;
}

/**
 **/
unsigned int Item::id() const noexcept {
    return id_;
}

/**
 **/
const v3d::type::Transform& Item::placement() const noexcept {
    return placement_;
}

/**
 **/
void Item::placement(const v3d::type::Transform& placement) noexcept {
    placement_ = placement;
    changed();
}

/**
 **/
const std::string& Item::name() const noexcept {
    return name_;
}

/**
 **/
void Item::name(const std::string& name) {
    name_ = name;
    changed();
}

/**
 **/
glm::vec3 Item::translation() const noexcept {
    return placement_.position;
}

/**
 **/
void Item::translation(const glm::vec3& translation) noexcept {
    placement_.position = translation;
    changed();
}

/**
 **/
glm::quat Item::rotation() const noexcept {
    return placement_.rotation;
}

/**
 **/
void Item::rotation(const glm::quat& rotation) noexcept {
    placement_.rotation = rotation;
    changed();
}

/**
 **/
glm::vec3 Item::scale() const noexcept {
    return placement_.scale;
}

/**
 **/
void Item::scale(const glm::vec3& scale) noexcept {
    placement_.scale = scale;
    changed();
}

/**
 **/
void Item::translate(const glm::vec3& offset) noexcept {
    placement_.position += offset;
    changed();
}

/**
 **/
glm::mat4 Item::matrix() const {
    return placement_.matrix();
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
    return changes_;
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

};  // namespace v3d::editor::scene::item
