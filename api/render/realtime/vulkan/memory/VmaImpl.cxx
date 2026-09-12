/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

// The Vulkan Memory Allocator ships as one header that is both the declaration and the
// implementation, and the implementation half has to be compiled in exactly one translation
// unit. This is it. The warning level is dropped and both analysers are switched off for this
// file in CMakeLists.txt, for the same reason cpplint skips vendored sources: a local fix is
// lost the next time the header is taken from upstream.

#pragma warning(push, 0)

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#pragma warning(pop)
