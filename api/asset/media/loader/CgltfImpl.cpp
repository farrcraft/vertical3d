/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

// cgltf ships as one header that holds both the declarations and the implementation. The
// implementation half has to be compiled in exactly one translation unit, and this is it.
// The pragma below drops the warning level, and CMakeLists.txt switches off both analysers
// for this file. A local fix to the header would be lost the next time it is updated from
// upstream.

#ifdef _MSC_VER
#pragma warning(push, 0)
#endif

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif
