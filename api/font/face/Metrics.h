/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

namespace v3d::font::face {

/**
 * A face's vertical measures at one size, in pixels, unhinted. Each is a distance from the
 * baseline, downwards for those below it.
 **/
struct Metrics final {
    float ascender = 0.0F;            /**< how far its tallest letters reach above the baseline **/
    float descender = 0.0F;           /**< how far its descenders reach below it **/
    float lineGap = 0.0F;             /**< the space the face asks for between one line and the next **/
    float underlinePosition = 0.0F;   /**< how far below the baseline an underline's centre is **/
    float underlineThickness = 0.0F;
    float strikeoutPosition = 0.0F;   /**< how far above the baseline a strikethrough's centre is **/
    float strikeoutThickness = 0.0F;
};

};  // namespace v3d::font::face
