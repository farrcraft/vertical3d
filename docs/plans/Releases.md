# Releases — Installed Apps, a Version, a Release Workflow, an Installer and a Flatpak

Drafted 2026-11-05 against `36deb542`. This plan takes up every section of
[the releases roadmap](../roadmap/Releases.md). When it closes, a pushed tag builds the editor,
moya, imagetool and the demos in Release, and publishes them as one versioned GitHub Release, with
an installer for Windows and a Flatpak for Linux. Every app runs from a read-only install, answers
`--version`, and ships with the notices its dependencies require.

[ADR-0165](../adr/project/0165-releases-the-apps-ship-as-one-versioned-product.md) decides that the
apps ship as one product and the api ships as source.
[ADR-0166](../adr/project/0166-releases-a-windows-installer-and-a-flatpak-on-linux.md) decides the
two platforms and their formats.

The plan keeps the roadmap's order, with three changes:

* **Each decision the roadmap names is a step of its own**, before the step that builds it. The
  roadmap's research is where each starts, not what it concludes.
* **Finding moya and the USD plugins comes after the linking decision.** Where OpenUSD's plugins
  live depends on whether a release links OpenUSD statically.
  [Step 10](#step-10--the-editor-finds-moya-and-its-usd-plugins-from-the-install).
* **The linking trial comes before its record**, so the record decides from a build rather than
  from research alone. [Step 8](#step-8--the-release-triplet-tried).

## What the survey found

The tree was read at `36deb542`. Line numbers are from that commit. The roadmap's account of the
tree holds.

**Where an app reads and writes:**

* **`engine::appPath` takes the directory of `argv[0]`** (`api/engine/Application.cpp:23`), and
  the engine reads `appPath + "data/"` (`api/engine/Engine.cpp:223`).
* **`v3d.log` is opened beside the executable** (`api/engine/Application.h:100`).
* **The editor's default project is `<exe>/project`** (`vertical3d/src/Controller.cxx:209`). RIB
  export writes `export.rib` and `export.png` beside the executable
  (`vertical3d/src/app/ProjectFiles.cxx:135`).
* **The editor runs moya from its own directory.** The name is `moya.exe` on Windows
  (`vertical3d/src/Controller.h:158`), and `render/Process.cxx` already starts a process on both
  platforms.
* **OpenUSD's plugins are copied beside each executable that reads USD**, as `usd/plugInfo.json`
  (`v3d_copy_usd_plugins`, `cmake/v3dHelpers.cmake:90`).

**Versions:**

* **The top-level `project()` has no version** (`CMakeLists.txt:8`).
* **moya prints `Moya v0.0.1`** for `--version` (`moya/moya.cxx:87`), and voxel holds `0.0.1` in
  `demos/voxel/src/Version.h:9`. No other app has a version.
* **Every app tells Vulkan 1.0.0** as both its application and engine version
  (`api/render/realtime/vulkan/device/Instance.cxx:69-71`).

**What ships:**

* **The tree has no `install()` rule.**
* **The fonts are 24 MB**, each family with its OFL text beside it in `data/fonts/`.
* **pong's three `.wav` sounds and tetris's seven `.tga` pieces** have no recorded source or
  licence. `pieces.psd` sits beside the pieces.
* **The only licence file the tree ships is `LICENSE`.** Nothing gathers the copyright files vcpkg
  installs.

**CI:**

* **`ctest.yml` builds Debug** through the `ci` preset, and uploads nothing.
* **`portable.yml` builds the libraries that need neither Vulkan nor SDL** with Clang 20, under
  ASan and UBSan, and fuzzes the parsers.

---

## Decisions

| ADR | Decides | Step |
|---|---|---|
| 0175 | The version scheme | [6](#step-6--the-record-the-version-scheme) |
| 0176 | How a release links its dependencies | [9](#step-9--the-record-how-a-release-links-its-dependencies) |
| 0177 | The changelog | [12](#step-12--the-record-the-changelog) |
| 0178 | The release flow | [13](#step-13--the-record-the-release-flow) |
| 0179 | The Windows installer tool, and the files it registers | [15](#step-15--the-record-the-windows-installer) |
| 0180 | How the Windows release is signed | [16](#step-16--the-record-signing) |
| 0181 | The compilers on Linux | [18](#step-18--the-record-the-compilers-on-linux) |
| 0182 | Where the Flatpak's dependencies come from | [20](#step-20--the-record-the-flatpaks-dependencies) |

Each record starts from how established projects do it, and goes in as `proposed`. It is accepted
by the owner before the step that builds it starts.

---

## The steps

| | What | Where | ADR | State |
|---|---|---|---|---|
| [1](#step-1--apps-find-their-data-from-the-install) | Apps find their data from the install | `api/engine`, apps | — | done |
| [2](#step-2--apps-write-to-the-users-directories) | Apps write to the user's directories | `api/engine`, `vertical3d` | — | done |
| [3](#step-3--a-release-build-in-ci) | A Release build of every app in CI | CI | — | done |
| [4](#step-4--the-demo-assets) | The demo assets recorded or replaced | `demos` | — | done |
| [5](#step-5--third-party-notices) | One generated notices file | `scripts` | — | done |
| [6](#step-6--the-record-the-version-scheme) | The record: the version scheme | `docs/adr` | **0175** | done |
| [7](#step-7--a-version-in-every-app) | A version in every app | `api/engine`, `api/render/realtime`, apps | 0175 | done |
| [8](#step-8--the-release-triplet-tried) | The release triplet, tried with the editor | build | — | done |
| [9](#step-9--the-record-how-a-release-links-its-dependencies) | The record: how a release links its dependencies | `docs/adr` | **0176** | done |
| [10](#step-10--the-editor-finds-moya-and-its-usd-plugins-from-the-install) | The editor finds moya and its USD plugins from the install | `vertical3d`, build | 0176 | done |
| [11](#step-11--install-rules) | Install rules, and a CI job that runs each app from the install | build, CI | 0176 | done |
| [12](#step-12--the-record-the-changelog) | The record: the changelog | `docs/adr` | **0177** | done |
| [13](#step-13--the-record-the-release-flow) | The record: the release flow | `docs/adr` | **0178** | done |
| [14](#step-14--the-release-workflow) | The changelog, the release workflow and its document | CI, `docs` | 0177, 0178 | open |
| [15](#step-15--the-record-the-windows-installer) | The record: the Windows installer | `docs/adr` | **0179** | open |
| [16](#step-16--the-record-signing) | The record: signing | `docs/adr` | **0180** | open |
| [17](#step-17--the-windows-installer) | The Windows installer, in the release workflow | apps, CI | 0179, 0180 | open |
| [18](#step-18--the-record-the-compilers-on-linux) | The record: the compilers on Linux | `docs/adr` | **0181** | open |
| [19](#step-19--the-whole-tree-on-linux) | The whole tree builds and its tests run on Linux | all of the tree, CI | 0181 | open |
| [20](#step-20--the-record-the-flatpaks-dependencies) | The record: the Flatpak's dependencies | `docs/adr` | **0182** | open |
| [21](#step-21--the-flatpak) | The Flatpak, in the release workflow and on Flathub | apps, CI | 0182 | open |
| [22](#step-22--the-documents) | The documents state the new rules, and the roadmap closes | `docs` | — | open |

## What blocks what

```
1 Data from the install ──┐
2 User's directories ─────┤
3 Release build in CI ────┤
4 Demo assets ────────────┤
5 Notices ────────────────┤
8 Triplet ──> 9 ADR ──> 10 moya and USD ──┴──> 11 Install rules ──┐
6 ADR ──> 7 Version ───────────────────────────────────────────────┤
12 ADR, 13 ADR ────────────────────────────────────────────────────┴──> 14 Release workflow
14 ──> 15 ADR, 16 ADR ──> 17 Windows installer
18 ADR ──> 19 Linux (after 1 and 2 to run from an install)
11, 14, 19 ──> 20 ADR ──> 21 Flatpak
22 Documents             (last; each step also updates its own section)
```

Steps 1 to 6, 8, 12, 13 and 18 can start now, in any order. Step 11 needs steps 1 to 5 and 10.
Step 14 needs steps 7, 11, 12 and 13. Step 17 needs steps 14 to 16. Step 21 needs steps 11, 14,
19 and 20.

## How every step is verified

* **The build and the gates.** Everything compiles at `/W4 /WX`, and cpplint, `/analyze`,
  clang-tidy and every review gate stay clean.
* **The suites.** Every suite passes. A step that adds cases runs `failsfirst.ts` on the change.
* **The apps.** A step that changes where an app reads or writes runs each app it touches with
  `--capture`, from its build directory and, once step 11 lands, from the install tree.
* **The release.** Steps 14, 17 and 21 are checked by a release candidate. Pushing its tag
  publishes a pre-release, so the owner pushes it.

---

### Step 1 — Apps find their data from the install

**Done.** `appPath()` asks `SDL_GetBasePath()` and takes no argument. `Launch::parse` reads the
program's name from `argv[0]`, and `engine::dataRoots()` gives the engine's asset manager two
roots: `data/<program>/` for the app's own assets, then `data/` for the shared ones. The asset
manager searches its roots in order. `v3d_add_app_data` copies an app's data into
`data/<executable>/`, so the build and an install lay the data out the same way. A test suite's
fixtures stay in `data/`, and an engine with no program name reads `data/` alone. Every app ran
with `--capture` from the scratchpad, after its `data/` was deleted and rebuilt.

The shared `data/` is still copied once for each app. Each app builds into its own directory,
and the copy is shared only in the install, which step 11 builds.

* **`appPath` takes `SDL_GetBasePath()`**, not the directory of `argv[0]`. An app started from
  `PATH` or a shortcut then finds its data.
* **One function names the data directory**, so the install layout is set in one place. The
  build keeps a `data/` beside each executable. The install tree has one `data/` that every app
  reads, with each app's own files in a directory named for the app.
* **The shared `data/` is copied once** into the build's output, not once for each app.

**Tests:** an engine case that the data directory does not depend on the working directory. Each
app runs with `--capture` from a directory other than its own.

### Step 2 — Apps write to the user's directories

**Done.** `engine::logPath()` puts `v3d.log` in the app's user directory, and beside the
executable for an app with no identity. `engine::documentsPath()` gives a directory named for
the organization in the user's documents folder, and the editor's default project is
`project` there. RIB export writes into the project's directory, and makes it when the project
has not been saved. A search of every app's writes found nothing else beside an executable: the
render batch, a workspace export and a capture each write where the user names. The editor and
pong ran with `--capture`, and each wrote its log under `AppData/Roaming/Vertical3D/`.

The survey said RIB export already wrote into the project's directory. It wrote beside the
executable, as the roadmap said, and this step moved it.

* **`v3d.log` goes to `userPath()`**, and still falls back to standard error.
* **The editor's default project goes to the user's documents**, through
  `SDL_GetUserFolder(SDL_FOLDER_DOCUMENTS)`, in a directory named for the editor. RIB export
  follows the project, as it does now.
* **Nothing an app writes is beside its executable.** A search of every app's writes confirms it.

**Tests:** cases that the log and the default project resolve under the user's directories.

### Step 3 — A Release build in CI

**Done.** `x64-Release` builds Release locally on `x64-Debug`'s vcpkg install, and `ci-release`
is the CI job's. `ctest.yml` runs the job as a matrix over `ci` and `ci-release`, each on a runner
of its own. The starter example and the fails-first harness run in the Debug build alone. The
tree compiled cleanly in Release, and two suites failed only there:

- `Interface::resolveImages` summed two passes in one expression, and C++ leaves the order of a
  sum's operands unspecified. The Release build resolved the containers before the themes. The
  passes are now two statements.
- The brep welder turned `-0` into `0` by adding zero, and the optimiser folded `x + 0.0F` to
  `x`. The key now clears the sign bit of `-0` on the integer.

Every suite passes in both builds, and every app ran from a Release build with `--capture`. The
matrix itself first runs on the branch's pull request.

* **A `release` preset** in `CMakePresets.json` builds Release with the release's options.
* **`ctest.yml` builds and tests it** beside Debug.

**Tests:** the suites pass in Release. A failure that only Release shows is a defect, and is
fixed in this step.

### Step 4 — The demo assets

**Done.** The owner made pong's sounds and tetris's pieces for the project, so they are under its
MIT licence. Demos.md has a table of every asset a demo ships and where it came from, which also
covers odyssey's placeholder sprite and diorama's generated models and sound. `pieces.psd` moved
to `demos/tetris/assets/`, outside `data/`, so neither the build nor an install copies it.

* **pong's sounds and tetris's pieces each get a recorded source and licence**, or are replaced
  by assets that have one. A replacement made in the tree names the script or tool that made it.
* **`pieces.psd` stays out of the install.**

**Tests:** none. Step 5's notices list each asset.

### Step 5 — Third-party notices

**Done.** `scripts/notices.ts --out <file>` writes the notices file: the project's `LICENSE`, each
font family's OFL text, and the copyright file of each vcpkg package that ships code. Run on the
Debug install, it holds eight font families and 80 packages. Without `--out` it is a gate, and
`ctest.yml` runs it after the Debug build's configure. It fails on a package that installs code
and has no copyright file, and on a font no licence covers. A package that installs only under
`share/`, such as vcpkg's build helpers, ships no code and needs no notice. Dependencies.md
states the rule, and nine cases test the script.

Two things differ from the plan. Nothing in the tree is vendored but vcpkg, which does not
ship, so there are no vendored licences to gather. The demos' assets are the project's own work,
recorded in Demos.md, so they are covered by `LICENSE` rather than checked by the script. Step
11 puts the file in the install.

* **`scripts/notices.ts` writes one notices file** from the copyright files vcpkg installs, the
  font licences, the vendored licences and the demo assets' licences.
* **A gate fails when a package or an asset has no licence** to put in it.

**Tests:** cases for the script. A package with no copyright file fails the gate.

### Step 6 — The record: the version scheme

**Done.** ADR-0175 is accepted: semantic versions from 0.1.0, with the research of three
engines and SemVer's own rule for 0.x.

ADR-0175 decides what a version number promises and where it starts. The roadmap's research
favoured SemVer from 0.1.0, because a 0.x version makes no compatibility promise.

### Step 7 — A version in every app

**Done.** The root's `project()` holds 0.1.0. A new api library, `v3dlib_version`, gives
`v3d::version::number()` and `text()`. `cmake/v3dVersion.cmake` writes its source at build time,
and rewrites it only when the text changes. A build shows `0.1.0-dev+<git describe>` unless it
is configured with `V3D_RELEASE=ON`. `--version` is a `Launch` option: `run()` prints the app's
name and version and returns without building the engine, and a windowed app on Windows writes
to the console it was started from. moya and imagetool print the same version, and voxel draws
it. The editor has Help > About, which opens its prompt with the version. `run()` names the app
and its version to SDL with `SDL_SetAppMetadata`, and the Vulkan instance reads them back for
its application name and version. Every app answered `--version`, the editor and pong through a
redirect.

The Vulkan application info has no case of its own: the instance reads SDL's metadata, which an
engine case checks `run()` sets.

* **The top-level `project()` holds the version**, and a generated header carries it into the
  code.
* **Every app answers `--version`**, from the shared command line. The editor shows the version
  in an about box.
* **Vulkan is told the release's version**, and each app's own name.
* **A build that is not a release adds `git describe`** to its version, so a bug report names the
  commit.
* **voxel's `Version.h` and moya's literal are deleted.**

**Tests:** cases that `--version` prints the generated version, and that the Vulkan application
info carries it.

### Step 8 — The release triplet, tried

**Done.** Every app built in Release with `x64-windows-static-md`. The editor and moya ran and
rendered, and the USD suite passed. The triplet does not leave no DLLs: vcpkg's `usd` port
declares `vcpkg_check_linkage(ONLY_DYNAMIC_LIBRARY)`, so OpenUSD's 18 DLLs and its plugins sit
beside the editor whatever the triplet, with the Vulkan loader. No overlay was needed. Each app
carries its own copy of every library, so the apps and libraries came to 104 MB, against 47 MB
for one shared copy of each of the 41 DLLs `x64-windows` installs. Each demo is 12 MB static and
1 MB shared.

* **The editor builds and runs with `x64-windows-static-md`**, the triplet the roadmap's research
  favoured, and imports USD. The triplet leaves no DLLs to gather.
* **A triplet overlay builds OpenUSD shared**, if its plugins need it.

**Tests:** the suites pass with the trial triplet. The editor imports a USD fixture.

### Step 9 — The record: how a release links its dependencies

**Done.** ADR-0176 is accepted: a Windows release links its libraries as DLLs, through
`x64-windows` as every build does. The install holds one copy of each DLL beside the apps, and
leaves out the Vulkan loader. Step 11 gathers them.

ADR-0176 decides the triplet a release builds with, from step 8's trial.

### Step 10 — The editor finds moya and its USD plugins from the install

**Done, with no code.** The Windows install is one flat directory, so it lays the editor out as
the build does. The editor names `moya.exe` or `moya` by platform already, and runs it from
`appPath()`, which step 1 made `SDL_GetBasePath()`. OpenUSD reads `usd/plugInfo.json` beside its
own DLLs, and step 11 installs `usd/` there. Nothing needed a path the editor registers. Neither
a render batch nor a USD import can run without the file chooser, so neither was run from the
install. The layout they rely on is the build's, which the USD suite and the editor use. The
Linux layout is step 19's and step 21's.

* **The editor finds moya** from the install's layout, with the platform's executable name.
* **OpenUSD finds its plugins** wherever the install puts them, through the path the editor
  registers, not only beside the executable.

**Tests:** an editor case that the moya path follows the install layout. The editor renders and
imports from the install tree once step 11 lands.

### Step 11 — Install rules

**Done.** `cmake/v3dInstall.cmake` holds the rules, for Windows only. Each app calls
`v3d_install_app`, and `v3d_add_app_data` installs the app's data into `data/<executable>/`.
The root installs the shared `data/`, the MSVC runtime, and `NOTICES.txt`, which `notices.ts`
writes during the install. The DLLs are found from the installed apps by
`cmake/v3dInstallDependencies.cmake`. A `RUNTIME_DEPENDENCY_SET` refused the build's DLLs,
because vcpkg copies each one beside every app and the copies conflict. The system's DLLs and
the Vulkan loader are left out. The Release install is 75 MB with the fonts: eight apps, 46
DLLs, `data/`, `usd/` and the notices. Every app ran from it with `--capture`, from another
working directory. `ctest.yml`'s Release job installs and runs every app the same way, and runs
first on the branch's pull request.

* **`install()` rules place each app, the shared data, moya, the USD plugins and the notices** in
  one install tree.
* **A CI job installs the Release build** and runs each app from the install tree with
  `--capture`. It fails when an app cannot find a file it needs.

**Tests:** the CI job.

### Step 12 — The record: the changelog

**Done.** ADR-0177 is accepted: a hand-written `CHANGELOG.md` in the Keep a Changelog form.

ADR-0177 decides how a release's changes are recorded. The roadmap's research favoured a
hand-written `CHANGELOG.md` in the Keep a Changelog form, updated in the same change as the code.

### Step 13 — The record: the release flow

**Done.** ADR-0178 is accepted: a tag on `main` starts a release, and a release branch is cut
only when a fix needs one.

ADR-0178 decides what starts a release and where a fix to a release goes. The roadmap's research
favoured a pushed `vX.Y.Z` tag on `main`, with an optional `-rc.N` first, and a release branch
only when a fix has to ship after `main` has moved on.

### Step 14 — The release workflow

* **`CHANGELOG.md`** in the form step 12 decides, with a gate that a change touching an app or a
  library updates it.
* **A release workflow** builds Release, installs, packages, attests each artifact and publishes
  a GitHub Release, with the changelog's section as its notes. Releases are immutable.
* **`docs/contributing/Releasing.md`** states the rules: the version, the changelog, the tag and
  what a release holds.
* **README.md, CONTRIBUTING.md and UsingTheApi.md** stop saying that nothing is released.
  UsingTheApi.md says how to pin a release.

**Tests:** a release candidate. The owner pushes its tag, and the pre-release holds the install
tree as a zip, its attestation and its notes.

### Step 15 — The record: the Windows installer

ADR-0179 decides the installer tool, among NSIS, WiX, Inno Setup and MSIX, and whether it
registers the editor's documents and `.rib` files with their apps.

### Step 16 — The record: signing

ADR-0180 decides how the Windows release is signed. The roadmap's research favoured SignPath
Foundation, which signs at no cost for a project whose every component has an OSI licence. The
application to it is the owner's.

### Step 17 — The Windows installer

* **Each app gets version resources** beside the icon it already embeds.
* **The installer** adds Start menu entries and an uninstaller, and registers the files step 15
  decides.
* **The release workflow builds and signs it.**

**Tests:** a release candidate. The installer installs, every app starts from the Start menu,
and the uninstaller removes everything it installed.

### Step 18 — The record: the compilers on Linux

ADR-0181 decides whether the Linux build uses Clang alone, or GCC as well.

### Step 19 — The whole tree on Linux

* **Every app and every library builds on Linux.**
* **The portable job builds the whole tree** and runs every suite, the device suites on a
  software Vulkan driver.
* **The `.cmd` scripts get shell equivalents**, or Build.md gives the commands for Linux.

**Tests:** the portable job.

### Step 20 — The record: the Flatpak's dependencies

ADR-0182 decides whether vcpkg builds inside the Flatpak, or the runtime's own libraries replace
some packages. Flathub builds with no network, so every source a build downloads is listed in the
manifest.

### Step 21 — The Flatpak

* **A Flatpak manifest** builds the install tree on a Freedesktop runtime.
* **Each app gets a desktop entry and an AppStream metainfo file**, and installs its SVG icon as
  its scalable icon.
* **The release workflow builds the Flatpak.**
* **The owner submits the manifest to Flathub.**

**Tests:** a release candidate. The Flatpak installs, and every app starts and renders on the
host's Vulkan driver.

### Step 22 — The documents

* **Build.md, Testing.md and Dependencies.md** state what the steps changed. Each step updates
  its own section as it lands.
* **ADR-0165 and ADR-0166** name `Releasing.md` in their `Documented in` lines.
* **The roadmap moves to `completed/`.**
