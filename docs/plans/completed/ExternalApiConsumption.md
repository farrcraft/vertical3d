# External API Consumption — The api Builds Inside Another Repository's Tree

Drafted 2026-09-05, **closed 2026-09-05**. Made the `api/` libraries usable by an application in a
different repository, by the route [ADR-0027](../../adr/0027-the-api-is-consumed-as-source.md)
settles: the consumer takes this repository as a submodule or through `FetchContent` and builds it
with its own compiler. Nothing is installed, exported or packaged.

**All seven steps landed.** [examples/starter/](../../../examples/starter/) is a project of its own
that subclasses the game engine, opens a window, draws a quad and is built on every push. Five things
turned up that this plan did not anticipate, and are in the step notes below: glm and EnTT were never
resolved at all, **the api libraries did not declare their dependencies on each other**, a stated
language standard collides with an interface requirement, the `SYSTEM` on an include path was doing
work, and moving the `glslc` check buys less than the plan claimed.

The tree has had exactly one reader of its CMake since it was written, and it showed. There was no
`install()` or `export()` anywhere; the only `target_include_directories` was the one inside
`v3d_add_shader` for generated shader headers; every header was reached by a relative path from
inside the tree; and the root `CMakeLists.txt` held all nine `find_package` calls, the compile
options, the four `v3d_add_*` helpers and an unconditional `add_subdirectory` for all ten apps.
`api/` could not be configured without the rest of it, and a target that left the tree carried
nothing — not its include path, not its third-party linkage, not the `/utf-8` its headers need.

Most of what follows was a correction in its own right rather than a concession to an external
consumer. A library that states its own include root and propagates its own dependencies is right
whether or not anything outside this repository ever links it; the second repository is only what
made the omission observable.

## Decisions

Recorded in [docs/adr/](../../adr/), not here.

| ADR | Decision |
|---|---|
| [0027](../../adr/0027-the-api-is-consumed-as-source.md) | The api is taken as source through a nestable root, not as an installed package — **step 1 wrote it** |
| [0022](../../adr/0022-offline-rendering-shares-an-api-library.md) | `api/render` builds two libraries, and the offline one names neither Vulkan nor SDL |

## What blocks what

Step 1 was the record and came first. Steps 2 to 6 all edit the same sixteen `CMakeLists.txt` files
under `api/` plus the root, so they were one pass in practice even though only step 2's `V3D_ROOT`
is a hard dependency of the rest. Step 7 is the only one that proves the pass finished, because
nothing else in this repository can tell a target that carries its own include path from one that is
picking it up off the root's global.

```
 1 ADR-0027 ─> 2 the root splits, V3D_ROOT ─┬─> 3 include roots and aliases ─┐
                                            ├─> 4 linkage on the libraries ──┼─> 7 a consumer CI configures
                                            ├─> 5 flags in the interface ────┤
                                            └─> 6 the shared data ───────────┘
```

## Steps

### Step 1 — ADR-0027, the api is taken as source

The record had to exist before step 2, because the shape of the root — options rather than install
rules — is one of the alternatives it weighs and not an obvious consequence of the others. It argues
four options: source consumption, an installed CMake package, a private vcpkg registry, and folding
the application back into this monorepo, which is free and is the right answer when the reason for
the split is only tidiness.

The two things it had to be honest about: a consumer builds the whole api on every clean build and
has no version but a submodule sha, and the vcpkg baseline in
[vcpkg-configuration.json](../../../vcpkg-configuration.json) has to be copied verbatim into the
consumer's own with nothing checking that it was. Boost here is static, so a baseline that drifts is
a link error rather than a warning.

**Landed.** Written, indexed, and accepted alongside the implementation.

### Step 2 — the root splits, and `V3D_ROOT` names this repository

The root did four jobs at once. Three of them moved.

- **[cmake/v3dDependencies.cmake](../../../cmake/v3dDependencies.cmake)** took the `find_package`
  calls, the `Boost_USE_STATIC_LIBS` and `Boost_NO_WARN_NEW_VERSIONS` settings that have to precede
  them, and **[cmake/v3dHelpers.cmake](../../../cmake/v3dHelpers.cmake)** took the four `v3d_add_*`
  functions. The root includes both. Splitting them is what lets a consumer include the helpers
  without re-running the dependency resolution, which is the seam the installed-package route would
  need too.
