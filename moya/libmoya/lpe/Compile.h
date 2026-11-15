/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/lpe/Automaton.h>
#include <moya/libmoya/lpe/Failure.h>
#include <moya/libmoya/lpe/Refused.h>

#include <cstddef>
#include <string>
#include <vector>

namespace v3d::moya::lpe {

/** A set of expressions compiled: its automaton, and each expression it could not read. **/
struct Compiled {
    /** An expression of the set that cannot be read: its place in the set, and why. **/
    struct Unread {
        std::size_t expression { 0 };
        Failure failure;
    };

    Automaton automaton;
    std::vector<Unread> unread;
};

/**
 * Every expression of a set, read by parse() under a render's labels, compiled into one
 * automaton. It is built by subset construction, visiting each state's symbols in order, then
 * made minimal. An expression that cannot be read matches nothing, and is named in unread.
 *
 * @throws Refused when the automaton holds more than kMostStates states while it is built
 **/
Compiled compile(const std::vector<std::string> & expressions, const std::vector<std::string> & labels);

};  // namespace v3d::moya::lpe
