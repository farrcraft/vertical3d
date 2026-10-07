/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Scrollbar.h"

#include <algorithm>

#include "SelectList.h"

namespace v3d::ui::component {

const float Scrollbar::minimumThumb = 16.0f;
const float Scrollbar::lineStep = 16.0f;

Scrollbar::Scrollbar() :
    Component(Type::Scrollbar),
    content_(0.0f),
    page_(0.0f),
    offset_(0.0f),
    direction_(Direction::Vertical) {
    // pickable for its drag and focusable for its keys, which a plain component is not
    pickable(true);
    focusable(true);
}

void Scrollbar::direction(Direction along) {
    direction_ = along;
}

Scrollbar::Direction Scrollbar::direction() const noexcept {
    return direction_;
}

void Scrollbar::scrolls(const boost::shared_ptr<SelectList>& list) {
    scrolled_ = list;
}

boost::shared_ptr<SelectList> Scrollbar::scrolls() const {
    return scrolled_.lock();
}

void Scrollbar::range(float content, float page) {
    content_ = std::max(content, 0.0f);
    page_ = std::max(page, 0.0f);
    offset(offset());
}

float Scrollbar::content() const noexcept {
    const boost::shared_ptr<SelectList> list = scrolled_.lock();
    return list ? list->content() : content_;
}

float Scrollbar::page() const noexcept {
    // a bound list's page is the box it was drawn in, so a bar reads as unscrollable until
    // the list has been drawn once
    const boost::shared_ptr<SelectList> list = scrolled_.lock();
    return list ? list->size().y : page_;
}

void Scrollbar::offset(float distance) {
    const boost::shared_ptr<SelectList> list = scrolled_.lock();
    if (list) {
        list->offset(distance);  // the list clamps against its own content
        return;
    }
    offset_ = std::clamp(distance, 0.0f, maximum());
}

float Scrollbar::offset() const noexcept {
    const boost::shared_ptr<SelectList> list = scrolled_.lock();
    return list ? list->offset() : offset_;
}

void Scrollbar::scroll(float distance) {
    offset(offset() + distance);
}

float Scrollbar::maximum() const noexcept {
    return std::max(content() - page(), 0.0f);
}

bool Scrollbar::scrollable() const noexcept {
    return maximum() > 0.0f;
}

float Scrollbar::line() const noexcept {
    const boost::shared_ptr<SelectList> list = scrolled_.lock();
    const float row = list ? list->rowHeight() : 0.0f;
    return row > 0.0f ? row : lineStep;
}

float Scrollbar::track() const noexcept {
    return direction_ == Direction::Vertical ? size().y : size().x;
}

float Scrollbar::thumb() const noexcept {
    return thumbLength(track(), page(), content());
}

float Scrollbar::thumbStart() const noexcept {
    return component::thumbStart(track(), thumb(), offset(), maximum());
}

void Scrollbar::drag(const glm::vec2& point) {
    if (track() - thumb() <= 0.0f) {
        return;
    }
    const float origin = direction_ == Direction::Vertical ? position().y : position().x;
    const float along = (direction_ == Direction::Vertical ? point.y : point.x) - origin;
    offset(dragOffset(track(), thumb(), along, maximum()));
}

float thumbLength(float track, float shown, float whole) noexcept {
    if (whole <= 0.0f || shown >= whole) {
        return track;
    }
    return std::clamp(track * (shown / whole), std::min(Scrollbar::minimumThumb, track), track);
}

float thumbStart(float track, float length, float offset, float span) noexcept {
    const float room = track - length;
    if (room <= 0.0f || span <= 0.0f) {
        return 0.0f;
    }
    return room * (offset / span);
}

float dragOffset(float track, float length, float along, float span) noexcept {
    const float room = track - length;
    if (room <= 0.0f) {
        return 0.0f;
    }
    return span * std::clamp((along - length * 0.5f) / room, 0.0f, 1.0f);
}

};  // namespace v3d::ui::component
