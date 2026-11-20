/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

/*
 * NanoVDB's grids, as kernel code reads them. NanoVDB marks its functions __hostdev__, which it
 * defines for the same compilers MOYA_KERNEL is defined for, so its lookups run wherever kernel
 * code does. Kernel code includes NanoVDB through this header alone.
 *
 * MSVC reports code it cannot reach in NanoVDB's headers once it inlines them. Its level for
 * external headers does not cover that warning, so it is switched off around them.
 */
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4702)
#endif
#include <nanovdb/NanoVDB.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif
