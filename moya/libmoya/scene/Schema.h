/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/scene/Declaration.h>
#include <moya/libmoya/scene/Kind.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace v3d::moya::scene {

/**
 * The kinds a scene's objects can be. A kind is reached by its index, which a handle holds.
 **/
class Schema final {
 public:
    /**
     * Add a kind, after checking each declaration.
     *
     * @return its index
     * @throws std::invalid_argument when the kind or a parameter has no name, or two share a
     *         name. Also when a fallback's type, count or string breaks its declaration, or a
     *         parameter that is not a string has choices. Also when a handle parameter has no
     *         targets, or one that is no kind added before
     **/
    std::uint32_t add(const Kind & kind);

    /**
     * @throws std::out_of_range when there is no such kind
     **/
    const Kind & kind(std::uint32_t index) const;

    /** The index of a kind by its name. **/
    std::optional<std::uint32_t> find(const std::string & name) const;

    std::uint32_t size() const noexcept;

    /**
     * Check a declaration as add() checks each of a kind's, and a primvar's declaration as a
     * client gives it.
     *
     * @throws std::invalid_argument as add() does
     **/
    void check(const Declaration & declaration) const;

 private:
    std::vector<Kind> kinds_;
};

};  // namespace v3d::moya::scene
