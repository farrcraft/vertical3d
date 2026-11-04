# Dependencies

This document is for contributors. It lists the third-party packages the tree uses, says where
each one comes from, and explains how to add or update one. [Build.md](Build.md) covers what the
build does with them. A project in another repository that uses the api manages its own
packages, as described in [UsingTheApi.md](../api/UsingTheApi.md#2-the-vcpkg-manifest).

- [Packages from vcpkg](#packages-from-vcpkg)
- [The Vulkan SDK](#the-vulkan-sdk)
- [TypeScript tooling from npm](#typescript-tooling-from-npm)
- [Setting up vcpkg](#setting-up-vcpkg)
- [Adding a package](#adding-a-package)
- [Updating versions](#updating-versions)

## Packages from vcpkg

vcpkg installs these in manifest mode, from [vcpkg.json](../../vcpkg.json), during the first
configure. [Build.md](Build.md#traps) says where they go, and why that directory is kept.

SDL3, SDL3_mixer, Vulkan and vulkan-memory-allocator are in the manifest's `platform` feature,
which is a default feature, so a configure installs them unless it is given
`-DVCPKG_MANIFEST_NO_DEFAULT_FEATURES=ON`. Only a build of the libraries that need neither a
window nor a device leaves it out, as the Linux CI job does. A new port that only the window,
the device or the audio needs goes in that feature.

OpenUSD is in the manifest's `usd` feature, also a default one, for the same reason: only
`v3dlib_interchange_usd` uses it, and it is the largest package in the tree. Its first install
builds TBB and OpenUSD from source, which takes a long time.

| Port | Notes |
|---|---|
| boost-filesystem, boost-foreach, boost-headers, boost-json, boost-lexical-cast, boost-optional, boost-program-options, boost-smart-ptr, boost-system, boost-test, boost-unordered | One port per boost library the tree uses. The manifest does not use the `boost` metapackage. See [Adding a boost library](#adding-a-boost-library) |
| cgltf | A single header with no CMake config. [Build.md](Build.md#traps) says how it is found and where its implementation is compiled |
| entt | |
| freetype | |
| glm | |
| harfbuzz | `v3dlib_font` shapes text through it, over the faces FreeType holds open. **It must have its `freetype` feature**, which builds the integration with FreeType. It is the `harfbuzz::harfbuzz` target of the `harfbuzz` config package, linked PRIVATE because no public header includes it |
| icu | `v3dlib_font` finds grapheme clusters, words and line breaks through it. It comes from the tree's [overlay port](#the-icu-overlay-port), not vcpkg's own. It is the `ICU::uc` and `ICU::data` targets of `find_package(ICU COMPONENTS uc data)`, linked PRIVATE because no public header includes it |
| libjpeg-turbo | |
| libpng | |
| nanosvg | `v3dlib_image` rasterises SVG through it, as the `NanoSVG::nanosvg` and `NanoSVG::nanosvgrast` targets of the `NanoSVG` config package, linked PRIVATE because no public header includes it. vcpkg builds it static on Windows |
| quickjs-ng | `v3dlib_script` runs scripts on it, as the `qjs` target of the `qjs` config package, linked PRIVATE because no public header includes it. vcpkg builds it static on Windows |
| sdl3 | The window, input, events and platform paths. There is no SDL2 in the tree. **It must have its `vulkan` feature.** Without it SDL is built without Vulkan support, and `SDL_Vulkan_LoadLibrary` fails at startup with "No dynamic Vulkan support in current SDL video driver (windows)" |
| sdl3-mixer | `v3dlib_audio` plays sound through it. It needs SDL 3.4.0 or newer |
| spdlog | |
| sqlite3 | `v3dlib_database` reaches it, as the `unofficial::sqlite3::sqlite3` target of the `unofficial-sqlite3` config package. It is linked PRIVATE because no public header includes it. Its JSON functions are built in |
| thorvg | `v3dlib_font` reads the SVG glyphs of colour fonts through it. The port installs pkg-config files and no CMake config, so `v3d_find_packages` finds its header and library by name and makes the `thorvg::thorvg` target, linked PRIVATE because no public header includes it. vcpkg builds it as a DLL |
| usd | OpenUSD, without its imaging. `v3dlib_interchange_usd` reaches it through the `pxr` config package, as the `usd`, `usdGeom` and `usdLux` targets, linked PRIVATE because no public header includes it |
| vulkan | |
| vulkan-memory-allocator | Used by `memory::Allocator`, which suballocates unless an app asks for one allocation per resource. It is a single header. Its implementation is compiled once, in `api/render/realtime/vulkan/memory/VmaImpl.cxx`, and it is linked PRIVATE because no public header includes it |

The configure looks only for the packages that the selected api libraries need. A build of this
repository always selects every library, so it needs all of them. Boost is always looked for,
because every api library links `Boost::headers`.

**boost::json has no `error_code` or `system_error` of its own** in the boost version the tree
uses. Include `<boost/system/error_code.hpp>` and `<boost/system/system_error.hpp>` and use the
`boost::system` types. [api/asset/loader/Json.cpp](../../api/asset/loader/Json.cpp) parses with a
`boost::system::error_code` and is the example to copy.

There is no OpenGL, and no package for it.

### The ICU overlay port

ICU's data holds every locale's formats, collations and conversions, and is about 30 MB. The
font library reads only the break rules, the character properties they read, the word
dictionaries and the normalization tables, and every app that draws text ships the data. The
tree therefore builds ICU through its own port, in [ports/icu/](../../ports/icu/), which both
[vcpkg-configuration.json](../../vcpkg-configuration.json) files name in `overlay-ports`.

- The port is vcpkg's own, from the baseline commit, with one change to its portfile. It
  downloads ICU's data sources beside the source archive and builds the data from them, filtered
  by [data-filter.json](../../ports/icu/data-filter.json). The source archive's prebuilt data is
  deleted, because ICU uses a prebuilt archive whatever filter it is given.
- The filter is additive: a category ICU's data build names is left out unless the filter
  includes it. Reading a new kind of ICU data, such as collation, means adding its category.
- When the baseline moves ICU to a new version, copy the new port over `ports/icu/`, apply the
  change again, and update the data archive's SHA512 from the release's `SHASUM512.txt`.

Background: [ADR-0021](../adr/0021-audio-use-sdl3-mixer.md),
[ADR-0091](../adr/0091-platform-sdl3-replaces-sdl2.md),
[ADR-0053](../adr/0053-memory-optional-vma-suballocation.md)

## The Vulkan SDK

The [Vulkan SDK](https://vulkan.lunarg.com/) is installed separately, not through vcpkg. Its
installer sets `VULKAN_SDK`, and the build uses that variable to find it.

**Building this repository needs the SDK.** Two parts of the configure use it:

- `find_package(Vulkan)` runs when the selected libraries include `render`. A build of this
  repository selects every library, so it always runs.
- `glslc`, the SDK's shader compiler, compiles shaders at build time. `v3d_add_shader` looks for
  it on its first call and stops the configure if it is missing. `api/render`, the render device
  tests and voxel call it.

A project in another repository needs the SDK only if the api libraries it selects include
`render`, directly or through `engine` or `ui`. See
[UsingTheApi.md](../api/UsingTheApi.md#selecting-libraries).

The SDK also provides the Khronos validation layer, which the renderer turns on when it is
installed. [Testing.md](Testing.md#verifying-a-rendering-change) explains how to use it.

## FMOD Studio

FMOD Studio backs `api/audio`'s event interface. It is optional, and the tree builds without it:
CI does, and so does any machine without the SDK. Without it, an app that plays events gets the
null backend, which plays nothing.

**The SDK is never committed and never downloaded by the build.** Its licence forbids
redistributing it, and its downloads need an account on fmod.com. To build the FMOD backend:

1. Install the FMOD Studio API for Windows. It installs to
   `C:/Program Files (x86)/FMOD SoundSystem/FMOD Studio API Windows` by default.
2. Configure with `-DV3D_FMOD_ROOT=<that directory>`. The configure stops if the directory holds
   no `api/studio/inc/fmod_studio.hpp`.
3. An executable that plays events needs `fmod.dll` and `fmodstudio.dll` beside it at run time.
   `v3dlib_audio` delay-loads FMOD, so an executable without them still starts, and `events()`
   returns the null backend. The audio test suite copies them. An app that plays events copies
   them itself, from `$<TARGET_FILE:v3d_fmod_core>` and `$<TARGET_FILE:v3d_fmod_studio>`.

Setting `V3D_FMOD_ROOT` back to empty builds the null backend alone again.

With the SDK configured, the audio suite plays an event from the example banks the SDK ships.
CI has no SDK, so that case never runs there.

## TypeScript tooling from npm

The tree's TypeScript runs on Node 24 or later, which erases the types and checks none of them.
That covers the review gates in `scripts/`, the fixture and asset generators, and the editor's
scripts. The type checker comes from npm through [package.json](../../package.json):

```
npm ci              # install exactly what package-lock.json names
npm run typecheck   # check every TypeScript file in the tree
npm test            # the review gates' own tests
```

The rules for `package.json`:

- **It holds development dependencies only.** No code that runs imports a package, so a gate or
  a generator runs with `node <file>.ts` and no install. A package that a gate would import at
  run time is not added.
- **Every version is exact,** and [package-lock.json](../../package-lock.json) is committed.
  [.npmrc](../../.npmrc) saves exact versions and never runs a package's install scripts.
- **Every locked package comes from the npm registry** with an integrity hash.
- **CI installs with `npm ci`** and type-checks in
  [review-gates.yml](../../.github/workflows/review-gates.yml), next to the gates.

`scripts/npm.ts` checks these rules, and that no TypeScript file imports a package at run time.
An `import type` statement is erased before the file runs, so it may name a package.

Two projects divide the files. [tsconfig.json](../../tsconfig.json) at the root checks every
`.ts` file that runs on Node, with Node's declarations. It names the directories it leaves out
rather than the ones it reads, so a new generator is checked without being listed.
[vertical3d/data/scripts/tsconfig.json](../../vertical3d/data/scripts/tsconfig.json) checks the
editor's scripts against `editor.d.ts`, with no Node declarations, because the editor runs them
on QuickJS. Both extend [tsconfig.base.json](../../tsconfig.base.json), which holds the strict
options and allows only syntax that can be erased.

To update a tool, run `npm install --save-dev <package>@<version>` with a version released at
least two weeks earlier. Then run `npm run typecheck` and commit `package.json` with the lockfile.

Background: [ADR-0158](../adr/0158-tooling-typescript-is-type-checked-through-a-package-json.md)

## Setting up vcpkg

`vendor/vcpkg/` is listed in `.gitignore`, so a clone of this repository does not include
vcpkg. Clone and bootstrap it once:

```
git clone https://github.com/Microsoft/vcpkg.git vendor/vcpkg
.\vendor\vcpkg\bootstrap-vcpkg.bat
```

For Visual Studio's CMake integration, also run:

```
.\vendor\vcpkg\vcpkg.exe integrate install
```

You do not need to run `vcpkg install` yourself. The first configure installs the manifest's
packages into the build directory. A `vcpkg install` run from the repository root installs a
second copy into `vcpkg_installed/` at the root, which the build does not use.

The baseline is a commit of microsoft/vcpkg, read from your clone's history, so the clone has
to hold it. A clone older than the baseline fails to find it: `git -C vendor/vcpkg pull`, then
configure again.

CI does not use `vendor/vcpkg` or the runner image's vcpkg. Each job checks out the baseline
commit alone, at depth one, bootstraps it, and installs from that.

## Adding a package

From a developer command prompt:

```
.\vendor\vcpkg\vcpkg.exe add port libpng
```

The package is installed at the next configure. Then:

1. Add the `find_package` call to [cmake/v3dDependencies.cmake](../../cmake/v3dDependencies.cmake),
   inside `v3d_find_packages` if the package needs anything other than a plain
   `find_package(<name> REQUIRED)`.
2. Link it from the api library that uses it, PUBLIC or PRIVATE as
   [Build.md](Build.md#linking-rules) describes.
3. Add it to that library's `PACKAGES` list in
   [cmake/v3dApiLibraries.cmake](../../cmake/v3dApiLibraries.cmake). If the package provides an
   imported target, add the target to `V3D_PACKAGE_TARGETS` as well. The configure fails if the
   manifest and the link lines disagree.
4. Add the port to [examples/starter/vcpkg.json](../../examples/starter/vcpkg.json). That file is
   kept identical to the root one.

### Adding a boost library

The manifest lists one `boost-*` port per boost library, so a boost header with no port in the
manifest does not compile. To include `<boost/signals2.hpp>`, add `boost-signals2` to
[vcpkg.json](../../vcpkg.json) and to [examples/starter/vcpkg.json](../../examples/starter/vcpkg.json).

A compiled boost library needs one more step. Add it to the `COMPONENTS` list of the
`find_package(Boost ...)` call in [cmake/v3dDependencies.cmake](../../cmake/v3dDependencies.cmake).
That creates the `Boost::<component>` target that `target_link_libraries` can name. All the
components are found in that one call.

A header-only boost library needs no component. Every api library links `Boost::headers`, which
carries the include directory for all of boost.

Boost is resolved in config mode, using the `BoostConfig.cmake` that vcpkg installs. FindBoost's
input variables, such as `Boost_USE_STATIC_LIBS`, have no effect. The `x64-windows` triplet
builds boost as shared libraries.

## Updating versions

To move every package to newer versions, update the baseline and configure again:

```
.\vendor\vcpkg\vcpkg.exe x-update-baseline
```

**The baseline is the `builtin-baseline` in [vcpkg.json](../../vcpkg.json).** It names a commit
of the microsoft/vcpkg repository, which vcpkg reads from your clone's history, not from the
ports checked out in `vendor/vcpkg`. A port that the baseline commit does not contain fails with
"the baseline does not contain an entry for port X", even when `vendor/vcpkg/ports/X` exists.
[vcpkg-configuration.json](../../vcpkg-configuration.json) names no default registry, because
one there would replace the builtin baseline.

Copy the new baseline into [examples/starter/vcpkg.json](../../examples/starter/vcpkg.json) as
well.

A new baseline usually reinstalls most packages, including boost. See [Build.md](Build.md#traps)
for the cost.

**Dependabot opens pull requests for these updates**, as
[.github/dependabot.yml](../../.github/dependabot.yml) configures. Each waits until a release is
seven days old.

- the vcpkg baseline, weekly. One pull request moves it in `vcpkg.json` and in
  `examples/starter/vcpkg.json` together;
- the GitHub actions the workflows use, weekly and in one group. It moves each pinned sha and the
  release in its comment together;
- the development container's base image in `.devcontainer/Dockerfile`, monthly.

Each pull request runs CI like any other, so an update that breaks the build is found there.
