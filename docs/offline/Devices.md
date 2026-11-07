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
- **Kernel code sees a buffer as a `kernel::Array<T>`**: a pointer and a 32-bit count, which owns
  nothing and checks no index. `device::view<T>(buffer)` makes one, holding as many whole values
  as fit. A buffer of more values than a 32-bit index reaches throws `std::out_of_range`.

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

A suite that launches on the CPU device finalizes TBB in its global fixture, so that TBB's
worker threads end before the debug runtime looks for leaks. moya's suite does this in
`TestMain.cxx`.

Background: [ADR-0196](../adr/offline/0196-offline-the-cpu-device-runs-on-onetbb.md)
