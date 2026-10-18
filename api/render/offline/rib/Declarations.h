/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <map>
#include <string>

#include "Declaration.h"
#include "Requests.h"

namespace v3d::render::offline::rib {

/**
 * What a scene has declared, and what the standard declares for it.
 *
 * The constructor declares the standard geometric primitive variables and the parameters of
 * the standard shaders. Their names are plain strings, not RenderMan.h's tokens, because
 * RenderMan.h belongs to moya and this library does not depend on moya.
 **/
class Declarations final {
 public:
    Declarations();

    /**
     * Ri Declare.
     *
     * @return false when the declaration text does not parse, leaving the table alone
     **/
    bool declare(const Declare & request);

    /**
     * Resolve a parameter name.
     *
     * A name may be an inline declaration - "uniform float squish" - in which case the
     * last word is the name and the rest types it, and nothing enters the table.
     *
     * @param token the name as it appeared in the file
     * @param name written with the name alone
     * @param declaration written with what types it
     * @return whether the name was declared, inline or earlier
     **/
    bool resolve(const std::string & token, std::string * name, Declaration * declaration) const;

 private:
    std::map<std::string, Declaration> declarations_;
};

};  // namespace v3d::render::offline::rib
