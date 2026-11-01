/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Tooltip.h"

#include <api/ui/Component.h>

namespace v3d::ui::shell {

bool Tooltip::due(const boost::shared_ptr<Component>& under, float now, const Delays& delays) {
    if (under != under_.lock()) {
        under_ = under;
        since_ = now;
    }
    if (!under || under->tooltip().empty()) {
        return false;
    }
    const bool following = shown_ && since_ - *shown_ <= delays.first;
    if (now - since_ < (following ? delays.following : delays.first)) {
        return false;
    }
    shown_ = now;
    return true;
}

};  // namespace v3d::ui::shell
