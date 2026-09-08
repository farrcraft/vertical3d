/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "../Component.h"

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

namespace v3d::ui::component {

class SelectList;

/**
 * A track with a thumb on it, saying which part of something taller than its box is shown.
 *
 * The bar is the arithmetic and not the input: an app that picked one calls drag() with
 * where the cursor is and reads offset(), which is how far to translate whatever the bar
 * scrolls. What it scrolls is a component of its own that clips its children, per
 * ADR-0037 - the bar does not hold it, because the two are laid out side by side rather
 * than one inside the other.
 *
 * A bar can instead be told which list it scrolls, and then it holds no range of its own:
 * the content, the page and the offset are the list's, and dragging the thumb moves the
 * list. An unbound bar is unchanged, which is what keeps it useful as a progress shaped
 * control for something with no component behind it.
 *
 * The track and the thumb are drawn in the "bar" style class the component names, per
 * ADR-0020.
 **/
class Scrollbar : public Component {
 public:
    enum class Direction {
        Vertical,
        Horizontal
    };

    /**
     * How short the thumb is allowed to get, in pixels. A page showing a hundredth of its
     * content would otherwise leave a thumb too small to take hold of.
     **/
    static const float minimumThumb;

    Scrollbar();
    ~Scrollbar() = default;

    void direction(Direction along);
    Direction direction() const noexcept;

    /**
     * Scroll a list rather than a range of its own.
     *
     * The bar then reports the list's content, its box as the page and its offset, and
     * drag() and scroll() move the list. Pass nothing to unbind it, which puts back
     * whatever range it was last given.
     *
     * Held weakly, the way ui::Engine holds the focus: the list belongs to its container,
     * and a bar outliving one that was unloaded should not keep it alive. A bar whose list
     * has gone reads as its own range again rather than crashing.
     **/
    void scrolls(const boost::shared_ptr<SelectList>& list);

    /**
     * @return the list this scrolls, or null when it scrolls a range of its own
     **/
    boost::shared_ptr<SelectList> scrolls() const;

    /**
     * What there is to scroll through and how much of it is shown, both in pixels along
     * the bar's direction. Ignored while a list is bound, which answers both itself.
     *
     * The offset is clamped to what the new range leaves, so a list that shrank while
     * scrolled to its end comes back to the end of what is left rather than past it.
     **/
    void range(float content, float page);
    float content() const noexcept;
    float page() const noexcept;

    /**
     * How far into the content the page starts, clamped to 0..maximum().
     **/
    void offset(float distance);
    float offset() const noexcept;

    /**
     * Move by a wheel notch's worth, or by whatever else asked. The same clamp as
     * offset(), written this way so a caller does not have to read the offset to add to
     * it.
     **/
    void scroll(float distance);

    /**
     * @return the furthest the page can start, which is what the page does not show
     **/
    float maximum() const noexcept;

    /**
     * @return whether there is anything to scroll, which is false when the page shows all
     *      of the content
     **/
    bool scrollable() const noexcept;

    /**
     * @return how long the thumb is, in pixels along the bar's direction
     **/
    float thumb() const noexcept;

    /**
     * @return how far along the track the thumb starts, in pixels
     **/
    float thumbStart() const noexcept;

    /**
     * Scroll to where a cursor has dragged the thumb to, taking the point as the middle
     * of it. Absolute rather than relative, so a bar picked anywhere on its track jumps
     * to what was clicked and then follows the cursor.
     *
     * The point is in canvas pixels and is measured against the box the bar was last
     * drawn in, so a bar that has not been drawn scrolls nowhere - the same rule as
     * picking one, per ADR-0019.
     *
     * @param point where the cursor is
     **/
    void drag(const glm::vec2& point);

 private:
    /**
     * @return how long the track is, which is the box's extent along the direction
     **/
    float track() const noexcept;

    float content_;
    float page_;
    float offset_;
    Direction direction_;
    boost::weak_ptr<SelectList> scrolled_;
};

};  // namespace v3d::ui::component
