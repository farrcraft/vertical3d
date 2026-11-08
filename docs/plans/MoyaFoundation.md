# Moya Foundation — Devices, Spectra, Samples, a Film and EXR Output

Drafted 2026-11-07 against `8c577437`. This plan takes up section 1 of
[the moya rebuild roadmap](../roadmap/MoyaRebuild.md). When it closes, the new core has a CPU
device that runs kernel code over tiles of an image on every core. It has spectral types that
carry four wavelengths, a ZSobol sampler, a film that holds any number of AOVs, and output to
OpenEXR and through OpenColorIO. Nothing in it renders geometry yet. Sections 2 and 3 of the
roadmap build on it. The reyes hider and the ray hider keep running beside it, and no picture
they make changes.

The rebuild's decisions are ADR-0186 to ADR-0194, all accepted. This plan follows four of them:

* [ADR-0187](../adr/offline/0187-offline-moya-owns-its-algorithms-libraries-own-formats.md):
  moya's own algorithms, and the reference libraries for OpenEXR and OpenColorIO.
* [ADR-0189](../adr/offline/0189-offline-light-is-carried-as-sampled-spectra.md): four sampled
  wavelengths, with the colour type fixed when moya is compiled.
* [ADR-0191](../adr/offline/0191-offline-one-c-kernel-source-for-every-device.md): one kernel
  subset of C++, and a device layer.
* [ADR-0194](../adr/offline/0194-offline-stateless-samples-one-picture-per-device.md): ZSobol,
  and the same picture on every run of a device.

The plan keeps the roadmap's order, with two changes:

