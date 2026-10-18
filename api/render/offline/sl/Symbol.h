/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>
#include <string>

#include "Types.h"

namespace v3d::render::offline::sl {

/**
 * One named value a shader run holds.
 *
 * Each declaration is its own symbol, so a local declared twice in nested scopes is two
 * symbols. Each symbol gets its own register. The order is globals, then the shader's
 * parameters, then every local and formal in the order they were declared.
 **/
class Symbol final {
 public:
    enum class Role : std::uint8_t {
        /** A shader global: P, N, Ci and the rest, decided by the shader type. **/
        GLOBAL,
        /** A shader parameter, which a scene may bind. **/
        PARAMETER,
        /** A local, or the formal of a function defined inside the shader. **/
        LOCAL
    };

    std::string name;
    Type type = Type::FLOAT;
    Storage storage = Storage::UNIFORM;
    Role role = Role::LOCAL;
    /** Whether a shader of this type may assign it. **/
    bool writable = true;
    /** Whether the declaration states the storage class. When false, it is inferred. **/
    bool declared = false;
    /**
     * Whether a shader parameter is declared `output`. Recorded from the declaration, but
     * nothing reads it, and no output parameter is written back to the caller.
     **/
    bool output = false;
};

};  // namespace v3d::render::offline::sl
