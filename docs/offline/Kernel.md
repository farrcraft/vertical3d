# Kernel code

This page is for anyone writing moya's kernel code. Kernel code is the part of the new core that
every device compiles from one source: the CPU now, and HIP and CUDA when their back ends exist.
It lives in `moya/libmoya/kernel/`, in the namespace `v3d::moya::kernel`. Everything outside that
directory is host code, which calls kernel code and is never called by it.

The new core sits beside the reyes hider and the ray hider until they are removed. Nothing here
changes how either of them renders.

## The rules

Kernel code compiles under the rules a GPU compiler imposes. On the CPU, the kernel target and
[`kernel.ts`](../contributing/Gates.md) enforce them between them, so a breach fails before any
GPU build exists.

| Rule | Why | Enforced by |
|---|---|---|
| No exceptions: no `throw`, `try` or `catch` | A GPU has no unwinding | The target, and the gate |
| No RTTI: no `dynamic_cast` or `typeid` | A GPU compiler has none | The target, and the gate |
| No virtual functions | A GPU has no virtual call. A choice between kinds is a `switch` over a tag | The gate |
| No allocation: no `new`, `delete`, `malloc` and its kin, and no type that allocates, such as `std::vector`, `std::function` or anything from boost | A GPU allocates nothing per thread. Memory comes from the device layer, sized before a launch | The gate |
| Includes only kernel headers, glm, and `cfloat`, `climits`, `cmath`, `cstddef`, `cstdint`, `limits` and `type_traits` | These are what every device compiler provides | The gate |
| C++20 | The newest standard both HIP and nvcc accept. The rest of the tree is C++23 | The target |
| No multiply and add fused into one rounding | MSVC, Clang and GCC then round kernel code alike | The target, for itself and for every source that links it |
| Every function carries `MOYA_KERNEL` | A GPU compiler builds a function for the device only when it is marked | Review |
| Every header is included by `Kernel.cxx` | A header is compiled under the kernel's rules only through a source of the kernel target | The gate |

A deleted function, `= delete`, is not an allocation, and the gate does not report one.

Vectors, matrices and quaternions are glm's, with glm's SIMD intrinsics left off. glm marks each
of its functions as host and device code under HIP and nvcc. Sampled spectra and the lanes of a
SIMD traversal are moya's own types.

Background: [ADR-0191](../adr/offline/0191-offline-one-c-kernel-source-for-every-device.md),
[ADR-0197](../adr/offline/0197-offline-kernel-code-uses-glm-for-vectors-and-matrices.md)

## The kernel target

`v3dlib_moya_kernel` is a static library built from `moya/libmoya/kernel/CMakeLists.txt`.

- **It compiles as C++20**, without exceptions and without RTTI. Under MSVC it has no `/EH` flag
  and defines `_HAS_EXCEPTIONS=0`, so the standard headers do not throw. The top-level build
  leaves `/EHsc` off any target whose `V3D_KERNEL` property is on, and puts it on every other.
  Under Clang and GCC it has `-fno-exceptions` and `-fno-rtti`.
- **Its contraction rule is PUBLIC.** A source that links the target and compiles a kernel header
  gets `-ffp-contract=off` under Clang and GCC too, so a kernel function inlined into host code
  rounds as it does in the kernel. MSVC's `cl` fuses only under `/fp:contract` or `/fp:fast`,
  which the tree never gives.
- **`Kernel.cxx` includes every kernel header**, and does nothing else. Each include carries
  `// IWYU pragma: keep`, since clang-tidy otherwise finds nothing in the file that uses it.
- **`Qualifiers.h` defines `MOYA_KERNEL`**: `__host__ __device__` under nvcc and HIP, and
  nothing on the CPU.

## Building moya without the apps

`V3D_BUILD_MOYA` adds `moya/` to a build whose `V3D_BUILD_APPS` is off: libmoya, the kernel
target, `moya.exe` and moya's suite. moya needs neither a window nor a device, so the Linux job
that builds the api libraries under the sanitizers builds and tests moya too. The api libraries
moya links are added to a narrowed `V3D_LIBRARIES`. Without the apps, moya has no icon and no
install rule.
