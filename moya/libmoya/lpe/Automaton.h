/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>
#include <vector>

namespace v3d::moya::lpe {

/** The most states an automaton holds while it is built, past which its set is refused. **/
inline constexpr std::uint32_t kMostStates = 1024;

/**
 * One deterministic automaton for every light path expression of a render. A path starts in
 * state zero and steps once for each event, by the event's symbol. A state accepts the
 * expressions accepts names, by their places in the set, once the events so far are a path each
 * of them matches. A state from which no expression can match steps only to itself.
 *
 * Its states are numbered as a walk from state zero first reaches them, taking each state's
 * symbols in order, so two sets that match the same paths give the same automaton.
 **/
struct Automaton {
    std::uint32_t symbols { 0 };

    /** Each state's next state on each symbol, symbols to a state. **/
    std::vector<std::uint32_t> next;

    /** The expressions each state accepts, by their places in the set, in order. **/
    std::vector<std::vector<std::uint32_t>> accepts;

    std::uint32_t states() const;

    /** The state after one on a symbol. **/
    std::uint32_t step(std::uint32_t state, std::uint32_t symbol) const;

    bool operator==(const Automaton & other) const = default;
};

};  // namespace v3d::moya::lpe
