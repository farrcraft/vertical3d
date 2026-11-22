/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/surface/Material.h>
#include <moya/libmoya/material/Compile.h>
#include <moya/libmoya/scene/Handle.h>

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace v3d::moya::scene {
class Scene;
};  // namespace v3d::moya::scene

namespace v3d::moya::geometry {
class Cache;
struct Prototypes;
};  // namespace v3d::moya::geometry

namespace v3d::moya::geometry::internal {

/**
 * Every material of a scene whose surface node is a MaterialX node, compiled into its program
 * before any geometry is gathered. Each material's displacement whose node is a MaterialX node is
 * compiled too, and the names of the primvars the programs read are kept. Each material holds the
 * medium it encloses: its volume's, compiled from its volume terminal, or else the one its
 * OpenPBR transmission gives. The names
 * begin with `N`, `Cs`, `tangent` and `st`, at the numbers kernel::surface gives them, which
 * shading reads whatever a program does. Each other name follows in the order a program first
 * reads it, and then each primvar a scene's output names a channel of, in the order the outputs
 * name them.
 *
 * Given a cache, each material is taken from it when the cache kept it, as Cache says, and
 * compiled otherwise. The cache then keeps every material of the scene, and forgets what it
 * was told to forget.
 **/
class Programs final {
 public:
    /** Compiles each such material through the libraries installed beside moya. **/
    explicit Programs(const scene::Scene & scene, Cache * cache = nullptr);

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
     * diffuse, and its one report says why. A material whose volume compiles appends the volume's
     * program after its surface's, and is bounded when that program reads nothing but its fields
     * and constants.
     **/
    kernel::surface::Material place(std::uint32_t material, Prototypes & prototypes, std::vector<std::string> & reports) const;

    /**
     * The displacement of the material at a scene's index, compiled here, or none. A displacement
     * that does not displace holds no values, and its reports say why.
     **/
    const material::Compiled * displacement(std::uint32_t material) const {
        const auto found = displacements_.find(material);
        return found == displacements_.end() ? nullptr : &found->second;
    }

    /**
     * Appends a compiled program to the prototypes', after every other, with its images and its
     * colour constants, and gives the material with its program's first instruction and value
     * where they now lie.
     **/
    static kernel::surface::Material append(const material::Compiled & held, Prototypes & prototypes);

 private:
    /**
     * Whether the cache holds a program of a material that can be kept: compiled from the same
     * object after the same primvars, with nothing in its network or the new one forgotten since.
     **/
    bool keeps(const Cache & cache, const scene::Handle & handle, const std::set<std::uint32_t> & network) const;

    std::vector<std::string> names_;
    std::map<std::uint32_t, material::Compiled> compiled_;
    std::map<std::uint32_t, material::Compiled> displacements_;
    std::map<std::uint32_t, material::Compiled> volumes_;
};

};  // namespace v3d::moya::geometry::internal
