# Devices

This page is for anyone changing how moya's new core holds memory or runs kernel code. A device
is a processor moya renders on: the CPU now, and one kind of GPU for each back end added later.
The device layer is host code, in `moya/libmoya/device/` and the namespace `v3d::moya::device`.
[Kernel.md](Kernel.md) covers the kernel code a device runs.

## Memory

- **`Device` allocates, uploads and downloads.** `allocate(bytes)` returns a `Buffer` whose
  contents are undefined until something writes them. `upload` copies from host memory into the
  start of a buffer, and `download` copies out of it. A copy larger than its buffer throws
  `std::out_of_range`.
- **A `Buffer` belongs to the device that allocated it.** `address()` is where kernel code finds
  it, on that device. On a GPU the host cannot read through it.
- **Every buffer is aligned to `Buffer::kAlignment`, 64 bytes.** That is enough for any type
  kernel code reads, such as the hierarchy's 32-byte aligned nodes.
- **Kernel code sees a buffer as a `kernel::Array<T>`**: a pointer and a 32-bit count, which owns
  nothing and checks no index. `device::view<T>(buffer)` makes one, holding as many whole values
  as fit. A buffer of more values than a 32-bit index reaches throws `std::out_of_range`, and one
  whose address is not aligned throws `std::invalid_argument`. A type aligned more than a buffer
  does not compile.

## Launching kernel code

A launch belongs to each device's own class, not to `Device`, because each device starts kernel
code its own way. The CPU calls a function directly, and a GPU starts an entry point it compiled.

`CpuDevice::launch(width, height, params, function)` runs a kernel function once for each pixel
of an image, and returns when every pixel has run. The function is a
`kernel::Function<Params>`: it takes the parameters, which it only reads, and the
`kernel::Item` it is run for, which holds the pixel's x and y from the upper left.

**A pixel's result depends only on its own inputs.** A kernel function writes only what is its
own pixel's, such as its slot in an output array. Pixels run in any order and on any thread, so
nothing else is safe, and a picture is then the same whatever order tiles finish in and however
many threads run them.

## The CPU device

- **Memory** is host memory.
- **A launch runs in tiles** of 32 by 32 pixels, smaller at the right and bottom edges. One
  thread runs a tile's pixels one after another, row by row.
- **Threads come from oneTBB.** Each `CpuDevice` has a TBB arena. It runs on at most the threads
  its constructor names, or on as many as TBB gives the process when it names 0. A host that
  limits TBB's threads, as a Hydra host may, limits moya's too.
- **TBB is behind `device/internal/Arena.h`**, so only the device's source includes it, and
  libmoya links it PRIVATE.
- **An image with no pixels runs nothing.**

**A texture lookup calls the host.** The CPU's texture access reads each texel through a function
of libmoya's, which loads the texel's tile into the texture cache when the cache does not hold it,
as [Textures.md](Textures.md#the-texture-access) says.

**Host work runs on the same threads.** `CpuDevice::each(count, task)` runs a task once for
each index below a count, in any order and on any thread, and returns when all have run. A task
may call `each()` again, and its work joins the same threads. Each task writes only what is its
own index's. The hierarchy builder runs on it, so a host that limits TBB limits the build too.

A suite that launches on the CPU device finalizes TBB in its global fixture, so that TBB's
worker threads end before the debug runtime looks for leaks. moya's suite does this in
`TestMain.cxx`.

Background: [ADR-0196](../adr/offline/0196-offline-the-cpu-device-runs-on-onetbb.md)

## The CPU's instruction sets

The CPU's fused path is built once for each of SSE4.2, AVX2 and AVX-512, the x86-64 levels v2, v3
and v4. Each build is a shared library of its own, `moya_cpu_sse42`, `moya_cpu_avx2` and
`moya_cpu_avx512`, compiled whole for its instruction set from `device/cpu/Entry.cxx`. What they
share with libmoya is `v3dlib_moya_cpu`, a target of headers alone in `device/cpu/`: the
instruction sets, the node layout, the traversal and the table of entry points.

- **Nothing compiled for one instruction set reaches another.** A library links only the shared
  headers, the kernel's and glm, never libmoya. Its symbols are hidden but for the one function
  it exports, `moya_cpu_module`, which gives a `cpu::Module`: a table of entry points, each a
  kernel function a launch runs. Two sources in one binary that compiled the same inline
  function for different instruction sets would leave the linker free to keep the newer copy
  for both.
- **A table's parameters hold no instruction-specific type**, since libmoya and each library
  compile them from the same headers and have to agree on their layout. `cpu::Traversal`
  declares its functions in a header that libmoya sees, and only the libraries define them.
- **The device chooses when it is made.** `CpuDevice(threads, newest)` takes the older of
  `newest` and what the processor and its operating system run, which `newestIsa()` reports.
  A test names each instruction set in turn. `isa()` gives the choice.
- **The library is loaded the first time it is needed**, by `module()` through a `CpuLibrary`,
  from the running
  executable's directory, and stays loaded while the device lives. A library that cannot be
  loaded, exports no table, or whose table is of another version or another instruction set,
  throws `std::runtime_error`.
- **Each executable that traces has the libraries beside it.** `v3d_moya_cpu_libraries(target)`
  copies them there on every build, and an install puts them beside the apps.

A processor older than SSE4.2 does not run moya.

Background: [ADR-0206](../adr/offline/0206-offline-the-cpu-path-is-built-per-instruction-set.md),
[ADR-0210](../adr/offline/0210-offline-each-cpu-instruction-set-is-a-library.md)
