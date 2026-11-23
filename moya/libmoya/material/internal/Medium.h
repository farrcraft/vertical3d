/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/program/Medium.h>
#include <moya/libmoya/material/internal/Network.h>
#include <moya/libmoya/material/internal/TerminalInput.h>
#include <MaterialXCore/Node.h>

#include <array>

namespace v3d::moya::material::internal {

/** The inputs of the node a volume's shader is rewritten into, one for each medium value. **/
inline constexpr std::array kMediumInputs {
    TerminalInput { .name = "absorption", .at = kernel::program::kMediumAbsorption, .width = 3 },
    TerminalInput { .name = "scattering0", .at = kernel::program::lobeScattering(0), .width = 3 },
    TerminalInput { .name = "anisotropy0", .at = kernel::program::lobeAnisotropy(0) },
    TerminalInput { .name = "scattering1", .at = kernel::program::lobeScattering(1), .width = 3 },
    TerminalInput { .name = "anisotropy1", .at = kernel::program::lobeAnisotropy(1) },
    TerminalInput { .name = "emission", .at = kernel::program::kMediumEmission, .width = 3 },
};

/**
 * Rewrites a volume shader, MaterialX's `volume` node, into a node whose inputs are a medium's
 * values, each fed by ordinary nodes the program compiles.
 *
 * Its VDF is a tree of `absorption_vdf` and `anisotropic_vdf` leaves under `add` and `mix`
 * nodes. The absorption is the sum of every leaf's absorption, each times the weights of the
 * mixes above it, and the same holds for its emission over a tree of `uniform_edf` leaves. The
 * first `anisotropic_vdf` in the tree's order, inputs before outputs and in1 before in2 or fg
 * before bg, scatters as the first lobe, by its own anisotropy, and the second as the second. A
 * third and every later one adds its scattering to the second lobe, and is reported. A node of
 * any other kind is reported and adds nothing.
 *
 * Gives the node, which takes the volume's place as the network's terminal, or none when the
 * network's terminal is not a `volume`.
 **/
MaterialX::NodePtr rewrittenMedium(Network & network);

};  // namespace v3d::moya::material::internal
