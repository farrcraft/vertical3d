/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Scrollbar.h"

#include <algorithm>

#include "SelectList.h"

namespace v3d::ui::component {

const float Scrollbar::minimumThumb = 16.0f;

Scrollbar::Scrollbar() :
    Component(Type::Scrollbar),
    content_(0.0f),
    page_(0.0f),
    offset_(0.0f),
    direction_(Direction::Vertical) {
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
    // the list has been drawn once - the same rule picking one follows, per ADR-0019
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

float Scrollbar::track() const noexcept {
    return direction_ == Direction::Vertical ? size().y : size().x;
}

float Scrollbar::thumb() const noexcept {
    const float length = track();
    const float whole = content();
    const float shown = page();
    if (whole <= 0.0f || shown >= whole) {
        return length;
    }
    // as much of the track as the page is of the content, which is what makes the thumb a
    // readout of how much there is as well as of where in it the page sits
    return std::clamp(length * (shown / whole), std::min(minimumThumb, length), length);
}

float Scrollbar::thumbStart() const noexcept {
    const float room = track() - thumb();
    if (room <= 0.0f || maximum() <= 0.0f) {
        return 0.0f;
    }
    return room * (offset() / maximum());
}

void Scrollbar::drag(const glm::vec2& point) {
    const float room = track() - thumb();
    if (room <= 0.0f) {
        return;
    }
    const float origin = direction_ == Direction::Vertical ? position().y : position().x;
    const float along = (direction_ == Direction::Vertical ? point.y : point.x) - origin;

    // the cursor holds the middle of the thumb, so what is under it stays under it
    offset(maximum() * ((along - thumb() * 0.5f) / room));
}

};  // namespace v3d::ui::component
