/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <string>
#include <vector>

namespace v3d::moya::light {

/**
 * The photometry an IES file gives: its vertical and horizontal angles in radians, both rising,
 * and its candela, all the vertical angles of the first horizontal angle, then of the next. The
 * candela are the file's times its candela multiplier, its ballast factor and its ballast-lamp
 * photometric factor.
 **/
struct Photometry {
    std::vector<float> vertical;
    std::vector<float> horizontal;
    std::vector<float> candela;
};

/**
 * Reads the text of an IES file, in the IESNA's LM-63 format of any year, into photometry. False
 * for a file moya cannot read, with why it cannot.
 *
 * - The keyword lines before TILT are skipped. TILT=NONE and TILT=INCLUDE are read, and the
 *   tilt an INCLUDE gives is skipped. A file whose TILT names another file is not read.
 * - Numbers may be parted by spaces, line breaks or commas.
 * - Only type C photometry is read, the photometry of luminaires. Types A and B are reported.
 * - The vertical angles lie in [0, 180] degrees. The horizontal angles are one angle, or span 0 to
 *   90, 0 to 180, 90 to 270 or 0 to 360 degrees. Angles that do not rise are reported.
 **/
bool parseIes(const std::string & text, Photometry & out, std::string & why);

};  // namespace v3d::moya::light
