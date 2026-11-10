/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstddef>

namespace v3d::moya::output {

/** How many linear spaces an EXR header's chromaticities name. **/
inline constexpr std::size_t kLinearSpaces = 6;

/** The Color Interop Forum IDs of the linear spaces whose chromaticities a header can carry. **/
inline constexpr const char * const kLinearIds[kLinearSpaces] = {
    "lin_ap0_scene", "lin_ap1_scene", "lin_rec709_scene", "lin_p3d65_scene", "lin_rec2020_scene", "lin_adobergb_scene",
};

/** Each of kLinearIds' chromaticities, x and y: the red, green and blue primaries, then white. **/
inline constexpr float kLinearPrimaries[kLinearSpaces][4][2] = {
    { { 0.7347F, 0.2653F }, { 0.0F, 1.0F }, { 0.0001F, -0.077F }, { 0.32168F, 0.33767F } },
    { { 0.713F, 0.293F }, { 0.165F, 0.830F }, { 0.128F, 0.044F }, { 0.32168F, 0.33767F } },
    { { 0.64F, 0.33F }, { 0.30F, 0.60F }, { 0.15F, 0.06F }, { 0.3127F, 0.3290F } },
    { { 0.680F, 0.320F }, { 0.265F, 0.690F }, { 0.150F, 0.060F }, { 0.3127F, 0.3290F } },
    { { 0.708F, 0.292F }, { 0.170F, 0.797F }, { 0.131F, 0.046F }, { 0.3127F, 0.3290F } },
    { { 0.64F, 0.33F }, { 0.21F, 0.71F }, { 0.15F, 0.06F }, { 0.3127F, 0.3290F } },
};

/** The place among kLinearIds of linear Rec. 709, which a header with none is taken to be in. **/
inline constexpr std::size_t kRec709 = 2;

};  // namespace v3d::moya::output
