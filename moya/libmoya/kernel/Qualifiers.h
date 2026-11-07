/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

/**
 * Marks a function of kernel code, so that a GPU compiler builds it for both the host and the
 * device. Every function in a kernel header carries it. It is empty on the CPU.
 *
 * nvcc defines __CUDACC__ and HIP's Clang defines __HIP__, and each takes the same two
 * qualifiers.
 **/
#if defined(__CUDACC__) || defined(__HIP__)
#define MOYA_KERNEL __host__ __device__
#else
#define MOYA_KERNEL
#endif