- **`V3D_ROOT` is set once at the root**, from `CMAKE_CURRENT_LIST_DIR`, and every path naming
  something in this repository goes through it. `CMAKE_SOURCE_DIR` is the consumer's root in a
  nested build, not this one — which was a live bug and not a hypothetical: `v3d_add_shared_data`
  copied from `${CMAKE_SOURCE_DIR}/data`, and step 6 is that bug.
- **`V3D_BUILD_APPS` and `V3D_BUILD_TESTS` gate what is not the api.** The ten app
  `add_subdirectory` lines and `enable_testing()` are behind the first; the sixteen
  `add_subdirectory("tests")` lines under `api/`, and the ones in `talyn/` and `moya/`, behind the
  second. The guard has to be on the `add_subdirectory` rather than inside `v3d_add_test`, because
  each `tests/CMakeLists.txt` names its target again afterwards — `api/image/tests/CMakeLists.txt`
  links `v3dtest_image` and hangs a fixture copy off it — so a helper that returned early would
  leave those referring to a target that was never created. Both default to `PROJECT_IS_TOP_LEVEL`.
- **The declared minimum was a fiction and is corrected.** `cmake_minimum_required(VERSION 3.8)` sat
  two lines above `cmake_policy(SET CMP0079 NEW)`, which arrived in 3.13; `PROJECT_IS_TOP_LEVEL`,
  which the option defaults want, arrived in 3.21. It declares 3.21.
- **The two `vendor/libnoise` entries moved to `voxel`**, from the root's global
  `include_directories(SYSTEM ...)` and `link_directories`, which were voxel's alone.

**Not as planned — the `glslc` move buys less than this said.** The check moved out of the root, but
into `v3d_add_shader` itself rather than into `api/render`: voxel calls that function too, so
`api/render` is not its only caller and the requirement belongs with the function. The claimed
benefit does not follow, though. `find_package(Vulkan)` is still unconditional, and `add_subdirectory("api")`
builds `v3dlib_render` and its four shaders whatever else is switched off, so **a consumer still
needs the Vulkan SDK**. What the move actually buys is that the requirement is stated where it is
incurred. The plan's "configures with no Vulkan SDK" was wrong and is struck; making it true would
mean a per-library dependency block, which is most of the installed-package work arriving early.

**Landed**, with that correction. `-DV3D_BUILD_APPS=OFF -DV3D_BUILD_TESTS=OFF` configures and builds
every `v3dlib_*` target and nothing else; a default configure builds and tests everything it did.

### Step 3 — every api library declares its own include root and an alias

- **The include root is `V3D_ROOT`, not `api/`.** api headers already reach each other by relative
  path, so the repository root is the only prefix under which they resolve unchanged. An external
  file writes `#include <api/image/Image.h>`, which is the path a reader sees in the tree. The
  `INSTALL_INTERFACE` half is deliberately not written: there is no install, and a half-filled
  export is worse than none.
- **Each target gained a `v3d::` alias**, which is what an external `CMakeLists.txt` names, and the
  name that would survive a later move to an installed package.
- **The root's `include_directories(SYSTEM ${Boost_INCLUDE_DIRS} ${SDL3_INCLUDE_DIRS}
  ${Vulkan_INCLUDE_DIRS})` came out**, once step 4 made every dependency arrive through an imported
  target.

**Not as planned — this was written as sixteen separate edits and became one function.**
`v3d_add_api_library` in the helpers module declares the target, the alias, the include root, the
interface flags of step 5 and the boost winapi workaround together. It is what collapsed sixteen
copies of the same six lines, and it is why the winapi definitions now reach the six libraries that
were missing them — an inconsistency that the workaround exists to prevent in the first place.

**Landed.** A `.cxx` file outside `api/`, inheriting none of the root's globals, compiles
`#include <api/image/Image.h>` after naming `v3d::image` alone.

### Step 4 — the third-party linkage moves onto the libraries that need it

[pong/CMakeLists.txt](../../../pong/CMakeLists.txt) named `${Boost_LIBRARIES}`, `${SDL3_LIBRARIES}`,
`${Vulkan_LIBRARIES}`, `${PNG_LIBRARIES}`, `JPEG::JPEG` and `Freetype::Freetype` itself, and every
other app repeated some subset. None of it belonged to an app: pong does not open a PNG or measure a
glyph, it links a library that does.

