/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/surface/Material.h>
#include <moya/libmoya/material/Compile.h>

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace v3d::moya::scene {
class Scene;
};  // namespace v3d::moya::scene

namespace v3d::moya::geometry {
struct Prototypes;
};  // namespace v3d::moya::geometry

namespace v3d::moya::geometry::internal {

/**
 * Every material of a scene whose surface node is a MaterialX node, compiled into its program
 * before any geometry is gathered, and the names of the primvars the programs read. The names
 * begin with `N`, `Cs`, `tangent` and `st`, at the numbers kernel::surface gives them, which
 * shading reads whatever a program does. Each other name follows in the order a program first
 * reads it.
 **/
class Programs final {
 public:
    /** Compiles each such material through the libraries installed beside moya. **/
    explicit Programs(const scene::Scene & scene);

    /** The name of each primvar shading reads, by its number. **/
    const std::vector<std::string> & names() const {
        return names_;
    }

    /** Whether the material at a scene's index was compiled here. **/
    bool compiles(std::uint32_t material) const {
        return compiled_.contains(material);
    }

    /**
     * Appends a compiled material's program to the prototypes', after every other, and gives the
     * material as shading reads it, with its reports. Without the libraries the material is
     * diffuse, and its one report says why.
     **/
    kernel::surface::Material place(std::uint32_t material, Prototypes & prototypes, std::vector<std::string> & reports) const;

 private:
    std::vector<std::string> names_;
    std::map<std::uint32_t, material::Compiled> compiled_;
};

};  // namespace v3d::moya::geometry::internal
