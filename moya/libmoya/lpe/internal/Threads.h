/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/lpe/Pattern.h>

#include <cstdint>
#include <limits>
#include <vector>

namespace v3d::moya::lpe::internal {

/** Thompson's construction: states that match patterns, each from a state given. **/
class Threads final {
 public:
    /** A state that no symbol moves from. **/
    static constexpr std::uint32_t kNowhere = std::numeric_limits<std::uint32_t>::max();

    /**
     * A state of an automaton that is not deterministic: the states it moves to on no symbol, and
     * the one it moves to on any symbol on holds, and the expressions it accepts.
     **/
    struct Loose {
        std::vector<std::uint32_t> empty;
        std::vector<bool> on;
        std::uint32_t to { kNowhere };
        std::vector<std::uint32_t> accepts;
    };


    std::vector<Loose> states;

    std::uint32_t add() {
        states.emplace_back();
        return static_cast<std::uint32_t>(states.size() - 1U);
    }

    void join(std::uint32_t from, std::uint32_t to) {
        states[from].empty.push_back(to);
    }

    /** The state where a match of a pattern from a state ends. **/
    std::uint32_t thread(const Pattern & pattern, std::uint32_t from) {
        switch (pattern.kind) {
            case Pattern::Kind::Events:
                return events(pattern, from);
            case Pattern::Kind::Sequence:
                for (const Pattern & part : pattern.parts) {
                    from = thread(part, from);
                }
                return from;
            case Pattern::Kind::Choice:
                return choice(pattern, from);
            case Pattern::Kind::Repeat:
                break;
        }
        return repeat(pattern, from);
    }

 private:
    std::uint32_t events(const Pattern & pattern, std::uint32_t from) {
        const std::uint32_t moving = add();
        const std::uint32_t end = add();
        join(from, moving);
        states[moving].on = pattern.events;
        states[moving].to = end;
        return end;
    }

    std::uint32_t choice(const Pattern & pattern, std::uint32_t from) {
        const std::uint32_t end = add();
        for (const Pattern & part : pattern.parts) {
            const std::uint32_t start = add();
            join(from, start);
            join(thread(part, start), end);
        }
        return end;
    }

    std::uint32_t repeat(const Pattern & pattern, std::uint32_t from) {
        const Pattern & part = pattern.parts.front();
        for (std::uint32_t i = 0; i < pattern.least; ++i) {
            from = thread(part, from);
        }
        if (pattern.most == kUnbounded) {
            const std::uint32_t loop = add();
            join(from, loop);
            join(thread(part, loop), loop);
            return loop;
        }
        const std::uint32_t end = add();
        join(from, end);
        for (std::uint32_t i = pattern.least; i < pattern.most; ++i) {
            from = thread(part, from);
            join(from, end);
        }
        return end;
    }
};

};  // namespace v3d::moya::lpe::internal
