/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H

namespace v3d::font::internal {

/**
 * Owns an outline FreeType made, from make() until destruction.
 **/
class OutlineHandle final {
 public:
    explicit OutlineHandle(FT_Library library) :
        library_(library) {
    }
    ~OutlineHandle() {
        if (made_) {
            FT_Outline_Done(library_, &outline_);
        }
    }
    OutlineHandle(const OutlineHandle&) = delete;
    OutlineHandle& operator=(const OutlineHandle&) = delete;

    /**
     * @return whether room was made for the points and contours
     **/
    bool make(FT_UInt points, FT_Int contours) {
        made_ = FT_Outline_New(library_, points, contours, &outline_) == 0;
        return made_;
    }

    FT_Outline* get() {
        return &outline_;
    }

 private:
    FT_Library library_;
    FT_Outline outline_{};
    bool made_ = false;
};

};  // namespace v3d::font::internal
