/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/ui/immediate/Id.h>

#include <string>
#include <vector>

namespace v3d::ui::immediate {

/**
 * The table being written.
 **/
struct Table final {
    Table() noexcept;

    bool open;
    std::vector<std::string> headers;
    std::vector<float> widths;
    float left;             /**< where the row starts, which columns are measured from **/
    unsigned int column;    /**< which column the pen is in **/
    Id id;                  /**< the id that holds the table's scroll and content **/
    Id scroll;              /**< the id its scrollbar is hit tested under **/
    float right;            /**< the row right to go back to, and the region's right edge **/
    float height;           /**< how tall it is, or zero for a table that does not scroll **/
    float top;              /**< where it starts, which its height is measured from **/
    float contentTop;       /**< where its rows start, below the header **/
    bool clipped;
};

};  // namespace v3d::ui::immediate
