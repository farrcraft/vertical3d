# Header Boundaries — Internal Headers Kept Internal, and moya's RIB Handler Split by Job

Drafted 2026-11-22 against `78af1431`. When this plan closes, no header outside an `internal/`
directory includes one under it, and a gate reports any that does. moya's RIB handler,
`ri::SceneClient`, passes each request to the part of moya's reader that does that job, and its
header names nothing of moya's but the scene.

The plan follows one record:

* [ADR-0260](../adr/project/0260-code-a-public-header-never-includes-an-internal-one.md): a
  public header never includes an internal one. A class keeps internal state behind a pointer to
  a private implementation, and a type a public header holds by value moves out of `internal/`.
  It is accepted.

---

## What the survey found

**Nine headers outside `internal/` include headers under it.** Each internal header can be
included from its own directory, so `scripts/filelocal.ts` passes them. Their users then compile
against the internal headers, from outside the directory:

| Header | Internal headers it includes | Reaches |
|---|---|---|
| `moya/libmoya/ri/SceneClient.h` | `Deforming`, `Lights`, `Network`, `Objects`, `Sampled`, `Shape`, `Warnings` | `moya.cxx`, nine of moya's suites, the editor's `SceneTranslatorTest` |
| `moya/libmoya/integrate/Session.h` | `Crop`, `FirstHits`, `Kept`, `LightPaths`, `Records`, `Tallies`, `TrailSteps` | the editor's `render/MoyaDelegate.h`, so every editor file that includes it |
| `moya/libmoya/light/Lights.h` | `Making`, `Profiles`, `Textures` | four files outside `light/` |
| `moya/libmoya/accel/Assembler.h` | `Part` | five files outside `accel/` |
| `moya/libmoya/geometry/Cache.h` | `Piece` | four files outside `geometry/` |
| `moya/libmoya/texture/Cache.h` | `Tile`, `TileKey` | two files outside `texture/` |
| `api/rib/Reader.h` | `Operands` | moya, the editor's tests, rib's fuzzer |
| `api/ui/paint/TextRenderer.h` | `PlacedEffect` | the ui's shell, the editor's `Renderer.h` |
| `api/ui/paint/CurveText.h` | `PlacedEffect` | diorama's `Renderer.h` |

**Nothing has leaked yet.** Every chain ends at a dependency its library already links PUBLIC.
MaterialX stays behind `ri/internal/DocumentReader.h`, which no public header reaches. The risk
is the next PRIVATE dependency an internal header takes: the configure's visibility check skips
`internal/`, so nothing would report it.

**Some of these are already half done.** `texture/Cache.h` forward-declares `internal::Opened`
and holds it by pointer. `CurveText.h` names `PlacedEffect` only as a reference parameter of a
private static helper. A forward declaration is enough for both.

**`ri::SceneClient` does six jobs.** It implements `rib::Handler`, one method for each RI
request, and that width belongs to the interface. Its 962-line source holds the state and the
rules of six separate jobs:

| Job | Lines | State |
|---|---|---|
| Options and the camera: `Format`, `Projection`, the pixel requests, `Option`, `Hider`, backgrounds | 130 | the options and camera handles, the resolution, two search paths |
| Outputs: `Display`, `DisplayChannel`, Cryptomatte | 155 | the outputs and channels by name |
| The transformation: its stack, motion blocks, coordinate systems, the displacement bound | 200 | five members for the motion block, the named spaces |
| Shading: the material an attribute state makes, its shader nodes | 130 | half of the attribute state |
| Geometry and objects | 150 | the objects by handle; the rest is in `internal::Shapes` and `internal::Deforming` |
| Making textures, which writes files and touches no scene object | 25 | the colour configuration |

Lights, shapes, deformation, networks, names, volumes and warnings are already their own
classes in `ri/internal/`. The rest follows that pattern.

**Its source repeats four rules by hand:**

* **A change of shading starts a new material slot.** The same assignment is in `color`,
  `opacity`, `surface`, `pattern`, `bxdf`, `displacement` and `worldBegin`.
* **A light's name** is the attribute state's, or `"light "` and its handle. It is written in
  `lightSource`, `areaLightSource` and `light`.
* **A request outside the world** is reported by `inWorld()` for a primitive, and by a copy of
  the same test in `lightSource` and `light`.
* **A displacement shader node** is made in both branches of `material()`.

Its warnings also go two ways. `projection()` and `makeTexture()` log each time, and every other
request logs once through `internal::Warnings`.

**The other open plan does not change it.** EditorScripting reads `SceneClient`'s handling of
`ObjectBegin` and changes the editor's export, not the handler.

**The design review found more in the same code.** [DesignReview.md](../audits/DesignReview.md)
lists three defects in the outputs this plan moves (MS-D1 to MS-D3). It also finds that the
editor's `render::SceneTranslator` repeats the handler's mapping by hand (EV2), and that
`Session` does too many jobs for a private implementation alone to fix (MR3). Steps 3 and 7 say
what each changes here.

## Decisions

