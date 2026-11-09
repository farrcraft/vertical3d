/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

namespace v3d::font::glyph::colour {

/**
 * The colour tables a face holds. A face may hold several, and a glyph is drawn from the first
 * of COLR version 1, COLR version 0, SVG, then a bitmap table that has it.
 **/
struct Tables final {
    bool layers = false;  /**< COLR version 0: outlines filled with palette colours **/
    bool paint = false;   /**< COLR version 1: adds gradients, transforms and composites **/
    bool svg = false;     /**< an SVG document for each glyph **/
    bool cbdt = false;    /**< PNG images at fixed sizes, as Android's emoji are **/
    bool sbix = false;    /**< PNG images at fixed sizes, as Apple's emoji are **/

    /**
     * @return whether there is any colour table
     **/
    bool any() const noexcept;
};

};  // namespace v3d::font::glyph::colour
