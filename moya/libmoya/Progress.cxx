/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Progress.h"

namespace v3d::moya {

void Progress::callback(const Callback & told) {
    told_ = told;
}

void Progress::begin() {
    stopped_ = false;
}

bool Progress::report(float done) {
    if (!stopped_ && told_ && !told_(done)) {
        stopped_ = true;
    }
    return !stopped_;
}

bool Progress::stopped() const noexcept {
    return stopped_;
}

};  // namespace v3d::moya
