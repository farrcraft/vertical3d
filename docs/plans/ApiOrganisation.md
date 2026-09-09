# API Organisation — One Include Root, And The Six Directories That Outgrew Themselves

Drafted 2026-09-08 from a survey of `api/`, and **open**. Eleven steps: three that change how a
header is named, seven that move files, and one that writes down what moved. No behaviour
changes anywhere in it.

`api/` is sixteen libraries and ~45,400 lines. Most of them are the right size and want nothing
done to them — `api/dag` is eighteen files and 493 lines total, and splitting it would buy a
directory listing and cost a reader two levels. The trigger for this plan is not file count. It
is **files in one directory that do not talk to each other**, and there are six directories
where that has happened:

| Directory | Files | What is in there |
|---|---|---|
| [`api/render/realtime/vulkan`](../../api/render/realtime/vulkan/) | 46 | five concerns, flat and alphabetical |
| [`api/ui`](../../api/ui/) | 36 | five concerns, above three subdirectories that already exist |
| [`api/render/offline`](../../api/render/offline/) | 32 | **two systems** — a RIB reader and a whole language |
| [`api/event`](../../api/event/) | 28 | six of core, twenty of one repeated kind |
| [`api/asset`](../../api/asset/) | 27 | a caching framework and the things it caches |
| [`api/type`](../../api/type/) | 19 | cameras, geometry, and two files that are neither |

What makes all seven of those moves expensive today is one thing, and it is the first three
steps of this plan rather than a caveat on the rest.

## Context

### Nothing in this tree uses the include root it already has

[`v3d_add_api_library`](../../cmake/v3dHelpers.cmake) puts the repository root on every api
target's PUBLIC include path, and says why in its own comment: *"an include root, so a consumer
writes `#include <api/image/Image.h>`."* One consumer does. It is
[`examples/starter`](../../examples/starter/), which is outside the tree and reaches in:

```
examples/starter/src/AppEngine.h:9:  #include <api/render/realtime/Canvas.h>
```

Inside the tree there are **763 relative includes across 337 files**, and not one `<api/...>`:

| Where | Relative includes |
|---|---|
| `api/` library sources | 206 |
| `api/*/tests` | 256 |
| The nine apps | 301 |

```
api/ui/component/menu/MenuBar.cpp:  #include "../../../render/realtime/Handle.h"
vertical3d/src/Controller.cxx:      #include "../../../api/brep/HalfEdge.h"
```

Three consequences, and the third is why this plan exists at all.

**A move rewrites files that were not moved.** Every `../` count is a fact about where *both*
files sit. Moving `api/ui/Cursor.h` into `api/ui/input/` changes the include in every file that
names it, including files in other libraries and in every app.

**The repeated basenames are unreadable.** There are three `Font2D.h`, three `TextureFont.h`,
four `Type.h` and six `Engine.h` in `api/`, and one file includes all three of one of them:

```
api/asset/loader/TextureFont.cpp:6:   #include "TextureFont.h"
api/asset/loader/TextureFont.cpp:11:  #include "../TextureFont.h"
api/asset/loader/TextureFont.cpp:12:  #include "../../font/TextureFont.h"
```

Three different types with one name, told apart by a `../` count.
[`Font2D.cpp`](../../api/asset/loader/Font2D.cpp) beside it does the same.

**The convention the tree is documented as having is the one it does not use.** ADR-0027
installs nothing and consumes as source; the include root is how. A new consumer copies
`examples/starter`, writes `<api/...>`, and then reads a library whose own files do it the other
way.

**And this was weighed once already, and declined.**
[ADR-0027](../adr/0027-the-api-is-consumed-as-source.md)'s fourth Negative bullet is exactly this
question: *"The same header has two spellings… Rewriting every in-tree include to the second form
is a whole-file diff across the tree for no build gain, so the asymmetry is kept and a reader has
to learn it."*

That was right on its own terms, and it is why step 1 is an ADR rather than a convention note:
this plan reverses a recorded trade-off, and the reversal turns on a fact that did not exist in
September. The gain is not a build gain — there is still none — it is that the asymmetry prices
every one of steps 4 to 10 at the whole-file diff ADR-0027 was avoiding, six times over, in files
that have nothing to do with the directory being split.

