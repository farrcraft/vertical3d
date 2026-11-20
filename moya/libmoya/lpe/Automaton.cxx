/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/lpe/Automaton.h>

#include <cstddef>
#include <cstdint>

namespace v3d::moya::lpe {

std::uint32_t Automaton::states() const {
    return symbols == 0U ? 0U : static_cast<std::uint32_t>(next.size() / symbols);
}

std::uint32_t Automaton::step(std::uint32_t state, std::uint32_t symbol) const {
    return next[(static_cast<std::size_t>(state) * symbols) + symbol];
}

};  // namespace v3d::moya::lpe