- **Each dependency moved to the library that uses it**: Freetype to `v3dlib_font`, Vulkan to
  `v3dlib_render`, PNG and JPEG to `v3dlib_image`, SDL3 to `v3dlib_input`, `v3dlib_render` and
  `v3dlib_engine`, and each boost component to the library whose sources use it.
- **The api libraries also gained their dependencies on each other**, which is the half of this step
  the plan did not see: `v3dlib_engine` used `event`, `input`, `config`, `asset` and `render` without
  naming any of them, and `v3dlib_render` used `font` and `image` the same way. Nothing in the tree
  noticed, because every app names all thirteen. The declared graph now matches the include graph
  exactly, and `v3d::engine` alone is enough to build an app.
- **PUBLIC or PRIVATE was decided by the header, not by convenience.** PUBLIC when a header of the
  library names the dependency's types — the rule `v3dlib_log`'s spdlog link already stated. The
  image codecs are reached only from `reader/` and `writer/`, so PNG and JPEG are PRIVATE; SDL is
  PUBLIC on `v3dlib_input` and PRIVATE on `v3dlib_engine` for the same test.

**Not as planned — glm and EnTT were never resolved at all.** Neither had a `find_package`, and
nothing linked them. They were reachable because `Boost_INCLUDE_DIRS` is the vcpkg installed include
directory, which also holds every header-only port, so the root's global include line made the whole
of vcpkg visible to every target in the tree. Removing that line is what surfaced it. Both are now
found and linked by the ten and seven libraries that name them in a header.

**Landed.** Every app's `target_link_libraries` names `v3dlib_*` targets and, where the app itself
parses a command line, `Boost::program_options`.

### Step 5 — the flags that are not cosmetic move into the interface

Nine of the sixteen api `CMakeLists.txt` end with `set(CMAKE_CXX_FLAGS "/utf-8")`. That is
directory-scoped: it reaches the library, its `tests/` subdirectory, and nothing else. An external
file that includes [Logger.h](../../../api/log/Logger.h) without it hits the `static_assert` in
spdlog's bundled fmt.

- **`/EHsc` and `/utf-8` are `INTERFACE` compile options** on every api library, set by
  `v3d_add_api_library`. INTERFACE rather than PUBLIC: the directory's own flags already carry both
  for this tree's compilation, and replacing that mechanism would restore the `/DWIN32 /D_WINDOWS`
  defaults those nine directories currently discard, which is a preprocessor change with nothing to
  do with this plan.
- **The `BOOST_USE_WINAPI_VERSION` trio was already `PUBLIC`** where it appeared. It is the model
  the other two follow, and it is now on all sixteen rather than nine.
- **`/std:c++latest` and `/permissive-` stay the consumer's to set**, stated in the contract in
  [docs/Dependencies.md](../../Dependencies.md) rather than forced through an interface property.

**Not as planned — the standard could not stay a raw flag.** `target_compile_features(INTERFACE
cxx_std_23)` was the plan's way of stating a floor, and it is unnecessary: glm and EnTT already
require `cxx_std_17` through their own interfaces, and CMake answers that by putting `/std:c++17` on
the command line beside the root's `add_compile_options("/std:c++latest")`. MSVC reports D9025 for a
command line naming two standards. The root now sets `CMAKE_CXX_STANDARD 23`, which CMake maps to
`/std:c++latest` on this compiler — the same flag, arrived at as a requirement that can be satisfied
rather than as a flag that can be contradicted.

**Landed.** A consumer file that includes `Logger.h` and sets no flags of its own compiles.

### Step 6 — the shared data resolves against this repository

`v3d_add_shared_data` copied `${CMAKE_SOURCE_DIR}/data` beside the target's executable. Nested,
`CMAKE_SOURCE_DIR` is the consumer's root, so an external app calling it would copy the consumer's
own `data/` — or nothing, silently, if there is none — and the fonts and themes the engine loads at
startup would simply be absent. It is `${V3D_ROOT}/data`.

`v3d_add_app_data` uses `${CMAKE_CURRENT_SOURCE_DIR}/data`, which is correct in either tree and is
unchanged: an external app's own assets are its own directory's.

**Landed.**

### Step 7 — a consumer, and CI that configures it

