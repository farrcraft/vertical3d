/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

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
 * is.** Only the declaration gives the length, so a parameter whose name cannot be resolved
 * cannot be read. The RIB reader resolves names through Declarations too, so a parameter is
 * typed the same way from a file and from the C interface.
 *
 * A name may carry its declaration inline, as in `"uniform float Ks"`. That declaration
 * holds for this request only, as it does in a file.
 *
 * @param declarations what the scene has declared, and what the standard declares for it
 * @param count how many parameters, which is RI's `n`
 * @param tokens the names, one per parameter
 * @param values the value arrays, one per parameter; an entry that is null is skipped
 * @param vertices how many vertices the request's primitive has, which decides the length
 *        of a varying or a vertex array. One for everything that is not geometry, since a
 *        uniform value is one element whatever it is attached to
 * @param list where the parameters go, added to whatever it already holds
 * @param unresolved where the names that could not be resolved go, or null to drop them.
 *        Those parameters are skipped, and a caller can use this list to report them
 **/
void arguments(const Declarations & declarations, int count,
    const char* const* tokens, const void* const* values, unsigned int vertices,
    ParameterList* list, std::vector<std::string>* unresolved = nullptr);

/**
 * The parameters of a primitive whose faces have more corners than it has vertices, such as a
 * mesh whose faces share points. A facevarying array has one element per corner, and the others
 * are read as the form above reads them.
 *
 * @param declarations what the scene has declared, and what the standard declares for it
 * @param count how many parameters, which is RI's `n`
 * @param tokens the names, one per parameter
 * @param values the value arrays, one per parameter; an entry that is null is skipped
 * @param vertices how many vertices the primitive has, which decides the length of a varying or
 *        a vertex array
 * @param corners how many corners the faces have in all: the length of the mesh's indices, which
 *        decides the length of a facevarying array
 * @param list where the parameters go, added to whatever it already holds
 * @param unresolved where the names that could not be resolved go, or null to drop them
 **/
void arguments(const Declarations & declarations, int count,
    const char* const* tokens, const void* const* values, unsigned int vertices, unsigned int corners,
    ParameterList* list, std::vector<std::string>* unresolved = nullptr);

};  // namespace v3d::render::offline::rib
