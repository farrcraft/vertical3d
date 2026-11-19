# Kernel code

This page is for anyone writing moya's kernel code. Kernel code is the part of moya that
every device compiles from one source: the CPU now, and HIP and CUDA when their back ends exist.
It lives in `moya/libmoya/kernel/`, in the namespace `v3d::moya::kernel`. Everything outside that
directory is host code, which calls kernel code and is never called by it.

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
| Includes only kernel headers, glm, NanoVDB's `nanovdb/NanoVDB.h`, and `cfloat`, `climits`, `cmath`, `cstddef`, `cstdint`, `limits` and `type_traits` | These are what every device compiler provides, with the two libraries whose code runs on every device | The gate |
| C++20 | The newest standard both HIP and nvcc accept. The rest of the tree is C++23 | The target |
| No multiply and add fused into one rounding | MSVC, Clang and GCC then round kernel code alike | The target, for itself and for every source that links it |
| Every function carries `MOYA_KERNEL` | A GPU compiler builds a function for the device only when it is marked | Review |
| Every table at namespace scope carries `MOYA_KERNEL_DATA` | A GPU compiler places a copy in device memory only when it is marked | Review |
| Every header is included by `Kernel.cxx` | A header is compiled under the kernel's rules only through a source of the kernel target | The gate |

A deleted function, `= delete`, is not an allocation, and the gate does not report one.

Vectors, matrices and quaternions are glm's, with glm's SIMD intrinsics left off. glm marks each
of its functions as host and device code under HIP and nvcc. Sampled spectra and the lanes of a
SIMD traversal are moya's own types.

Background: [ADR-0191](../adr/offline/0191-offline-one-c-kernel-source-for-every-device.md),
[ADR-0197](../adr/offline/0197-offline-kernel-code-uses-glm-for-vectors-and-matrices.md)

## Reading a grid

Kernel code reads a volume's grids through NanoVDB's own header, by way of
`kernel/volume/Nano.h`, which a kernel header includes in its place. NanoVDB marks its functions
for the same compilers `MOYA_KERNEL` marks moya's. MSVC reports code it cannot reach in NanoVDB's
header once it inlines it, so `Nano.h` switches that warning off around it. NanoVDB chooses its
type traits by `__cplusplus`, so the kernel target compiles with `/Zc:__cplusplus` on MSVC, and so
does every source that links it.

- **A grid is a NanoVDB buffer of floats.** The host reads it from its OpenVDB file and converts it
  with NanoVDB's own tools, `volume::read()`, which keep each node's least and most. Each buffer
  is held as `kernel::volume::Block`s, runs of NanoVDB's 32-byte alignment, and every grid's
  follows the one before in one array.
- **A lookup is trilinear**, `volume::valueAt()`, between the eight voxels from a point's floor to
  one past it, in the grid's index space.
- **A cell bounds the lookups in it**, `volume::cellAt()`, as
  [the integrator](Integrator.md#tracking-through-volumes) says.

Background: [ADR-0245](../adr/offline/0245-offline-grids-are-held-as-nanovdb-on-every-device.md)

## Reaching a traversal

Kernel code names no device's traversal. A stage that traces rays takes its traversal as a template
parameter, and a device instantiates the stage with its own: the CPU's is `device::cpu::Traversal`,
and `kernel::geometry::BruteForce` is the oracle every traversal is checked against. A traversal
gives `nearest(ray, filter)` and `any(ray, filter)`, which the concept `Traversal` names.
[Traversal.md](Traversal.md#the-traversal-a-stage-takes) states the rules, and
[Devices.md](Devices.md#the-cpus-instruction-sets) the CPU's builds for each instruction set.

Background: [ADR-0204](../adr/offline/0204-offline-stages-take-their-traversal-as-a-type.md)

## Reaching textures

Kernel code names no device's texture access either. A stage that runs a material's program takes
the access as a second template parameter, after its traversal. A device instantiates the stage
with its own: the CPU's is `device::cpu::Textures`, and `kernel::texture::Resident` is the oracle
every access is checked against. An access gives a texel of a texture's level, which the
concept `kernel::texture::Texels` names. [Textures.md](Textures.md#the-texture-access) states the
rules.

Background: [ADR-0229](../adr/offline/0229-offline-stages-take-their-texture-access-as-a-type.md)

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
  nothing on the CPU. It defines `MOYA_KERNEL_DATA` too: `__device__` under nvcc and HIP, and
  nothing on the CPU.

## Building moya without the apps

`V3D_BUILD_MOYA` adds `moya/` to a build whose `V3D_BUILD_APPS` is off: libmoya, the kernel
target, `moya.exe` and moya's suite. moya needs neither a window nor a device, so the Linux job
that builds the api libraries under the sanitizers builds and tests moya too. The api libraries
moya links are added to a narrowed `V3D_LIBRARIES`. Without the apps, moya has no icon and no
install rule.
