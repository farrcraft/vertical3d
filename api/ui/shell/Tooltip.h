/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/ui/Component.h>

#include <optional>

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

namespace v3d::ui::shell {

/**
 * When the tooltip of what the cursor rests on is due.
 *
 * A component's tooltip shows once the cursor has rested on it for a delay. When a tooltip was
 * showing a moment before, a shorter delay applies, so moving along a row of buttons shows each
 * one's almost at once. "A moment" is the full delay: a cursor that has been off every tooltip
 * for longer waits the full delay again.
 **/
class Tooltip final {
 public:
    /**
     * How long the cursor rests before a tooltip shows, in seconds.
     **/
    struct Delays final {
        float first = 0.0F;      /**< after no tooltip was showing **/
        float following = 0.0F;  /**< after one was showing a moment before **/
    };

    /**
     * @param under what the cursor rests on now, or null
     * @param now the ui clock's time, in seconds
     * @return whether its tooltip shows this frame. A component with no tooltip never does
     **/
    bool due(const boost::shared_ptr<Component>& under, float now, const Delays& delays);

 private:
    boost::weak_ptr<Component> under_;
    float since_ = 0.0F;
    std::optional<float> shown_;
};

};  // namespace v3d::ui::shell