* **Five decisions the roadmap leaves open are steps of their own**, each before the step that
  builds on it. They are where the new core lives, how the CPU device runs threads, the kernel's
  vector types, the film's working space, and how a sample reaches the film.
  [Decisions](#decisions).
* **The dependencies come before the output steps.** OpenEXR, Imath and OpenColorIO are not in
  the tree, and adding a package touches the Flatpak and the notices as well as vcpkg.
  [Step 12](#step-12--openexr-imath-and-opencolorio-in-the-tree).

## What the survey found

The tree was read at `8c577437`. Line numbers are from that commit.

**The film and the sampler:**

* **`film::Film` splats each sample** into every pixel within half the filter's width of it, and
  holds a weighted sum per pixel (`api/render/offline/film/Film.h:30`, `add` at `:62`). Its
  colour is a `glm::vec3` of RGB, and its planes are colour, opacity, coverage and depth.
* **`film::Sampler` stratifies and jitters** a grid of samples per pixel, from a generator
  seeded by the pixel's column and row (`film/Sampler.h:26`, `pixel` at `:54`). It returns a
  `std::vector` per pixel.
* **The five pixel filters RI names** are in `film/Sampling.h:19-47`, each with RI's formula and
  a width per axis. These carry over. The film and the sampler around them do not.

**Threads:** nothing in `api/render/offline`, `moya` or the editor's render code starts a
thread. moya renders on one core.

**The libraries:**

* **`v3dlib_render_offline` is an api library.** It links log, image, type and glm publicly, and
  Boost.Filesystem privately (`api/render/offline/CMakeLists.txt:99-103`). Its package list is in
  `cmake/v3dApiLibraries.cmake`.
* **`v3dlib_moya` is not.** It is added with a plain `add_library` in
  `moya/libmoya/CMakeLists.txt:1`, holds RI's state and the two hiders, and links
  `v3dlib_render_offline`.
* **The editor links both** (`vertical3d/CMakeLists.txt:146` and `:157`). `MoyaDelegate`
  includes libmoya's `RIBHandler` (`vertical3d/src/render/MoyaDelegate.cxx:13`).

**Packages:**

* **OpenEXR, Imath and OpenColorIO are neither in `vcpkg.json` nor installed.**
* **TBB is installed only as OpenUSD's dependency**, under the manifest's `usd` feature. OIDN,
  which section 6 adds, needs it too.
* **`api/image` has no float pixel type.** `Image` holds bytes at a bit depth
  (`api/image/Image.h:22`, `:42`).

**The compilers:**

* **The tree compiles as C++23** (`CMakeLists.txt:37`).
* **portable.yml's clang job builds every api library that needs neither a window nor a device**
  with Clang under the sanitizers, `v3dlib_render_offline` among them. Its whole job builds the
  tree with Clang and with GCC.

---

## Decisions

| ADR | Decides | Step |
|---|---|---|
| 0195 | Where the new core lives | [1](#step-1--the-record-where-the-new-core-lives) |
| 0196 | How the CPU device runs threads | [2](#step-2--the-record-the-cpu-devices-threads) |
| 0197 | The kernel's vector and math types | [3](#step-3--the-record-the-kernels-vector-types) |
| 0198 | The film's RGB working space | [7](#step-7--the-record-the-working-space) |
| 0199 | How a sample reaches the film | [10](#step-10--the-record-how-a-sample-reaches-the-film) |

Each goes in as `proposed`, and is shown to the owner before the step that builds on it starts.
Each record step researches how production renderers make the choice first. The lean given in
each step is where the survey points, not a conclusion.

---

## The steps

| | What | Where | ADR | State |
|---|---|---|---|---|
| [1](#step-1--the-record-where-the-new-core-lives) | The record: where the new core lives | `docs/adr` | **0195** | done |
| [2](#step-2--the-record-the-cpu-devices-threads) | The record: the CPU device's threads | `docs/adr` | **0196** | done |
| [3](#step-3--the-record-the-kernels-vector-types) | The record: the kernel's vector types | `docs/adr` | **0197** | done |
| [4](#step-4--the-kernel-subset-as-a-target) | The kernel subset as a target, with its rules checked | `moya/libmoya`, `scripts` | 0195, 0197 | done |
| [5](#step-5--the-device-interface-and-the-cpu-device) | The device interface, and the CPU device over tiles | `moya/libmoya` | 0196 | done |
| [6](#step-6--sampled-spectra) | Sampled spectra, hero wavelengths and the CIE observer | `moya/libmoya` | — | done |
| [7](#step-7--the-record-the-working-space) | The record: the working space | `docs/adr` | **0198** | done |
| [8](#step-8--rgb-to-spectrum) | RGB to spectrum by Jakob and Hanika's table, and the RGB build | `moya/libmoya` | 0198 | done |
| [9](#step-9--the-zsobol-sampler) | The ZSobol sampler | `moya/libmoya` | — | done |
| [10](#step-10--the-record-how-a-sample-reaches-the-film) | The record: how a sample reaches the film | `docs/adr` | **0199** | done |
| [11](#step-11--the-film) | The film: AOV channels, fixed-order sums and snapshots | `moya/libmoya` | 0199 | open |
| [12](#step-12--openexr-imath-and-opencolorio-in-the-tree) | OpenEXR, Imath and OpenColorIO in the tree | `vcpkg.json`, `cmake`, `flatpak` | — | open |
| [13](#step-13--exr-output) | EXR output: half and float channels, several parts | `moya/libmoya` | — | open |
| [14](#step-14--colour-management) | Colour management through OpenColorIO's ACES 2.0 configuration | `moya/libmoya` | 0198 | open |
| [15](#step-15--the-foundation-end-to-end) | The foundation end to end: a colour chart rendered to EXR | `moya/libmoya` | — | open |
| [16](#step-16--the-documents) | The reference pages for the new core | `docs` | — | open |

## What blocks what

```
1 ADR ──┬──> 4 Kernel target ──> 5 Devices ──┐
3 ADR ──┘                                    │
2 ADR ──────────────────────────> 5          │
6 Spectra ──> 7 ADR ──> 8 RGB to spectrum ───┤
9 Sampler                       (after 4) ───┼──> 15 End to end ──> 16 Documents
10 ADR ──> 11 Film              (after 6) ───┤
12 Packages ──┬──> 13 EXR       (after 11) ──┤
              └──> 14 Colour    (after 7) ───┘
```

Steps 1 to 3 depend on nothing, and step 4 needs the records of steps 1 and 3. Step 6 can start
once step 4 lands, since spectra are kernel code. Steps 9 and 12 can start at any point after
step 4. Step 15 needs everything before it. Each step updates its own section of the reference
documents, and step 16 checks that they read as a whole.

## How every step is verified

* **The build and the gates.** Everything compiles at `/W4 /WX`, and cpplint, `/analyze`,
  clang-tidy and every review gate stay clean.
* **The suites.** The offline suite and moya's suites pass. Each step with code adds cases, and
  `failsfirst.ts` is run on the change to show that each new case fails without it.
* **Linux.** portable.yml's clang job builds `v3dlib_moya` and its kernel target, and runs moya's
  suite under the sanitizers. Step 4 widens the job to reach them.
* **No picture changes.** The old hiders are not touched, so every committed PNG in
  `moya/tests/data/` stays as it is.

---

### Step 1 — The record: where the new core lives

**The question.** Which library holds the device layer, the kernel code and the new film, and
in which directories. The old film, `trace` and `sl` stay in place until section 7 of the
roadmap removes them.

**The options:**

* **New directories in `v3dlib_render_offline`.** The editor and other repositories already link
  it, and an api library is what another repository builds as source. After section 7 it holds
  only the new core, the RIB reader and the lexer.
* **A new api library beside it.** A clean break, at the cost of a second offline library until
  section 7, and a rename or a merge after.
* **`moya/libmoya`.** moya's own library, but not an api library, so a pipeline in another
  repository could not build it as the api's other libraries are built.

**Drafted** as [ADR-0195](../adr/offline/0195-offline-the-renderers-core-lives-in-libmoya.md),
proposed: the third option. Every client of the core is built in this tree, and a pipeline
elsewhere reaches moya through a Hydra plugin or through files, never by building it from
source. Only the first two options would have put the core in the api, which serves no consumer.
ADR-0022's rule then holds as it stands. How the new film is told apart from the old one is a
name, not a decision, so step 11 chooses it.

### Step 2 — The record: the CPU device's threads

**The question.** How the CPU device spreads tiles over cores, and later how the BVH build of
section 3 spreads its work.

**The options:**

* **oneTBB.** Work stealing, nested parallelism, and one scheduler shared with OpenUSD and OIDN,
  which both use it. A Hydra host that already runs TBB is not oversubscribed by moya.
* **A pool of moya's own over `std::thread`**, as pbrt-v4 has. No package, and nested work needs
  a design of its own.

**Drafted** as [ADR-0196](../adr/offline/0196-offline-the-cpu-device-runs-on-onetbb.md),
proposed: oneTBB, for the scheduler a host shares and for cancellation. The standard library's
parallel algorithms and OpenMP are rejected beside a pool of moya's own. oneTBB moves from the
`usd` feature into the base manifest at step 12, so the Linux clang job builds it too.

### Step 3 — The record: the kernel's vector types

**The question.** Which vector, matrix and colour types kernel code uses. Whatever is chosen has
to compile under MSVC, Clang, GCC, HIP and nvcc, and round the same way on each CPU compiler.

**The options:**

* **glm**, as the rest of the tree uses. glm supports CUDA, and the step finds whether it
  compiles under HIP's Clang without changes.
* **Types of moya's own**, as Cycles and pbrt-v4 have, sized for the kernel and with conversions
  to glm at the scene interface.

**Drafted** as
[ADR-0197](../adr/offline/0197-offline-kernel-code-uses-glm-for-vectors-and-matrices.md),
proposed: glm. The installed glm 1.0 detects HIP as it detects nvcc, and marks every function as
both host and device code under each (`glm/detail/setup.hpp`, `GLM_CUDA_FUNC_DEF`). The tree
turns on none of glm's intrinsics. Imath is rejected beside types of moya's own. Spectra and
SIMD lanes are moya's own types whichever way the record went.

### Step 4 — The kernel subset as a target

**Done.** `v3dlib_moya_kernel` is built from `moya/libmoya/kernel/`, at C++20, with `/GR-`,
no `/EH` flag and `_HAS_EXCEPTIONS=0` under MSVC, and `-fno-exceptions -fno-rtti` elsewhere. Its
`-ffp-contract=off` is PUBLIC. CMake seeds `/GR` and `/EHsc` into every MSVC command line, so the
root strips both, and puts `/EHsc` back on every target whose `V3D_KERNEL` property is off.
`Qualifiers.h` defines `MOYA_KERNEL`, and `Kernel.cxx` includes every kernel header. `kernel.ts`
reports the six rules [Kernel.md](../offline/Kernel.md#the-rules) states, and each rule broken
by hand fails one of its tests. `V3D_BUILD_MOYA` builds moya without the other apps, and
portable.yml's clang job turns it on. moya's suite passes there under both sanitizers, in 320
seconds. The kernel target holds no code yet: step 6's spectra are the first.

* **A kernel target** that compiles the kernel directories with exceptions and RTTI off.
  Everything else in the library calls it, and it calls nothing in the library outside the
  kernel.
* **The standard kernel code is held to**: the newest that HIP and nvcc both accept, which is
  older than the tree's C++23. The compilers force it, so it is a rule in the reference page, not
  a record.
* **Contraction pinned**: every CPU compiler compiles kernel code with the same rule for fusing a
  multiply and an add, so MSVC, Clang and GCC round alike.
* **A review gate** in `scripts/` that rejects a virtual function, `new`, `delete`, `throw`, a
  standard container or a `boost::shared_ptr` in the kernel directories. It reads the source, so
  it checks what a GPU compiler would reject before any GPU build exists.
* **A host-and-device qualifier** in one header, empty on the CPU, which every kernel function
  carries.
* **portable.yml's clang job widened** to build `v3dlib_moya`, its kernel target and moya's
  suite, under the sanitizers and with no window or device. The job builds api libraries alone
  today (`scripts/portable/build.sh`, with `V3D_BUILD_APPS=OFF`), so moya's library needs a way
  to build without the apps.

**Tests:** the gate's rules, in its own test, as every gate has. The kernel target builds on
Windows and in portable.yml, and moya's suite runs there under the sanitizers.

### Step 5 — The device interface and the CPU device

**Done.** `device::Device` allocates, uploads and downloads `Buffer`s, which kernel code sees as
`kernel::Array<T>`. A launch is on each device's own class rather than on `Device`, since a GPU
starts entry points it compiled where the CPU calls a function. `CpuDevice::launch` runs a
`kernel::Function<Params>` over 32-pixel tiles in a TBB arena of a given width, behind
`device/internal/Arena.h`. No wait is needed while the CPU is the only device, since its launch
returns when every pixel has run. Traversal and texture lookups are left to the sections that
build them, rather than declared with nothing behind them. The manifest's `moya` feature holds
oneTBB, OpenUSD never named it directly, and the clang job names the feature.
[Devices.md](../offline/Devices.md) states the rules.

Two fixes outside moya came with it. `linkrule.ts` read every header below an app library's
directory as that library's own, so the kernel target's headers did not count as a link of
libmoya's. It now takes the library that owns a header, and its package table holds TBB's
headers. moya's suite also finalizes TBB in its global fixture, because TBB's workers outlive
`main` and the debug runtime reported what they held as leaks.

* **The device interface**: buffers of device memory, a launch of a kernel function over the
  tiles of an image, and a wait. Ray traversal and texture lookups are declared here and
  implemented by sections 3 and 5 of the roadmap.
* **The CPU device**: buffers in host memory, and tiles run by the threads step 2 decides.
* **moya's manifest feature, with oneTBB in it.** oneTBB leaves the `usd` feature. The Linux
  clang job builds without `usd`, so it has no TBB until moya's feature carries it, and it turns
  the feature on. Step 12 adds the other packages to the same feature.
* **The rule that a launch is deterministic**: each pixel's result depends only on its own
  inputs, so the order tiles finish in cannot change a picture.

**Tests:** a launch fills every pixel of an odd-sized image once. A kernel that writes a hash of
each pixel's coordinates gives the same image on one thread and on every thread.

### Step 6 — Sampled spectra

**Done.** `SampledSpectrum` and `SampledWavelengths` are in `kernel/spectrum/`, with
`observer()`, `toXyz()`, `d65()` and `illuminantA()`. Two changes from the list below:

* **The wavelengths are spread in the distribution, not at equal steps in nanometres.** The
  hero is drawn from a density that follows the observer's luminance, and the other three sit at
  the same point offset by quarters of the distribution, as pbrt-v4 draws them. One random
  number still places all four.
* **Illuminant A comes from its defining formula**, a Planckian radiator at 2848 K, not a
  table. Only the observer and D65 are tables.

The CIE's tables are CC BY-SA 4.0, which their metadata states and their pages do not. The
owner chose to carry them, in two files that hold only the data, each marked with that licence
and the CIE's citation. `notices.ts` adds `CIE-NOTICE.txt` to the notices file, and reports it
missing. `MOYA_KERNEL_DATA` marks a table kernel code reads. D65's chromaticity from the tables
is (0.31272, 0.32903), as published. [Spectra.md](../offline/Spectra.md) states the rules.

* **A sampled spectrum** of four values, with the arithmetic the integrator needs.
* **Sampled wavelengths**: a hero wavelength drawn over the visible range, with three more at
  equal steps, and the probability of each.
* **The CIE 1931 two-degree observer** at one-nanometre steps, as a table in the source, with
  conversion from a sampled spectrum to XYZ.
* **The CIE illuminants D65 and A**, as tables in the source.

**Tests:** D65 converts to its published white point in XYZ. A constant spectrum converts to an
equal-energy point. Hero sampling's estimate of a known spectrum's XYZ converges to the
integral of its table.

### Step 7 — The record: the working space

**The question.** Which RGB space the film holds and RGB inputs are read in when they name none.
Every RGB value a scene gives is converted into it before its spectrum is found.

**The options:**

* **ACEScg**, the rendering space of ACES and OpenColorIO's configurations, with a gamut wide
  enough for any display.
* **Linear Rec. 709**, the gamut of sRGB, which most texture files are authored in.
* **A space the scene names**, at the cost of a spectrum table built per space.

**Drafted** as
[ADR-0198](../adr/offline/0198-offline-the-working-space-is-opencolorios-scene-linear-role.md),
accepted. The question changed in the drafting. In a spectral renderer the space the film stores
in decides nothing, since XYZ to any RGB space is one matrix, which commutes with a sum of
samples. What matters is the space outputs are written in and untagged RGB is read in, and the
gamut of the spectrum table. The record takes the working space from OpenColorIO's scene-linear
role, ACEScg in the default configuration, and builds the table for ACEScg alone.

### Step 8 — RGB to spectrum

**Done.** `moya_rgbtable` fits the table at
64 points along each axis of each cube, on oneTBB's threads, in about 25 seconds in a Debug build.
libmoya compiles the source it writes, and `colour::acescgTable()` views it. The kernel's
`RgbTable`, `reflectance()`, `unbounded()` and `illuminant()` read it. Three things the list
below did not foresee:

* **The fit needs ACES white as a spectrum,** which no CIE table gives. CIE daylight at 6000 K
  has the ACES white point's chromaticity to five decimal places, so ACES white is that daylight.
  The CIE's daylight components are a third table, under the same licence as the other two.
* **The corners of ACEScg cannot be tested.** About one point in eight of the table is a colour
  no reflectance has, ACEScg's most saturated, and the fit gives the nearest. The tests take
  sRGB's primaries and secondaries in ACEScg instead, with Bradford adaptation, and they return
  within 0.005 on each channel, as the 24 moderate colours of the chart do.
* **The generated source is skipped by clang-tidy and `/analyze`**, which would spend minutes on
  its literals.

`V3D_MOYA_RGB` builds the colour type as ACEScg's three channels, and each spectral function has
its RGB counterpart. Three cases of the spectrum suite, and one and a half of the RGB suite, need
wavelengths, and an RGB build leaves them out. A case for the RGB build's own behaviour, and one
that checks its illuminants' colours in both builds, take their place. portable.yml's clang job
builds moya both ways and runs its suite in each.
[Spectra.md](../offline/Spectra.md#rgb-to-spectrum) states the rules.

* **A tool of moya's own, in C++, that builds Jakob and Hanika's table** for the working space,
  by fitting a three-coefficient sigmoid polynomial at each point of a grid over the gamut. It
  runs at build time, as a shader is compiled at build time, so the table is not committed.
* **Lookup**: an RGB value gives the polynomial's coefficients by interpolating the table, and
  its spectrum is evaluated at the path's wavelengths. Reflectances are bounded to one. Emission
  and unbounded values are scaled from a bounded spectrum.
* **The RGB build.** A CMake option makes the colour type three RGB values rather than four
  wavelengths, as ADR-0189 keeps possible. CI builds and tests both.

**Tests:** the 24 patches of a colour chart, and the corners of the gamut, return to their RGB
within a stated tolerance after conversion to a spectrum and back through the observer.

### Step 9 — The ZSobol sampler

**Done.** `sample::ZSobol` is in `kernel/sample/`, with `get1D` and `get2D` of a pixel, an
index and a dimension. One change from the list below: **there is no table of direction
numbers, and no notice.** ZSobol draws every dimension from the Sobol sequence's first two,
which it tells apart by reordering the index's digits and by scrambling. Those two dimensions'
matrices are the same in every construction of the sequence, van der Corput's and Pascal's
triangle mod 2, so `SobolMatrices::make()` computes them, and Joe and Kuo's data is not needed. The
scrambling is Laine and Karras's hash with Vegdahl's improvement. The (0,2)-net test runs for
every power of two from 1 to 256 samples, odd and even. [Sampler.md](../offline/Sampler.md)
states the rules.

* **Sobol generator matrices** from Joe and Kuo's direction numbers, as a table in the source.
  The notices file gains their licence.
* **Owen scrambling** by a hash of the index, the dimension and the seed.
* **Morton order over pixels**, so neighbouring pixels draw from disjoint parts of one
  sequence, as pbrt-v4's `ZSobolSampler` does.
* **A seed that includes the frame number.** A sample is a function of its pixel, index,
  dimension and seed, and holds no state.

**Tests:** the same pixel, index, dimension and seed give the same value on every call. The first
two dimensions of a power-of-two count of samples in one pixel are stratified as a (0,2)-net. An
analytic integral converges with less error than independent random numbers at the same count.
Two frames' seeds give different samples.

### Step 10 — The record: how a sample reaches the film

**The question.** Whether a sample is splatted into every pixel under its filter, as the old
film does, or lands in the one pixel it was drawn for, with its position drawn from the filter.

**The options:**

* **Filter importance sampling**, as pbrt-v4 and Cycles do. A pixel's value depends only on its
  own samples, so tiles need no locks or shared edges, and the denoiser of section 6 sees pixels
  whose noise is independent. A filter with negative lobes needs its weights carried with the
  sample.
* **Splatting**, as the old film does. Exact for every filter, but tiles write into one
  another's pixels, and neighbouring pixels' noise is correlated.

**Drafted** as
[ADR-0199](../adr/offline/0199-offline-a-sample-lands-in-the-one-pixel-it-was-drawn-for.md),
accepted: filter importance sampling. pbrt-v4 and Cycles do the same. RenderMan offers both, and
its denoiser needs the importance-sampled mode, because it assumes no sample is shared between
pixels. A one-pixel box, where the two coincide, is rejected beside splatting.

### Step 11 — The film

* **AOV channels**: the beauty, then any number of named channels, each a float per component.
* **Sums in a fixed order**: each pixel's samples are added in sample order, so a picture is the
  same under any thread count.
* **Snapshots**: a copy of the film's current state on request, while sampling continues, for
  the editor's render view in section 9.
* **The pixel filters**, from `film::Sampling`, reached as step 10 decides.

**Tests:** a pixel's value is the weighted mean of its samples. A snapshot taken during sampling
is a complete picture of the samples taken so far. Filtered samples drawn from each of RI's
filters reproduce the filter's shape.

### Step 12 — OpenEXR, Imath and OpenColorIO in the tree

* **The three ports** in moya's manifest feature in `vcpkg.json`, which step 5 adds with oneTBB.
  `examples/starter/vcpkg.json` builds no moya, so it does not take them.
* **Found** in `cmake/v3dDependencies.cmake`, and linked by `v3dlib_moya` alone.
* **The Flatpak** takes each from the Freedesktop runtime or as a pinned module, as
  [Dependencies.md](../contributing/Dependencies.md#the-flatpaks-packages) describes.
* **The notices check** finds a copyright file for each, and for each package they bring with
  them.

**Tests:** the configure, flatpak.yml and the notices check.

### Step 13 — EXR output

* **A writer from the film to OpenEXR**: the beauty as half or float RGBA, each AOV as named
  channels, and a part per output when several are asked for.
* **The header** carries the camera's matrices, the working space's chromaticities and a
  render time.
* **Compression** is chosen per output, with ZIP as the default.

**Tests:** a film written and read back through the OpenEXR library holds the same values,
exactly in float and within half's precision in half. A file of several parts holds each part's
channels and no others.

### Step 14 — Colour management

* **OpenColorIO's built-in ACES 2.0 configuration**, so no configuration file ships. A path to
  another configuration can be given.
* **A display transform** from the working space to a display and view, for 8-bit output and for
  the editor's render view. EXR output stays in the working space.
* **Input colour spaces**: a colour or texture names its space, and is converted into the
  working space before step 8's lookup.

**Tests:** a working-space value through the sRGB display's view matches OpenColorIO's own
answer. An sRGB input converts to the ACEScg value the configuration gives.

### Step 15 — The foundation end to end

**A test renders a colour chart** with no geometry. Each pixel samples wavelengths and filter
positions, takes its patch's RGB through step 8's spectrum under D65, and adds the result to the
film. The film is written to EXR and through the sRGB display to PNG.

* **The PNG is a reference image**, compared as moya's references are.
* **The EXR's patches** match the chart's ACEScg values within the noise of the sample count.
* **The same test on one thread and on every thread** gives the same file.

### Step 16 — The documents

* **New reference pages** in `docs/offline/` for the kernel subset and the device layer, for
  spectra and the working space, and for the sampler and the film. The docs site's summary lists
  them.
* **[The offline renderer](../offline/README.md)** says that the new core sits beside the old
  hiders until section 7 of the roadmap.
* **[Dependencies.md](../contributing/Dependencies.md)** lists OpenEXR, Imath, OpenColorIO, and
  oneTBB, and the manifest feature that holds them.
* **Each record's `Documented in` line** names the reference page that states its rule, in
  place of this plan.
* **The roadmap's section 1** links here, and says so when this plan closes.
