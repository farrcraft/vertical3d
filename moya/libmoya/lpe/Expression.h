/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/lpe/Failure.h>
#include <moya/libmoya/lpe/Pattern.h>

#include <string>
#include <variant>
#include <vector>

/** Light path expressions, read from OSL's grammar and compiled for the device. Host code. **/
namespace v3d::moya::lpe {

/**
 * An expression read as OSL's light path expressions are written, under the names of a render's
 * light groups, by their labels. Spaces between its parts are ignored.
 *
 * - An event is written <type scattering label>, each place a letter, a quoted label, a '.' for
 *   any, or a set of them in brackets, [...], or of all but them, [^...]. Places left off match
 *   anything. The types are C, R, T, V, L, O and B, and the scatterings D, G, S and s.
 * - A type's letter alone is that type's event, and a scattering's letter alone an event of that
 *   scattering. A quoted label alone, such as 'key', is an event of that label, and '.' is any
 *   event.
 * - A set of events in brackets matches any of them, and [^...] any event but them.
 * - Patterns follow one another, are grouped in parentheses, and are alternatives across |.
 *   A pattern is repeated by *, +, ?, {n}, {n,} or {n,m}, a count at most kMostRepeats.
 *
 * An event in a medium, V, has no scattering, so it matches a scattering's place only where that
 * place is left off or is '.'. A scattering's letter alone, such as D, matches no event in a
 * medium. Moya's paths have no background or straight events, so B and s match none. A label
 * that no light group holds matches none too.
 **/
std::variant<Pattern, Failure> parse(const std::string & text, const std::vector<std::string> & labels);

};  // namespace v3d::moya::lpe
