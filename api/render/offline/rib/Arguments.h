/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>
#include <vector>

#include "Declarations.h"
#include "Parameters.h"

namespace v3d::render::offline::rib {

/**
 * The C interface's token and pointer arrays as a `ParameterList`.
 *
 * RI's `Ri*V` entry points carry a parameter list as two parallel arrays: a name per
 * parameter and an untyped pointer to its values. **Neither says how long a value array
 * is** - the declaration does, which is why a renderer that cannot resolve a name cannot
 * read past it either. This is the same answer the reader reaches from a file, and it
 * lives here so that the two paths into a render context agree rather than each having
 * its own.
 *
 * A name may carry its declaration inline - `"uniform float Ks"` - which declares it for
 * this request only, exactly as it does in a file.
 *
 * @param declarations what the scene has declared, and what the standard declares for it
 * @param count how many parameters, which is RI's `n`
 * @param tokens the names, one per parameter
 * @param values the value arrays, one per parameter; an entry that is null is skipped
 * @param vertices how many vertices the request's primitive has, which is what decides
 *        the length of a varying or a vertex array. One for everything that is not
 *        geometry, since a uniform value is one element whatever it is attached to
 * @param list where the parameters go, added to whatever it already holds
 * @param unresolved where the names that could not be resolved go, or null to drop them.
 *        A name with no declaration is not merely absent from the answer: nothing after
 *        it can be trusted either, and a caller that wants to say so needs to know
 **/
void arguments(const Declarations & declarations, int count,
    const char* const* tokens, const void* const* values, unsigned int vertices,
    ParameterList* list, std::vector<std::string>* unresolved = nullptr);

};  // namespace v3d::render::offline::rib
