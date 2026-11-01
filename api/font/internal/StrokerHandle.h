/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <ft2build.h>
#include FT_STROKER_H

namespace v3d::font::internal {

/**
 * Owns the stroker while the outline is built.
 *
 * Each step after FT_Stroker_New can fail and return, so the destructor releases the stroker
 * on every path.
 **/
class StrokerHandle final {
 public:
    explicit StrokerHandle(FT_Stroker stroker) : stroker_(stroker) {
    }
    ~StrokerHandle() {
        FT_Stroker_Done(stroker_);
    }
    StrokerHandle(const StrokerHandle&) = delete;
    StrokerHandle& operator=(const StrokerHandle&) = delete;

    FT_Stroker get() const {
        return stroker_;
    }

 private:
    FT_Stroker stroker_;
};

};  // namespace v3d::font::internal