The comment in [`v3dHelpers.cmake`](../../cmake/v3dHelpers.cmake) has the same shape: it
justifies rooting at the repository rather than at `api/` *"because api headers reach each other
by relative path and no other prefix leaves those resolving unchanged."* The decision survives —
rooting at the repository is still right, because it is what makes `<api/...>` say which
repository — but the sentence has to change with the code.

### `api/render/offline` is two systems under one namespace

Thirty-two files in `v3d::render::offline`, and the prefixes are doing the work a directory
should:

- **`RIB*` — six files.** A lexer, a declarations table, a parameter list and a reader that
  dispatches [ADR-0025](../adr/0025-the-rib-reader-dispatches-a-cpp-request-interface.md)'s
  request interface.
- **`SL*` — twenty-four files.** A complete language per
  [ADR-0026](../adr/0026-shading-is-a-language-over-a-batch.md): lexer, syntax, parser, type
  checker, built-ins, compiler and emitter, plus a bytecode runtime of value, program and
  machine. [`SLCompiler.cxx`](../../api/render/offline/SLCompiler.cxx) is 989 lines and
  [`SLParser.cxx`](../../api/render/offline/SLParser.cxx) is 664.

The two halves share `FrameBuffer` and nothing else. And the language's own two halves are a
sharper boundary than the one between the language and RIB: the compiler runs once when a shader
is named, the machine runs once per shading batch, and nothing in the runtime should be able to
reach the parser.

### `api/ui` has three subdirectories and 36 files above them

`component/`, `style/` and `style/property/` exist. Everything else is flat, and it is five
different jobs:

- the shape of the library — `Engine`, `Loader`, `Component`, `Container`, `Layout`, `Arranger`
- paint — `ComponentRenderer`, `Painter`, `TextRenderer`, `Text`, `Dressing`
- input — `Cursor` ([ADR-0038](../adr/0038-a-cursor-is-routed-by-the-library-that-drew-it.md)),
  `Keys` ([ADR-0040](../adr/0040-a-key-goes-to-a-focused-component.md)), `Command`
- the immediate layer — `Immediate`, alone, at 909 lines of `.cpp` and 554 of `.h`
  ([ADR-0035](../adr/0035-an-immediate-mode-layer-over-the-same-canvas.md))
- the shell pieces — `GameMenu`, `StatisticsOverlay`, which are
  [ADR-0028](../adr/0028-an-apps-shell-belongs-to-the-api.md) rather than components

And one file is in the wrong place outright. [`api/ui/Style.h`](../../api/ui/Style.h) declares
`v3d::ui::Style`, whose `property()` returns a `boost::shared_ptr<style::Property>` and which
`style::Theme` holds vectors of. It is the root of the `ui/style/` subtree, sitting outside it,
in the parent namespace. [UiConsolidation](completed/UiConsolidation.md) step 5 gave the three
`Style`s distinct names; it did not move this one, because nothing then depended on where it sat.

### `api/event` is six files and twenty instances of one thing

Core is `Engine`, `Context`, `Event`, `Mapper`, `State`, `Type`. The other ten pairs — `Key`,
`KeyDown`, `KeyUp`, `TextInput`, `MouseButton`, `MouseMotion`, `WindowFocus`, `WindowResize`,
`Sound` — are concrete events of ~50 lines each that the core never names individually. The
library is 1,332 lines across 32 files, which is the signature of a directory holding a list.

### `api/asset` mixes the framework with the payloads

`Asset`, `Cache`, `Manager`, `Loader`, `Writer` and `Type` are the framework of
[ADR-0030](../adr/0030-a-model-is-an-interleaved-array-that-names-its-texture.md)'s asset path.
`Font2D`, `Image`, `Json`, `JsonFile`, `Model`, `Sound`, `Text` and `TextureFont` are the things
it holds. They sit interleaved, alphabetically, and the result is that
[`api/asset/Loader.h`](../../api/asset/Loader.h) sits directly above `api/asset/loader/` — which
reads as a mistake rather than as a design, and is the first thing anyone asks about.

### `api/type` holds two files that are not types, and one is dead

