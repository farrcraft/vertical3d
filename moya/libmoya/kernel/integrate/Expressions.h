/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Array.h>
#include <moya/libmoya/kernel/film/Combine.h>
#include <moya/libmoya/kernel/integrate/Event.h>
#include <moya/libmoya/kernel/Qualifiers.h>

#include <cstdint>

namespace v3d::moya::kernel::integrate {

/** The most light path expressions a render takes, one flag of a state's accepts for each. **/
inline constexpr std::uint32_t kMostExpressions = 32;

/**
 * A render's light path expressions as one automaton, which a path steps once for each event by
 * the event's symbol. next holds each state's next state on each symbol, symbols to a state.
 * accepts holds, for each state, a flag for each expression that the events so far match, the
 * first expression's the lowest bit. channels holds the film channel each expression adds to, by
 * its place. A render of no expression has no channels.
 **/
struct Expressions {
    /** The film channel an expression adds the light it matches to, and how. **/
    struct Channel {
        std::uint32_t offset { 0 };
        film::Combine combine { film::Combine::Mean };
    };

    Array<std::uint32_t> next;
    Array<std::uint32_t> accepts;
    Array<Channel> channels;
    std::uint32_t symbols { 0 };

    /** Whether the render has no expression. **/
    MOYA_KERNEL bool none() const {
        return channels.size == 0U;
    }

    /** The state after one on an event. **/
    MOYA_KERNEL std::uint32_t step(std::uint32_t state, const Event & event) const {
        return next[(state * symbols) + symbolOf(event)];
    }
};

};  // namespace v3d::moya::kernel::integrate
