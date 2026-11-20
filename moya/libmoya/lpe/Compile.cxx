/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/lpe/Compile.h>

#include <moya/libmoya/kernel/integrate/Event.h>
#include <moya/libmoya/lpe/Automaton.h>
#include <moya/libmoya/lpe/Expression.h>
#include <moya/libmoya/lpe/Failure.h>
#include <moya/libmoya/lpe/internal/Threads.h>
#include <moya/libmoya/lpe/Pattern.h>
#include <moya/libmoya/lpe/Refused.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace v3d::moya::lpe {

namespace {

/** The states reached from some states on no symbol, sorted, each once. **/
std::vector<std::uint32_t> closure(const std::vector<internal::Threads::Loose> & states, std::vector<std::uint32_t> from) {
    std::vector<bool> seen(states.size(), false);
    std::vector<std::uint32_t> out;
    while (!from.empty()) {
        const std::uint32_t state = from.back();
        from.pop_back();
        if (seen[state]) {
            continue;
        }
        seen[state] = true;
        out.push_back(state);
        from.insert(from.end(), states[state].empty.begin(), states[state].empty.end());
    }
    std::sort(out.begin(), out.end());
    return out;
}

/** The expressions any of some states accepts, sorted, each once. **/
std::vector<std::uint32_t> acceptedBy(const std::vector<internal::Threads::Loose> & states, const std::vector<std::uint32_t> & set) {
    std::vector<std::uint32_t> out;
    for (const std::uint32_t state : set) {
        out.insert(out.end(), states[state].accepts.begin(), states[state].accepts.end());
    }
    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}

/** Subset construction from state zero, each state's symbols visited in order. **/
Automaton determined(const std::vector<internal::Threads::Loose> & states, std::uint32_t symbols) {
    Automaton out { .symbols = symbols };
    std::map<std::vector<std::uint32_t>, std::uint32_t> numbered;
    std::vector<std::vector<std::uint32_t>> sets { closure(states, { 0U }) };
    numbered.emplace(sets.front(), 0U);
    for (std::size_t at = 0; at < sets.size(); ++at) {
        out.accepts.push_back(acceptedBy(states, sets[at]));
        for (std::uint32_t symbol = 0; symbol < symbols; ++symbol) {
            std::vector<std::uint32_t> moved;
            for (const std::uint32_t state : sets[at]) {
                if (states[state].to != internal::Threads::kNowhere && states[state].on[symbol]) {
                    moved.push_back(states[state].to);
                }
            }
            const std::vector<std::uint32_t> reached = closure(states, moved);
            const auto [found, made] = numbered.emplace(reached, static_cast<std::uint32_t>(sets.size()));
            if (made) {
                if (sets.size() == kMostStates) {
                    throw Refused("the light path expressions' automaton holds more than " + std::to_string(kMostStates) + " states");
                }
                sets.push_back(reached);
            }
            out.next.push_back(found->second);
        }
    }
    return out;
}

/** Numbers each value by where it first appears, from zero. **/
template <typename T>
std::vector<std::uint32_t> classesOf(const std::vector<T> & values, std::uint32_t & count) {
    std::map<T, std::uint32_t> named;
    std::vector<std::uint32_t> out;
    out.reserve(values.size());
    for (const T & value : values) {
        out.push_back(named.emplace(value, static_cast<std::uint32_t>(named.size())).first->second);
    }
    count = static_cast<std::uint32_t>(named.size());
    return out;
}

/**
 * The minimal automaton that matches what one does, by Moore's refinement: states fall into
 * classes by what they accept, and each class splits by the classes its states step to, until
 * none splits. Its states are numbered as a walk from the start first reaches them.
 **/
Automaton minimal(const Automaton & from) {
    const std::uint32_t states = from.states();
    std::uint32_t count = 0;
    std::vector<std::uint32_t> classes = classesOf(from.accepts, count);
    for (std::uint32_t before = 0; before != count;) {
        before = count;
        std::vector<std::vector<std::uint32_t>> signatures(states);
        for (std::uint32_t state = 0; state < states; ++state) {
            signatures[state].push_back(classes[state]);
            for (std::uint32_t symbol = 0; symbol < from.symbols; ++symbol) {
                signatures[state].push_back(classes[from.step(state, symbol)]);
            }
        }
        classes = classesOf(signatures, count);
    }
    std::vector<std::uint32_t> member(count, 0);
    for (std::uint32_t state = states; state-- > 0;) {
        member[classes[state]] = state;
    }
    std::vector<std::uint32_t> order(count, internal::Threads::kNowhere);
    std::deque<std::uint32_t> walk { classes[0] };
    std::vector<std::uint32_t> walked;
    order[classes[0]] = 0;
    while (!walk.empty()) {
        const std::uint32_t at = walk.front();
        walk.pop_front();
        walked.push_back(at);
        for (std::uint32_t symbol = 0; symbol < from.symbols; ++symbol) {
            const std::uint32_t to = classes[from.step(member[at], symbol)];
            if (order[to] == internal::Threads::kNowhere) {
                order[to] = static_cast<std::uint32_t>(walked.size() + walk.size());
                walk.push_back(to);
            }
        }
    }
    Automaton out { .symbols = from.symbols };
    for (const std::uint32_t at : walked) {
        out.accepts.push_back(from.accepts[member[at]]);
        for (std::uint32_t symbol = 0; symbol < from.symbols; ++symbol) {
            out.next.push_back(order[classes[from.step(member[at], symbol)]]);
        }
    }
    return out;
}

};  // namespace

Compiled compile(const std::vector<std::string> & expressions, const std::vector<std::string> & labels) {
    Compiled out;
    internal::Threads threads;
    const std::uint32_t start = threads.add();
    for (std::size_t i = 0; i < expressions.size(); ++i) {
        const std::variant<Pattern, Failure> read = parse(expressions[i], labels);
        if (const Failure * failed = std::get_if<Failure>(&read)) {
            out.unread.push_back(Compiled::Unread { .expression = i, .failure = *failed });
            continue;
        }
        const std::uint32_t from = threads.add();
        threads.join(start, from);
        const std::uint32_t end = threads.thread(std::get<Pattern>(read), from);
        threads.states[end].accepts.push_back(static_cast<std::uint32_t>(i));
    }
    out.automaton = minimal(determined(threads.states, kernel::integrate::symbolsOf(static_cast<std::uint32_t>(labels.size()))));
    return out;
}

};  // namespace v3d::moya::lpe