| ADR | Decides | Step |
|---|---|---|
| 0260 | Whether a public header may include an internal one, and what a class does instead | [1](#step-1--the-record-internal-headers) |

---

## The steps

| | What | Where | ADR | State |
|---|---|---|---|---|
| [1](#step-1--the-record-internal-headers) | The record: internal headers | `docs/adr` | **0260** | accepted |
| [2](#step-2--the-transformation) | The transformation | `moya/libmoya/ri` | — | open |
| [3](#step-3--the-outputs) | The outputs | `moya/libmoya/ri` | — | open |
| [4](#step-4--the-options-and-the-camera) | The options and the camera | `moya/libmoya/ri` | — | open |
| [5](#step-5--the-attribute-state-and-its-materials) | The attribute state and its materials | `moya/libmoya/ri` | — | open |
| [6](#step-6--the-handlers-private-implementation) | The handler's private implementation | `moya/libmoya/ri` | 0260 | open |
| [7](#step-7--the-session) | The session | `moya/libmoya/integrate` | 0260 | open |
| [8](#step-8--moyas-other-headers) | moya's other headers | `moya/libmoya/{accel,geometry,light,texture}` | 0260 | open |
| [9](#step-9--the-api-headers) | The api headers | `api/rib`, `api/ui/paint` | 0260 | open |
| [10](#step-10--the-gate-and-the-rule) | The gate and the rule | `scripts`, `docs/contributing` | 0260 | open |
| [11](#step-11--the-documents) | The documents | `docs` | — | open |

## What blocks what

```
2 Transformation ──┐
3 Outputs ─────────┼──> 6 Private implementation ──┐
4 Options ─────────┤                               │
5 Attributes ──────┘                               │
1 ADR ──┬──────────────> 6                         ├──> 10 Gate ──> 11 Documents
        ├──> 7 Session ────────────────────────────┤
        ├──> 8 moya's other headers ───────────────┤
        └──> 9 The api headers ────────────────────┘
```

Steps 2 to 5 depend on nothing. They split the handler by job whatever the record decides, and
each can land alone. Step 6 needs them and the record, because the private implementation holds
the classes they make. Steps 7 to 9 need only the record. The gate in step 10 lands last, since
the tree has to be clean at it. Step 11 checks that the documents read as a whole.

## How every step is verified

* **Behaviour does not change**, apart from step 5's warnings. Each step that moves code from
  `SceneClient` passes moya's suites unchanged: `SceneClientTest`, `SceneWorldTest`,
  `SceneMaterialsTest`, `SceneHitsTest`, the volume and texture suites, and the reference renders.
  The internal classes are tested through `SceneClient`, as the ones already in `ri/internal/`
  are, since a suite cannot include an internal header.
* **The verify preset builds each step.** x64-Debug and the gates run neither clang-tidy nor
  `/analyze`, so each step is clean at `/W4 /WX`, at `/analyze`, at clang-tidy and at every gate.
* **A case added runs fails-first.** A step that adds Boost.Test cases runs `failsfirst.ts` on
  them.
* **A step on a render path is measured.** Steps 7 and 8 run `moya/bench` before and after, and
  record both in the step's state note against `moya/bench/Baseline.md`.

---

### Step 1 — The record: internal headers

**The question.** Whether a header outside `internal/` may include one under it, and, if not,
what a class with internal state holds instead.

**The options:**

* **Internal headers stay internal (Qt, LLVM).** A public header includes none. A class keeps its
  internal state behind a pointer to a private implementation, and forward-declares what it only
  names by pointer or reference. Qt's public headers never include its `_p.h` headers. LLVM keeps
  a library's private headers in `lib/`, which no header in `include/` can reach.
* **Internal means not named (Abseil, Boost).** A public header may include any internal header of
  its own library. Abseil's `flat_hash_map.h` includes `internal/raw_hash_map.h`, and Boost's
  headers include their `detail/`. The visibility check then has to follow includes from header to
  header, and Conventions.md's claim that an internal header's dependencies are PRIVATE is
  reworded.
* **No type a public header needs is internal.** Each such type moves out of `internal/` and joins
  the reference.

**Accepted** as
[ADR-0260](../adr/project/0260-code-a-public-header-never-includes-an-internal-one.md):
internal headers stay internal. A pointer to a private implementation is the means, a forward
declaration where it is enough, and a type held by value in a public header moves out.

### Step 2 — The transformation

* **`ri::internal::Transformations` holds the current transformation's stack, the open motion
  block and the named coordinate systems.** It takes `TransformBegin` and `TransformEnd`, each
  transform request, `MotionBegin` and `MotionEnd`, `CoordinateSystem` and `CoordSysTransform`.
* **It answers the space a name gives**, which `displacementBound()` reads.
* **`WorldBegin` resets it** and names the camera's space, as the handler does now.

The attribute state still saves and restores the current transformation, so `Transformations`
gives the handler the current samples to save and takes them back.

### Step 3 — The outputs

* **`ri::internal::Outputs` holds the outputs and channels by name.** It takes `Display`,
  `DisplayChannel` and `PxrCryptomatte`, and owns `channelOf()` and the reading of a display's
  mode.
* **The three defects the review found in this code are fixed first**, each with a case that
  fails first. A second Cryptomatte of one layer reuses its channel (MS-D1). A redeclared channel
  is replaced in every output that names it (MS-D2). A channel's components come from
  `rib::Declaration` (MS-D3).
* **The class is shaped so the editor's translator can call it later.** EV2 asks for one mapping
  from RI to moya's scene. This plan does not make the translator use it, but leaves nothing in
  `Outputs` that only the handler can supply.

### Step 4 — The options and the camera

* **`ri::internal::Frame` holds the options and camera objects, the resolution and the two search
  paths.** It takes `Format`, `FrameAspectRatio`, `ScreenWindow`, `CropWindow`, `Projection`,
  `Clipping`, `DepthOfField`, `Shutter`, the pixel requests, `Option`, `Hider`, `FrameBegin`, the
  display filter and the imager.
* **It makes the camera's transformation at `WorldBegin`** from the samples step 2's class gives.

`MakeTexture` stays in the handler. It writes a file and sets no scene object, so it belongs to no
job above.

### Step 5 — The attribute state and its materials

* **`SceneClient::Attributes` and `SceneClient::Shader` move to `ri/internal/`**, each in a header
  of its own.
* **`ri::internal::Materials` makes the material of an attribute state**, with its shader nodes.
  It owns the one rule that a change of shading starts a new material slot, and the seven copies
  call it.
* **One function names a light**, and the three requests that name one call it.
* **One function reports a request outside the world**, and primitives, lights and objects call
  it.
* **Every warning goes through `internal::Warnings`.** `projection()` and `makeTexture()` then log
  each problem once, as the other requests do.

**Tests:** a case that a repeated unknown projection is logged once.

### Step 6 — The handler's private implementation

* **`SceneClient` holds one pointer to its state**, declared in `ri/internal/`. The state holds
  the classes steps 2 to 5 made, with the lights, the deformations, the names and the warnings.
* **`SceneClient.h` includes the RIB requests, the log, the scene's handle and the scene.** It
  includes no header under `internal/`.
* **The class comment stays on `SceneClient`.** It states what moya does with each request, and a
  reader of the reference needs that. Each internal class's comment states the rule that
  class keeps.

### Step 7 — The session

* **`integrate::Session` keeps its seven internal members behind a pointer to a private
  implementation.** It reads them to build each launch's views, not for each ray, so the pointer
  is not on the inner loop.
* **The private implementation is not the split.** The review's MR3 splits `Session` into a
  structure builder, a film owner and a picture maker. That split is a plan of its own, and this
  step lands first so the split happens behind the pointer.
* **`render/MoyaDelegate.h` in the editor then reaches no moya internal header.**

**Tests:** `moya/bench` before and after, and `SessionTest` unchanged.

### Step 8 — moya's other headers

Each header gets the least change the record allows:

* **`texture/Cache.h`**: `Tile` and `TileKey` are held in its map and list by value. Either the
  cache's tables move behind a pointer, or the two types move to `texture/`. The tile is the
  texture cache's unit of memory, so a reader of its memory limit names it, which argues for
  moving it.
* **`geometry/Cache.h`** and **`accel/Assembler.h`**: `Piece` and `Part` are held in maps by
  value, behind a pointer to a private implementation.
* **`light/Lights.h`**: `Textures`, `Profiles` and `Made` behind a pointer to a private
  implementation.

**Tests:** `moya/bench` before and after, and moya's suites unchanged.

### Step 9 — The api headers

* **`rib/Reader.h`** holds `internal::Operands` by value, behind a pointer to a private
  implementation. The pointer costs one allocation for each reader, and moya's suites read the
  same files before and after.
* **`ui/paint/CurveText.h`** forward-declares `internal::PlacedEffect` for its helper's parameter.
  Its `std::vector` of them and `TextRenderer.h`'s move behind a pointer to a private
  implementation, or `PlacedEffect` moves to `paint/`. A placed effect is drawn every frame, so
  the step measures a frame of diorama before and after.
* **The configure's visibility check then sees what each of these includes**, since none reaches
  an internal header.

### Step 10 — The gate and the rule

* **`scripts/filelocal.ts` reports a header outside `internal/` that includes one under it**, as
  a new kind beside `internal-include`. Its cases go in `scripts/tests/`.
* **Conventions.md states the rule** in "One class per header". It says what a public header may
  not include, and when a type moves out of `internal/` instead. It names the pointer to a private
  implementation: a `std::unique_ptr`, with a destructor defined in the source file. It ends with
  `Background: ADR-0260`.
* **The comment in `cmake/v3dApiLibraries.cmake`** that an internal header is included only by
  the library's own sources is then true as written.

**Tests:** the gate's cases, and the tree clean at it.

### Step 11 — The documents

* **docs/offline/README.md and Rib.md** name the parts of moya's reader the handler passes
  requests to.
* **The record's status** is set, and this plan moves to `completed/` with an outcome.
