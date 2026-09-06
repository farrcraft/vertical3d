# TODO

Loose ends and open work. The modernization plan closed on 2026-09-04 with all six phases
done — [plans/completed/Modernization.md](plans/completed/Modernization.md), kept for the
reasoning behind each phase — and the items it closed around are collected here rather than
left in a finished plan.

## Loose ends

[x] work out all of the size_t / unsigned int type issues - api/brep names an index with one type, `brep::Index`, as of 2026-09-04. It is uint32_t rather than uint64_t: a half edge holds four and a mesh is mostly half edges. Nothing else in the tree mixed the two.
[x] fix all of the build warnings - a clean build reported 72 at MSVC's default /W1 and reports none as of 2026-09-04. Raising to /W3 or /W4 has never been tried and would find more.
[x] factor out all SDL calls from apps and into the api instead - done 2026-09-05 under [ADR-0028](adr/0028-an-apps-shell-belongs-to-the-api.md). `Window::focused()` replaced voxel's `window_->sdl()` reach-through, and odyssey's two direct includes went with the shared `main`. What is left in an app is `SDL_main.h`, which is how a windows subsystem executable is entered.
[x] decide whether api/brep keeps Edge, HalfEdgeBRep and WingedEdgeBRep - decided 2026-09-04. `Edge` and `WingedEdgeBRep` are ported and built, with suites; `HalfEdgeBRep` is deleted, because `BRep` is what it became.

## External api consumption

Carried out of [plans/completed/ExternalApiConsumption.md](plans/completed/ExternalApiConsumption.md),
which closed on 2026-09-05.

[] make the Vulkan dependency conditional - find_package(Vulkan) is unconditional and add_subdirectory("api") builds v3dlib_render whatever else is off, so a consumer wanting only v3dlib_image still needs the Vulkan SDK. A per-library dependency block would fix it and is most of the installed-package work of [ADR-0027](adr/0027-the-api-is-consumed-as-source.md) Alternative 2 arriving early.
[] decide what find_package(Boost) at the root does to a consumer cache - Boost_USE_STATIC_LIBS ON is a cache variable and is in force for the consumer own boost lookup. Either state it in the contract or set it scoped. The example consumer does not detect it, because it never looks boost up itself.

## The clang-tidy backlog

[.clang-tidy](../.clang-tidy) enables bugprone, performance, misc and readability and subtracts
23 checks by name. The tree is clean at the 183 that are left. Seven of the subtractions are
settled rather than pending and are not listed here - the file says why. The rest are this
table: what the tree reports at that check, counted once per distinct site over a full
`-DV3D_CLANG_TIDY=ON` build. Removing a line means fixing what it reports, never widening the
exclusion. `voxel/src/noise` is not counted - it is vendored verbatim and is skipped by
clang-tidy, `/analyze` and cpplint alike.

| Check | Sites | Note |
|---|---|---|
| `readability-convert-member-functions-to-static` | 26 |  |
| `performance-unnecessary-value-param` | 31 | the fix is a const reference, not the by-value-and-move the check suggests |
| `bugprone-derived-method-shadowing-base-method` | 6 | `size()` on a strip and on a component mean different things |
| `readability-implicit-bool-conversion` | 69 |  |
| `bugprone-narrowing-conversions` | 111 |  |
| `readability-braces-around-statements` | 111 |  |
| `readability-math-missing-parentheses` | 131 |  |
| `bugprone-easily-swappable-parameters` | 233 |  |
| `performance-enum-size` | 303 |  |
| `misc-use-internal-linkage` | 526 |  |
| `misc-const-correctness` | 939 |  |
| `misc-non-private-member-variables-in-classes` | 1303 |  |
| `readability-magic-numbers` | 1883 |  |
| `readability-identifier-length` | 2483 |  |
| `misc-include-cleaner` | 3346 |  |
| `readability-uppercase-literal-suffix` | 4156 |  |

## Ongoing workstreams

**Tests.** Every library needing neither a window nor a GPU is covered as of 2026-09-04. What
is left needs one: everything below the recorder in `api/render`, `Feature::Window`, and
`audio::Engine::initialize()` — all of it waiting on
[ADR-0007](adr/0007-ci-rendering-tests.md).

**Documentation.** The rationale for the Vulkan move and for the SDL3 upgrade is recorded
nowhere — [ADR-0001](adr/0001-vulkan-replaces-opengl.md) records the decision, not the
reasoning behind it. [ECSDesign.md](ECSDesign.md) is still a set of open questions, and the
one about what a renderable component looks like is the live one.

## Editor

Open if the app is what gets pushed rather than the platform.

[] 55 of the menu's 75 commands have no handler and log themselves
[] there is no modelling operation, so a component mode selects a face and then moves the whole object
[] one thing is selected at a time - no rubber band and no shift-click
[] there is no file chooser, no "save as" and no dirty flag
[] the viewport panes are not draggable
[] input capture for input-type menu items is unbuilt, so the five in `pong/data/vgui.json` are unreachable

## Done

[x] Replace all of the old XML config stuff with JSON equivalents - the config layer is JSON throughout, and pong, tetris, voxel and odyssey are all on the indirect `{"configs": [...]}` form
[x] update pong / tetris / voxel to use json instead of xml - 2026-08-31, odyssey 2026-09-01
[x] update the ui loader to use json instead of xml - `ui::Engine::load` reads JSON, and per ADR-0020 a theme is JSON too
[x] Get tests working again
[x] integrate tests into github actions
[x] rework luxa - move into api / re-namespace / modernize ptr usage, etc - done as `api/ui`, and `luxa/` is deleted; see [LuxaAudit.md](audits/completed/LuxaAudit.md)
[x] replace OpenAL with SoLoud - `v3dlib_audio` is soloud, and no OpenAL call is left in the tree
[x] decide what a Tool is in api/event - settled by [ADR-0017](adr/0017-a-command-is-a-name-in-a-context.md): `Tool` stays in the editor, because no game holds a gesture open across events and one consumer is not a library