Nothing inside this repository can tell a target that carries its own include path from one picking
it up off the root's global, because the root's global is always there. Only an out-of-tree
configure separates them, so one lives here and runs on every push. Without this step the previous
five describe a build that worked once.

- **[examples/starter/](../../../examples/starter/)** is a complete CMake project that reaches the
  tree through `add_subdirectory` with both options off, subclasses `v3d::engine::Engine`, reads a
  window config, opens a window and draws a quad through the realtime engine. It sets its own
  language standard and `/permissive-` and nothing else — every other flag it needs arrives through
  the interface properties step 5 wrote, or the example is not testing anything. It was first written
  as a two-library link check against `v3d::image`, which passed while the engine's own link graph was
  incomplete; an example that names one leaf library tests almost nothing.
- **It is a project in its own right, not a subdirectory of this one.** Its root is its own, which
  is what makes `CMAKE_SOURCE_DIR` differ from `V3D_ROOT`, and what would have caught step 6's bug.
- **The walkthrough is written down** in [docs/NewProject.md](../../NewProject.md): the submodule
  layout, the manifest a consumer needs, the CMakeLists with the five things in it worth knowing, the
  app, the config, and how to tell a first run worked. [docs/Dependencies.md](../../Dependencies.md)
  keeps the vcpkg half and points at it.

**Not as planned — it is a step in `ctest.yml`, not a workflow of its own.** The plan argued for a
separate workflow because it needs the same vcpkg install and toolchain, which is the argument for
putting it in the same job: a second runner would install the Vulkan SDK and build boost again to
prove one link line. It runs after the test step, reuses that job's `vcpkg_installed`, and takes a
couple of minutes.

**Landed.** `examples/starter` configures and builds against the tree, and the executable opens
its window with a clean validation log.

## Verification

- **The tree builds exactly as it did.** Every target, default options, no new warnings. Steps 2 to
  6 are a build-system change with no source change, so anything that stopped compiling would have
  been a mistake rather than a consequence — and one did: `target_include_directories` without
  `SYSTEM` turned libnoise's headers into eight C4100 reports at `/W4`, where the root's global had
  been marking them `SYSTEM` all along. Both voxel targets name it now.
- **`ctest --test-dir out/build/x64-Debug --output-on-failure`** runs 22 suites, all passing, the
  same set as before the change.
- **The api-only configure** builds every `v3dlib_*` target and nothing else.
- **The starter example** builds, opens a window at the size its config names, and logs a clean
  Vulkan startup with validation on. It is the only one of these that tests what the plan is for, and
  the only one that found anything.
- **cpplint** is clean, including the example's `main.cxx`, which is inside the recursive scan.

## What this plan does not do

- **No `install()`, `export()`, package config or vcpkg port.** That is Alternatives 2 and 3 of
  ADR-0027, and this plan is deliberately their first half rather than a detour around them.
- **No version number.** A submodule sha is the version, and there is no compatibility check.
- **No rewrite of the tree's own includes.** `../../api/engine/Engine.h` is still what an in-tree
  file writes. The two spellings for one header are named in ADR-0027 as a cost of the choice.
- **No non-MSVC support.** `/utf-8` and `/EHsc` are in the interface properties, and a consumer on
  another compiler is a different plan.
- **It does not create the second repository.** What landed here is the half that has to be true
  before that repository can exist.

## Open questions

Two are settled, two are carried to [TODO.md](../../TODO.md).

- **Whether `find_package(Vulkan)` stays unconditional. It does**, and step 2's note says what that
  costs: every consumer of the api needs the Vulkan SDK, because `api/render` builds with the rest
  of it. Making it conditional means a per-library dependency block. **Carried to TODO.md.**
- **The include prefix.** `<api/image/Image.h>` **is chosen** — it mirrors the tree and moved no
  header. `<v3d/image/Image.h>` would match the namespace and is what an installed package would
  want, which is a rename an install would force anyway.
- **What `find_package(Boost)` at the root does to a consumer's cache.** `Boost_USE_STATIC_LIBS ON`
  is a cache variable and is in force for the consumer's own boost lookup. The example does not
  detect this, because it never looks boost up itself. **Carried to TODO.md.**
- **Whether the example is enough. It is, for now**, having been made an app rather than a link
  check. It cannot drift, because it lives here; it also cannot catch what only a real app needs, and
  that is the thing to revisit when the second repository exists.
