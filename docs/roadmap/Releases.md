# Releases

What the tree needs to ship versioned releases: the editor, moya, imagetool and the demos as one
product, as an installer on Windows and a Flatpak on Linux. The api libraries stay consumed as
source, and a release's tag gives a consumer a named commit to pin. Both choices are recorded in
[ADR-0165](../adr/0165-releases-the-apps-ship-as-one-versioned-product.md) and
[ADR-0166](../adr/0166-releases-a-windows-installer-and-a-flatpak-on-linux.md).

Today nothing is installed, versioned or packaged. Every app runs from its build directory, and
CI builds Debug on Windows only.

## What exists

- **Data beside the executable.** The build copies `data/` and each app's own `data/` into a
  `data/` directory beside the executable.
  [`engine::appPath`](../../api/engine/Application.h) finds it from `argv[0]`, which resolves
  against the working directory when the app is started from `PATH`.
- **Shaders inside the executable.** glslc compiles them at build time, and the SPIR-V is built
  into each binary. moya's standard shaders are built in too.
- **Settings in the user's directory.** `engine::userPath` calls `SDL_GetPrefPath`, so settings
  already live where an installed app can write them.
- **Files written beside the executable.**
  - The editor's default project is `<exe>/project`, and RIB export writes `export.rib` and
    `export.png` there.
  - Every app writes `v3d.log` there, and falls back to standard error when it cannot.
- **moya beside the editor.** The editor runs `moya` from its own directory, and the build copies
  moya there.
- **The editor's USD plugins** are read from `<exe>/usd/`.
- **FMOD is optional and off by default.** The default build has no proprietary code, and audio
  goes through SDL3_mixer.
- **Licences.**
  - The tree is MIT.
  - Every vcpkg package is permissive. OpenUSD, FreeType and ICU need attribution.
  - The fonts are under the SIL Open Font License, each with its licence beside it.
- **Version numbers.**
  - `moya --version` prints 0.0.1, and voxel draws 0.0.1 on screen. No other app has a version.
  - The Vulkan application version is 1.0.0 for every app.
  - There is no about box.
- **Linux.**
  - The libraries that need neither Vulkan nor SDL build with Clang in the portable job.
  - No app, and no library that needs SDL, Vulkan or USD, has been built off Windows.
  - The app code already has a branch for each platform where one is needed, such as how the
    editor starts moya.
- **CI** builds Debug only, and uploads nothing.

## What it needs

### 1. Apps that run from an install

An installed app's directory is read-only, and it may be started from anywhere.

- Each app finds its data through `SDL_GetBasePath`, not `argv[0]`.
- The installed tree has one shared `data/` for every app, rather than a copy per app. The
  fonts alone are 24 MB.
- The editor's default project and its RIB export go to the user's documents, not beside the
  executable.
- `v3d.log` goes to the user's directory.
- The editor finds moya and its USD plugins wherever the install puts them.
- A Release build of every app runs in CI.

### 2. What the releases may ship

- **Demo assets.** The source and licence of pong's sounds and tetris's piece images are not
  recorded. Each is either recorded or replaced. `pieces.psd` stays out of the install.
- **Third-party notices.** A script generates one notices file from the copyright files vcpkg
  installs, the font licences and the vendored licences. It ships with every release.

### 3. A version in every app

Needs a decision on the version scheme. The research favoured SemVer from 0.1.0, because the api
has consumers and a 0.x version states that it makes no compatibility promise yet.

- The top-level `project()` holds the version, and a generated header carries it into the code.
- Every app answers `--version`. The editor shows the version in an about box.
- The Vulkan application version is the release's, and each app names itself to Vulkan.
- A build that is not a release adds `git describe` to its version, so a bug report names the
  commit.

### 4. Install rules

Needs steps 1 and 2.

- `install()` rules place each app, the shared data, moya, the USD plugins and the notices in one
  install tree.
- Needs a decision on how a release links its dependencies. The research favoured the
  `x64-windows-static-md` triplet, which leaves no DLLs to gather. OpenUSD's plugins may need it
  built as shared libraries, so the triplet is tried with the editor before it is chosen.
- A CI job installs the Release build and runs each app from the install tree with `--capture`.
  It fails when an app cannot find a file it needs.

### 5. A changelog and a release workflow

Needs steps 3 and 4.

- Needs a decision on the changelog. The research favoured a hand-written `CHANGELOG.md` in the
  Keep a Changelog form, updated in the same change as the code. Its section for a release
  becomes the release notes. The prose gate's rule against history in documents does not apply
  to it, as it does not apply to plans.
- Needs a decision on the release flow. The research favoured a pushed `vX.Y.Z` tag on `main`,
  with an optional `-rc.N` first, that starts a release workflow. A release branch is cut only
  when a fix has to ship after `main` has moved on.
- The workflow builds Release, installs, packages, attests each artifact and publishes a GitHub
  Release. Releases are immutable.
- A release process document under `docs/contributing/` states the rules.
- README.md, CONTRIBUTING.md and UsingTheApi.md stop saying that nothing is released.
  UsingTheApi.md says how to pin a release.

### 6. The Windows installer

Needs step 5.

- Needs a decision on the installer tool, among NSIS, WiX, Inno Setup and MSIX.
- Each app gets an icon and version resources. The installer adds Start menu entries and an
  uninstaller. Whether it registers the editor's documents or `.rib` files with their apps is
  part of the same decision.
- Needs a decision on signing. The research favoured SignPath Foundation, which signs at no cost
  for projects whose every component has an OSI licence. A build with FMOD would not qualify.

### 7. The apps on Linux

Can start at any time. Needs step 1 before it can run the apps from an install.

- Every app and every library builds on Linux. Whether the build uses Clang alone, or GCC as
  well, is a decision.
- The portable job builds the whole tree and runs the tests on a software Vulkan driver.
- The `.cmd` scripts get shell equivalents, or Build.md gives the commands for Linux.

### 8. The Flatpak

Needs steps 4, 5 and 7.

- A Flatpak manifest builds the install tree on a Freedesktop runtime. Flathub builds with no
  network, so every source vcpkg downloads is listed in the manifest. Whether vcpkg builds inside
  the Flatpak at all, or the runtime's own libraries replace some packages, is a decision.
- Each app gets a desktop entry, an AppStream metainfo file and an icon.
- The release workflow builds the Flatpak, and the manifest is submitted to Flathub.

## What blocks what

- Step 1 and step 2 can start now, in either order.
- Step 3 can start now.
- Step 4 needs steps 1 and 2.
- Step 5 needs steps 3 and 4.
- Step 6 needs step 5.
- Step 7 can start now, beside steps 1 to 6.
- Step 8 needs steps 4, 5 and 7.