The nineteen files are four cameras (`Camera`, `CameraProfile`, `IsometricCamera`, `ArcBall`),
three geometry types (`AABBox`, `Bound2D`, `Ray`), `Model` — the
[ADR-0024](../adr/0024-api-type-serves-both-renderers.md) seam both renderers read — and two
others:

- **[`String.{h,cxx}`](../../api/type/String.h)** is one function, `string_to_vec2`. It has **no
  consumers**: not another library, not an app, not a test. It is dead code.
- **[`3dtypes.h`](../../api/type/3dtypes.h)** declares `floor_log2` and `npot` at **global
  namespace** and defines `RANDOM_FLOAT` as a macro calling `rand_r`, which MSVC does not have,
  so the macro would not compile if anything used it. Nothing does. `floor_log2` is called only
  by its own test. `npot` has exactly one caller,
  [`api/font/Font2D.cxx:147`](../../api/font/Font2D.cxx#L147). Two moya files include the header
  and use none of it.

It is the oldest surviving file in `api/` and the only one that puts a name in the global
namespace, which is a rule the rest of the tree keeps without being told.

## Decisions

One record, and it is step 1. The seven moves after it are not ADR material: a directory split is
reversible with a `git mv`, constrains nothing later, and the reasoning belongs beside the
convention in [Conventions.md](../Conventions.md) rather than in a decision record.

| ADR | Decision |
|---|---|
| [0048](../adr/0048-an-api-header-is-named-from-the-repository-root.md) (step 1) | An api header is named by its path from the repository root, inside the tree as well as outside |
| [0027](../adr/0027-the-api-is-consumed-as-source.md) | The api is taken as source through a nesting root — it created the include root step 1 starts using, and declined to use it |
| [0033](../adr/0033-a-consumer-selects-the-api-libraries-it-wants.md) | A consumer names the libraries it wants — the library boundaries this plan does **not** move |

Note what step 1 does *not* do to ADR-0027. It does not supersede it: that record's decision — the
api is consumed as source through a nesting root — is untouched, and the include root it created
is the thing step 1 finally uses. What it reverses is one *consequence* that record accepted, its
fourth Negative bullet, on a cost that has since changed. ADR-0027 stays accepted and gains a note
pointing at 0048; a change of consequence does not restate a decision.

## What blocks what

Three groups, and the first is a hard barrier rather than a preference.

**Steps 1 to 3 are the include convention, and everything else waits behind them.** Not because
the moves are impossible first, but because doing them first means each move rewrites `../`
counts in files it did not touch, in libraries and apps that have nothing to do with it — and
doing that seven times is seven times the diff, seven times the review, and seven chances to
renumber a `../` wrong in a file nobody thought was involved. After step 3, a move is a `git mv`,
a `CMakeLists.txt` edit, and one namespace line.

Steps 2 and 3 are independent of each other and each must land as **a single commit**. They touch
337 files between them; a half-converted tree conflicts with everything.

**Steps 4 to 10 are the moves, and they are independent.** Six directories, no ordering between
them, except that step 5 comes before step 6 — `ui::Style` moving into `ui/style/` is a namespace
change, and doing it after the regroup means moving the same file twice.

**Step 4 has a schedule constraint the others do not.**
[OfflineRenderingPhase3](OfflineRenderingPhase3.md) is open and is rewriting most of the `SL*`
files this step renames. Step 4 does not start until that phase closes. It is listed first among
the moves because it is the strongest case, not because it is the first one to do — if this plan
is started while phase 3 is open, start at step 5.

**Step 11 is the record**, and needs every move that lands to be in it.

```
 1 ADR-0048 ─┬─> 2 api/ converts ─┬─> 4 offline   (after OfflineRenderingPhase3 closes)
             │                    ├─> 5 ui::Style ─> 6 api/ui
             └─> 3 apps convert ──┼─> 7 vulkan
                                  ├─> 8 event
                                  ├─> 9 asset
                                  └─> 10 type

                                     every move that lands ─> 11 docs
```

## Steps

### Step 1 — ADR-0048, an api header is included by its path from the root

**Was.** [adr/](../adr/), per [sdlc.md](../sdlc.md) — the record comes first, and this one is the
only thing in the plan that is hard to reverse. 763 call sites is not a decision to make twice.

What it has to settle:

- **The rule.** A header outside the including file's own directory is `#include <api/…>` from
  the repository root. A header in the same directory stays `"Neighbour.h"`. An app reaching into
  `api/` uses the same angle-bracket form the external consumer does, so there is one spelling of
  an api header in the tree rather than two.
- **Why the alternatives lose.** A per-library include root (`<image/Image.h>`) collides with
  system and vendored headers and says nothing about which repository. A `v3d/` prefix means
  either a mirrored header tree or a rename of `api/`, and ADR-0027 installs nothing that would
  make a mirrored tree worth maintaining. Angle brackets across libraries but relative within one
  is the tempting middle, and it buys nothing here — four of the six splits are intra-library.
  Leaving it alone is ADR-0027's own answer, and what it costs is steps 4 to 10 paying a
  tree-wide diff each.
- **What it costs.** cpplint classifies an angle-bracket include as a system header, so
  `<api/…>` sorts into the group with `<boost/…>` and `<glm/…>` rather than with the quoted
  project headers. Every converted file's include block gets reordered, and
  [Linting.md](../Linting.md) is explicit that there is no `--filter` — so this is a real
  constraint on the conversion, not a warning to suppress.
- **Whether tests follow.** They should, and the plan assumes it: a test that includes its
  subject differently from the way every consumer does is a test written against a private view
  of the library.

**Landed as `proposed` on 2026-09-08.**
[ADR-0048](../adr/0048-an-api-header-is-named-from-the-repository-root.md) is written and in the
index. Steps 2 and 3 do not start until it is accepted, and the note on ADR-0027's fourth Negative
bullet is held until then — a pointer to a proposed record is a claim that may not survive.

Drafting it found that ADR-0027 had already asked and answered this question, which sharpened the
record: five alternatives rather than three, and the chosen one argued against that record's own
reasoning rather than around it. The corrected comment in
[`v3dHelpers.cmake`](../../cmake/v3dHelpers.cmake) goes with step 2, beside the code it describes.

### Step 2 — `api/` converts

**Landed 2026-09-08.** 462 includes in 215 files, none unresolved — every relative include in
`api/` named a real file inside `api/`. Build clean, `ctest` 24 of 24, cpplint clean.

**It was incomplete, and a third commit finished it.** The conversion matched only includes
containing `../`, but a subdirectory is outside the including file's own directory too, so
`#include "component/Bar.h"` in `api/ui/ComponentRenderer.cpp` was equally in scope and was
left alone — 159 of them across 24 files, 134 in `api/`. Nothing failed, because the quoted
form compiles and lints exactly as well; the completeness grep asked whether `../` was gone,
which is what the script did rather than what the record says. It surfaced only when step 5
went to add an api include to `ComponentRenderer.cpp` and found no api block to add it to.
The check in [Conventions.md](../Conventions.md) is now the one that would have caught it.

**Where `<api/…>` goes was not a choice.** cpplint reads an angle-bracket include ending in `.h`
as a *C* system header, so it must precede every C++ system header; leaving the converted lines
where the relative ones sat is 438 `build/include_order` errors. The position that lints is the
one [`examples/starter`](../../examples/starter/) already used and the one every
`<vulkan/vulkan.h>` in the tree already sat in: own header, then `<api/…>`, then the C++ system
block. So the conversion is two passes, a respell and a move, not one.

**It found one defect.** [`api/engine/Feature.h`](../../api/engine/Feature.h) declares
`enum class Feature : uint32_t` and includes no `<cstdint>`. It compiled because every consumer
included something that pulled `<cstdint>` in first — in `FeatureTest.cpp`,
`<boost/test/unit_test.hpp>` — and moving the api include above that took the crutch away. The
header now includes what it uses. It is the only one: `ninja -k 0` over the whole tree found no
other.

206 includes in library sources, 256 in tests, across the sixteen libraries. Concentrated in
`api/ui` (68), `api/asset` (42) and `api/render` (32); `api/type`, `api/event`, `api/dag`,
`api/ecs`, `api/grid` and `api/log` have none outside their tests.

Same-directory includes are left alone. Only the ones with a `../` in them change, and the
mapping is mechanical: resolve the path against the file's directory, then spell the result from
the root.

**One commit.** A tree where half the libraries have converted is a tree where every other branch
conflicts.

**Additive.** No target, no symbol and no behaviour changes. The build is the check that the
resolution was done right, and it is a complete one — a wrong path does not compile.

**Watch the include order.** cpplint's grouping is the only thing in this step that can fail
after the build passes.

### Step 3 — The apps convert

**Landed 2026-09-08.** 301 includes in 122 files. Build clean, `ctest` 24 of 24, cpplint clean
over the whole tree. No relative parent include survives anywhere in the repository.

**They were not all api includes.** 191 point at `api/`; the other 110 are an app reaching its
own other directories — `vertical3d/src/view` including `../scene/WireframeVisitor.h`. Those are
converted too, so `<vertical3d/src/scene/WireframeVisitor.h>` is how an app names its own header
and no `../` survives. The alternative was to convert only the api ones and leave an app file
holding both spellings, which is the problem [ADR-0048](../adr/0048-an-api-header-is-named-from-the-repository-root.md)
exists to remove, in miniature. It also makes the editor's own directories movable, which is what
the open question below was waiting on.

**It is not purely an include rewrite, and this is the exception.**
[`v3d_add_test`](../../cmake/v3dHelpers.cmake) never put the repository root on a test target's
include path. Every api suite had been getting it by accident, from the PUBLIC include directory
of the library it links; `v3dtest_voxel` links only boost and libnoise, so it had no source for
it and all seven of its files failed at once. Relative includes need no include directory at all,
which is what hid it. Fixed in the helper rather than in
[`voxel/tests/CMakeLists.txt`](../../voxel/tests/CMakeLists.txt), so the next suite that links no
api library is not caught by it.

Same mapping and same one-commit rule as step 2, and independent of it — an app converted while
`api/` still uses relative includes builds fine, because the two never resolve through each other.

Worth doing even without the rest of the plan: an app is the thing a person copies when starting
a new one, and what they copied disagreed with [`examples/starter`](../../examples/starter/).

**Additive** in behaviour; one build file changed.

### Step 4 — `api/render/offline` splits into the reader and the language

**Waits for [OfflineRenderingPhase3](OfflineRenderingPhase3.md) to close.**

```
offline/             FrameBuffer
offline/rib/         Lexer Declarations Parameters Reader Handler      v3d::render::offline::rib
offline/sl/          Lexer Syntax Parser Types Builtins Compiler Emitter   …::offline::sl
offline/sl/runtime/  Value Program Machine Renderer                    …::offline::sl::runtime
```

The `RIB` and `SL` filename prefixes come off — they are a hand-rolled namespace, and after the
move a real one says the same thing. `RIBLexer` and `SLLexer` become `rib::Lexer` and `sl::Lexer`,
which is the pair the current names are working around.

The `sl/` to `sl/runtime/` line is the one to hold: the compiler runs when a shader is named and
the machine runs per shading batch, and after this step a runtime file that includes the parser is
visible as a mistake rather than as an alphabetical neighbour.

**Additive.** One library target, one set of sources, no link change.

**Tests.** `api/render/offline/tests` stays flat — it is one target and its files have no
neighbours to be confused with — but the cases rename with their subjects: `RIBLexerTest.cxx`
becomes `RibLexerTest.cxx`, `SLCompilerTest.cxx` becomes `SlCompilerTest.cxx`. The suite passing
unchanged is the check that the move was a move.

### Step 5 — `ui::Style` becomes `ui::style::Style`

**Landed 2026-09-08.** Build clean, `ctest` 24 of 24, cpplint clean.

Three forward declarations moved with it rather than one: `Loader.h` and `ComponentRenderer.h`
each declared `class Style;` in `v3d::ui`, and `Resolver.h` declared it and never included the
header, so removing the declaration left `Style` undeclared in a file that had compiled for
months without ever seeing its definition. That is the shape of what this step was for — three
files reaching across a namespace boundary for a type that belonged on their side of it.

`Style.cpp` also carried `style::property::Number` and `style::property::Color`, which resolve
to nothing once the file is inside `v3d::ui::style`; they are `property::` now. The class comment
described an XML configuration language the tree has not read since the JSON loader landed, and
describes the class instead.

[`Style.{h,cpp}`](../../api/ui/Style.h) moves into [`api/ui/style/`](../../api/ui/style/) and
joins the namespace its own return types are already in. Four files include it —
`ComponentRenderer.cpp`, `Engine.cpp`, `Immediate.cpp`, `Loader.cpp` — plus `Theme`, which holds
`vector<shared_ptr<Style>>` and currently qualifies upward to get it.

Not a pure move: the namespace changes, so every use of the unqualified name inside `ui/style/`
resolves differently afterwards. That is the point — `Theme` stops reaching out of its own
namespace for the type it is a collection of.

While in here, the doc comment on `Style.h` still describes the XML configuration language, which
this tree has not read since the JSON loader landed. It should describe what the class is.

**Additive.**

**Tests.** `ThemeTest.cpp` and `ResolverTest.cpp` cover both sides and should need only the
qualification change.

### Step 6 — `api/ui` regroups

```
ui/            Engine Loader Component Container Layout Arranger
ui/paint/      ComponentRenderer Painter TextRenderer Text Dressing    v3d::ui::paint
ui/input/      Cursor Keys Command                                     v3d::ui::input
ui/immediate/  Immediate                                              v3d::ui::immediate
ui/shell/      GameMenu StatisticsOverlay                             v3d::ui::shell
ui/style/      Style Theme Resolver Property + property/              (step 5)
ui/component/  unchanged
```

Six pairs left at the top, which is the library's public shape: what a ui *is*, what builds one,
and the two types a component is made of.

`ui/component` stays flat at 34 files. They are seventeen instances of one concept and a
`layout/` versus `widget/` split there would be arbitrary — a `Panel` is both.

`ui/shell/` is the one grouping that is a claim rather than a tidy-up:
[ADR-0028](../adr/0028-an-apps-shell-belongs-to-the-api.md) says what every app repeats belongs to
the api, and `GameMenu` and `StatisticsOverlay` are that rather than components. Putting them
under a name says which of the two things in this library they are.

**Additive.**

**Tests.** `api/ui/tests` stays flat; twenty cases, none of which should change beyond their
includes and a namespace qualification.

### Step 7 — `api/render/realtime/vulkan` regroups

46 files, and the alphabet is currently the only organising principle.

```
vulkan/device/    Result Instance Surface Device
vulkan/memory/    Memory Buffer DeviceBuffer Uploader Mesh TextureFactory
vulkan/frame/     Swapchain Presenter RenderTarget DepthBuffer CommandPool Recorder FrameUniforms
vulkan/pipeline/  PipelineBuilder PipelineCache Resources
vulkan/renderer/  QuadRenderer LineRenderer WorldRenderer
```

`renderer/` earns the split on its own. Those three are the primitives of
[ADR-0005](../adr/0005-one-batched-quad-primitive.md),
[ADR-0011](../adr/0011-lines-are-the-second-primitive.md) and
[ADR-0042](../adr/0042-a-textured-quad-in-world-space.md) — the files an app-facing rendering
change reaches — and they are currently filed between `Recorder` and `Resources`.

`frame/` is the largest of the five and the one whose membership is arguable: it is everything
that happens once per frame, which puts `CommandPool` and `Recorder` with the targets they record
into rather than with the `Device` that creates them. The alternative is a `command/` of two, and
two is not a directory.

**This is the step most likely to break the build**, and not for an interesting reason:
[`api/render/CMakeLists.txt`](../../api/render/CMakeLists.txt) lists all 69 realtime paths by
hand, and a missed line is a link error at the end of a long build rather than a compile error at
the start.

**Additive.**

### Step 8 — `api/event`'s event kinds move to `event/kind/`

`Key`, `KeyDown`, `KeyUp`, `TextInput`, `MouseButton`, `MouseMotion`, `WindowFocus`,
`WindowResize` and `Sound` move to `event/kind/`, namespace `v3d::event::kind`. `Engine`,
`Context`, `Event`, `Mapper`, `State` and `Type` stay.

A finer `input/` and `window/` split reads better and strands `Sound` alone, so the single
directory is the one to take.

`Mapper` and the apps' binding configs are what name these types; the rest of the tree sees
`event::Event`. So the blast radius is smaller than the file count suggests — check
[`api/engine/`](../../api/engine/) and each app's binding setup, and little else.

**Additive.**

### Step 9 — `api/asset` separates the framework from what it caches

`Font2D`, `Image`, `Json`, `JsonFile`, `Model`, `Sound`, `Text` and `TextureFont` move to
`asset/kind/`, namespace `v3d::asset::kind`. `Asset`, `Cache`, `Manager`, `Loader`, `Writer` and
`Type` stay.

That leaves `asset/`, `asset/kind/` and `asset/loader/` as three layers that read in order: the
framework, the things it holds, and the readers that produce them. It also stops `Loader.h` from
sitting on top of `loader/`, and it separates the two `Font2D`s and the two `TextureFont`s inside
this library — which is three of each name across the tree today, counting `api/font`.

**Additive.**

### Step 10 — `api/type` keeps types, and two files leave

```
type/           Model
type/camera/    Camera CameraProfile IsometricCamera ArcBall     v3d::type::camera
type/geometry/  AABBox Bound2D Ray                               v3d::type::geometry
```

`Model` stays at the top: it is the ADR-0024 seam both renderers read and ADR-0030's interleaved
array, and it is the one thing in this library that is not a camera or a shape.

**`String.{h,cxx}` is deleted.** `string_to_vec2` has no caller anywhere in the tree and no test.
If a config parser wants it back later it is four lines, and it belongs to whichever library is
parsing rather than to `type`.

**`3dtypes.h` is retired.** `npot` has one caller,
[`api/font/Font2D.cxx`](../../api/font/Font2D.cxx) — it moves there, into `v3d::font`, as a static
free function. `floor_log2` has no caller but its own test; it goes with `npot` if the same file
wants it and is deleted otherwise. `RANDOM_FLOAT` is deleted outright: it is a macro naming
`rand_r`, which does not exist on this toolchain, so it has never been compiled and cannot be.
`3dtypesTest.cxx` follows whatever survives. The two moya includes
([`Polygon.cxx`](../../moya/libmoya/Polygon.cxx),
[`RenderContext.cxx`](../../moya/libmoya/RenderContext.cxx)) use none of its symbols and are
deleted.

That is the last name `api/` puts in the global namespace.

**Additive** except for the deletions, which remove nothing that is called.

### Step 11 — The documents catch up

- **[Conventions.md](../Conventions.md)** took the include rule with step 3 rather than waiting
  for this step: it binds every file in the tree from the moment step 2 landed, and nothing in
  the build or the linter enforces it, so a week of it being undocumented is a week in which a
  new `../` is nobody's fault. What it still needs from this step is the rule for when a
  directory splits — when its files stop sharing a reader, not when it passes a file count, with
  `api/dag` as the worked counter-example.
- **[Architecture.md](../Architecture.md)** describes the shape of `api/`, so the six changed
  libraries change in it.
- **[v3dHelpers.cmake](../../cmake/v3dHelpers.cmake)**'s include-root comment currently states
  the reason step 1 removes. Corrected, not deleted — the root is still the repository, for a
  reason that survives.
- **[RenderingPipeline.md](../RenderingPipeline.md)** for step 7,
  **[UserInterface.md](../UserInterface.md)** for steps 5 and 6,
  **[OfflineRenderers.md](../OfflineRenderers.md)** for step 4. Each of these names files by
  path.
- **[ADR-0027](../adr/0027-the-api-is-consumed-as-source.md)**'s fourth Negative bullet gets a
  note saying [0048](../adr/0048-an-api-header-is-named-from-the-repository-root.md) reversed it.
  Held until 0048 is accepted, and earlier than step 11 if that happens first.
- **[README.md](README.md)** and **[sdlc.md](../sdlc.md)** get this plan's entry, and lose it
  again when it closes.

## Verification

The four gates in [sdlc.md](../sdlc.md) §4, and this plan is unusual in that they are close to
sufficient:

- **Build.** `ninja -C out/build/x64-Debug`. For steps 2, 3 and 4 to 10 this is most of the
  verification there is — a mis-resolved include or a source file dropped from a `CMakeLists.txt`
  is a compile or link error, not a subtle one. It is also the reason each step is a build of the
  whole tree rather than one target.
- **Tests.** `ctest --test-dir out/build/x64-Debug --output-on-failure`. **Every suite should
  pass unchanged at every step.** A test that has to be edited beyond an include line or a
  namespace qualification means the step moved more than files, and is the signal to stop and
  look.
- **Lint.** cpplint over the tree, and `/W4 /WX` through the build. Steps 2 and 3 are the ones to
  watch: converting to angle brackets moves those includes into cpplint's system-header group,
  and [Linting.md](../Linting.md) allows no filter.
- **Run.** Once, at the end, rather than per step: the editor and one game, with the validation
  layer on and silent. Nothing here changes a draw, so this is a check that the tree still links
  and starts rather than a rendering verification.

Two things worth adding that the gates do not cover:

- **A grep is the completeness check for steps 2 and 3.** `#include "../` should return nothing
  under `api/` after step 2 and nothing in the apps after step 3. The build cannot tell a
  converted file from an unconverted one that still resolves.
- **`examples/starter` builds against the moved tree.** It is the only out-of-tree consumer and
  the only thing that exercises the include root from outside — and after steps 4 to 10 its
  `<api/render/realtime/…>` includes name paths that have changed.

## What this plan does not do

**It does not move a library boundary.** Sixteen `v3dlib_*` targets in, sixteen out. Every split
here is inside a library, and [ADR-0033](../adr/0033-a-consumer-selects-the-api-libraries-it-wants.md)'s
manifest is untouched. A directory is not a target.

**It does not split `api/ui/component`.** 34 files, and they are one concept repeated. The five
places that must be edited together to add a widget are recorded in
[UiConsolidation](completed/UiConsolidation.md) step 8 and partly closed by
[ADR-0047](../adr/0047-a-component-type-is-checked-by-the-compiler.md); a directory split does
nothing for that and adds a level to seventeen paths.

**It does not split `api/dag`, `api/brep`, `api/font`, `api/image`, `api/input`, `api/grid`,
`api/ecs`, `api/config`, `api/engine`, `api/log` or `api/audio`.** They are between 4 and 37
files and between 141 and 2,914 lines, and `image/` already has `reader/` and `writer/` where the
split was real. Nothing about them is hard to find.

**It does not break up a large file.** `SLCompiler.cxx` at 989 lines, `ComponentRenderer.cpp` at
915 and `Immediate.cpp` at 909 stay as they are. That is a different question with different
answers per file, and mixing it into a move makes the move unreviewable — the whole value of
steps 4 to 10 is that a reviewer can confirm nothing changed.

**It does not mirror the new directories into the test trees.** Each `api/*/tests` is one target
whose files have no neighbours to be confused with, and a test file's name already says its
subject.

**It does not add a per-library convenience header.** An `api/ui/ui.h` that includes the
subdirectories would hide exactly the dependency information the split exists to expose.

## Open questions

- **Whether `Layout` and `Arranger` stay at `api/ui`'s top level.** They are the layout half of
  [ADR-0039](../adr/0039-layout-never-reads-the-box-it-wrote.md) and could be a `ui/layout/`. The
  plan leaves them up because `Layout` is a value type a caller writes into a component and
  `Arranger` is the walk that reads it — closer to the library's shape than to a concern of it.
  Settle it while doing step 6.
- **Whether `vulkan/frame/` is one directory or two.** Seven files is the largest of the five
  groups, and `CommandPool` plus `Recorder` are arguably a `command/` of their own. Two is not a
  directory, which is why the plan does not split it; a third file would change the answer.
- **What `TextureFactory` belongs to.** It is filed under `memory/` in step 7 because it
  allocates and uploads, but it is the only member of that group an app calls directly. If
  `renderer/` grows a texture concern it may follow it there.
- **Whether the apps' own `src/` directories want the same treatment.**
  [`vertical3d/src`](../../vertical3d/src/) is the editor and is the largest of them. Splitting
  them is out of scope here — this plan is about `api/` — but step 3 converted their internal
  includes along with the api ones, so an editor directory is now as movable as an api one and
  the question can be asked whenever someone wants to.
