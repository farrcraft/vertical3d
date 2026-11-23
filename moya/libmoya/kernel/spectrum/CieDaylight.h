/*
 * The components of the CIE daylight illuminants, at 5 nm steps, from the
 * International Commission on Illumination (CIE):
 *
 * CIE 2018. Components of the relative spectral distribution of daylight used in the calculation
 * of relative spectral power distributions of CIE daylight illuminants of different correlated
 * colour temperatures.
 * International Commission on Illumination (CIE), Vienna, AT. DOI: 10.25039/CIE.DS.w7zunnny
 *
 * Copyright (c) CIE. This file is licensed under the Creative Commons Attribution-ShareAlike 4.0
 * International licence, https://creativecommons.org/licenses/by-sa/4.0/, unlike the rest of the
 * tree. The CIE's values are written here as C++ float literals, which rounds each to single
 * precision. No value is otherwise changed.
 */

#pragma once

#include <moya/libmoya/kernel/Qualifiers.h>

namespace v3d::moya::kernel::spectrum {

/** The first wavelength of the daylight components' table, in nanometres. **/
inline constexpr int kDaylightFirst = 300;

/** The daylight components' table has one row every kDaylightStep nanometres. **/
inline constexpr int kDaylightStep = 5;

/** The daylight components' table has this many rows, from kDaylightFirst. **/
inline constexpr int kDaylightRows = 107;

/** The components S0, S1 and S2, one row every kDaylightStep nanometres from kDaylightFirst. **/
MOYA_KERNEL_DATA inline constexpr float kDaylight[kDaylightRows][3] = {
    { 0.04F, 0.02F, 0.0F },
    { 3.02F, 2.26F, 1.0F },
    { 6.0F, 4.5F, 2.0F },
    { 17.8F, 13.45F, 3.0F },
    { 29.6F, 22.4F, 4.0F },
    { 42.45F, 32.2F, 6.25F },
    { 55.3F, 42.0F, 8.5F },
    { 56.3F, 41.3F, 8.15F },
    { 57.3F, 40.6F, 7.8F },
    { 59.55F, 41.1F, 7.25F },
    { 61.8F, 41.6F, 6.7F },
    { 61.65F, 39.8F, 6.0F },
    { 61.5F, 38.0F, 5.3F },
    { 65.15F, 40.2F, 5.7F },
    { 68.8F, 42.4F, 6.1F },
    { 66.1F, 40.45F, 4.55F },
    { 63.4F, 38.5F, 3.0F },
    { 64.6F, 36.75F, 2.1F },
    { 65.8F, 35.0F, 1.2F },
    { 80.3F, 39.2F, 0.05F },
    { 94.8F, 43.4F, -1.1F },
    { 99.8F, 44.85F, -0.8F },
    { 104.8F, 46.3F, -0.5F },
    { 105.35F, 45.1F, -0.6F },
    { 105.9F, 43.9F, -0.7F },
    { 101.35F, 40.5F, -0.95F },
    { 96.8F, 37.1F, -1.2F },
    { 105.35F, 36.9F, -1.9F },
    { 113.9F, 36.7F, -2.6F },
    { 119.75F, 36.3F, -2.75F },
    { 125.6F, 35.9F, -2.9F },
    { 125.55F, 34.25F, -2.85F },
    { 125.5F, 32.6F, -2.8F },
    { 123.4F, 30.25F, -2.7F },
    { 121.3F, 27.9F, -2.6F },
    { 121.3F, 26.1F, -2.6F },
    { 121.3F, 24.3F, -2.6F },
    { 117.4F, 22.2F, -2.2F },
    { 113.5F, 20.1F, -1.8F },
    { 113.3F, 18.15F, -1.65F },
    { 113.1F, 16.2F, -1.5F },
    { 111.95F, 14.7F, -1.4F },
    { 110.8F, 13.2F, -1.3F },
    { 108.65F, 10.9F, -1.25F },
    { 106.5F, 8.6F, -1.2F },
    { 107.65F, 7.35F, -1.1F },
    { 108.8F, 6.1F, -1.0F },
    { 107.05F, 5.15F, -0.75F },
    { 105.3F, 4.2F, -0.5F },
    { 104.85F, 3.05F, -0.4F },
    { 104.4F, 1.9F, -0.3F },
    { 102.2F, 0.95F, -0.15F },
    { 100.0F, 0.0F, 0.0F },
    { 98.0F, -0.8F, 0.1F },
    { 96.0F, -1.6F, 0.2F },
    { 95.55F, -2.55F, 0.35F },
    { 95.1F, -3.5F, 0.5F },
    { 92.1F, -3.5F, 1.3F },
    { 89.1F, -3.5F, 2.1F },
    { 89.8F, -4.65F, 2.65F },
    { 90.5F, -5.8F, 3.2F },
    { 90.4F, -6.5F, 3.65F },
    { 90.3F, -7.2F, 4.1F },
    { 89.35F, -7.9F, 4.4F },
    { 88.4F, -8.6F, 4.7F },
    { 86.2F, -9.05F, 4.9F },
    { 84.0F, -9.5F, 5.1F },
    { 84.55F, -10.2F, 5.9F },
    { 85.1F, -10.9F, 6.7F },
    { 83.5F, -10.8F, 7.0F },
    { 81.9F, -10.7F, 7.3F },
    { 82.25F, -11.35F, 7.95F },
    { 82.6F, -12.0F, 8.6F },
    { 83.75F, -13.0F, 9.2F },
    { 84.9F, -14.0F, 9.8F },
    { 83.1F, -13.8F, 10.0F },
    { 81.3F, -13.6F, 10.2F },
    { 76.6F, -12.8F, 9.25F },
    { 71.9F, -12.0F, 8.3F },
    { 73.1F, -12.65F, 8.95F },
    { 74.3F, -13.3F, 9.6F },
    { 75.35F, -13.1F, 9.05F },
    { 76.4F, -12.9F, 8.5F },
    { 69.85F, -11.75F, 7.75F },
    { 63.3F, -10.6F, 7.0F },
    { 67.5F, -11.1F, 7.3F },
    { 71.7F, -11.6F, 7.6F },
    { 74.35F, -11.9F, 7.8F },
    { 77.0F, -12.2F, 8.0F },
    { 71.1F, -11.2F, 7.35F },
    { 65.2F, -10.2F, 6.7F },
    { 56.45F, -9.0F, 5.95F },
    { 47.7F, -7.8F, 5.2F },
    { 58.15F, -9.5F, 6.3F },
    { 68.6F, -11.2F, 7.4F },
    { 66.8F, -10.8F, 7.1F },
    { 65.0F, -10.4F, 6.8F },
    { 65.5F, -10.5F, 6.9F },
    { 66.0F, -10.6F, 7.0F },
    { 63.5F, -10.15F, 6.7F },
    { 61.0F, -9.7F, 6.4F },
    { 57.15F, -9.0F, 5.95F },
    { 53.3F, -8.3F, 5.5F },
    { 56.1F, -8.8F, 5.8F },
    { 58.9F, -9.3F, 6.1F },
    { 60.4F, -9.55F, 6.3F },
    { 61.9F, -9.8F, 6.5F },
};

};  // namespace v3d::moya::kernel::spectrum
