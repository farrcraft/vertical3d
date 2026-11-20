/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>
#include <limits>
#include <vector>

namespace v3d::moya::lpe {

/** The most a counted repetition, {n} or {n,m}, repeats what it follows. **/
inline constexpr std::uint32_t kMostRepeats = 32;

/** The most of a repetition with no bound: *, + and {n,}. **/
inline constexpr std::uint32_t kUnbounded = std::numeric_limits<std::uint32_t>::max();

/**
 * A light path expression read into a pattern over the automaton's symbols, which
 * kernel::integrate::symbolOf() gives each event.
 *
 * - Events matches one event whose symbol events holds, one flag to a symbol.
 * - Sequence matches its parts one after another, and nothing when it has none.
 * - Choice matches any one of its parts.
 * - Repeat matches its one part from least to most times.
 **/
struct Pattern {
    enum class Kind : std::uint8_t {
        Events,
        Sequence,
        Choice,
        Repeat
    };

    Kind kind { Kind::Sequence };
    std::vector<bool> events;
    std::vector<Pattern> parts;
    std::uint32_t least { 0 };
    std::uint32_t most { 0 };
};

};  // namespace v3d::moya::lpe
