# Documentation refresh

**Status: complete (2026-10-05).** Decided: retired and merged ADRs are deleted, with a row in the index
saying where their content went; kept ADRs get a full rewrite to the new template; docs move into
audience folders; the phases run straight through.

Rewrite the documentation, the ADRs and the code comments so each one reads plainly and serves
one kind of reader. Fix the ADR framework so it admits fewer records, names them clearly and
keeps their status true.

## What the review found

**ADRs (82 records, about 83,000 words).**

- **Titles don't say what was decided.** Every title follows "Topic — Stylised Claim, And A
  Second Claim". The file name keeps only the stylised half, so the file list can't be read at
  a glance.
- **Most records bundle two to four decisions.** When one of them changes, the record goes
  stale in part and gets patched with a banner paragraph.
- **Records outgrow the template.** The Decision section is meant to be 1–3 sentences and
  usually runs 4–10 paragraphs of implementation detail. That detail is what goes stale:
  renamed classes, member counts, run logs.
- **Context sections tell history instead of stating constraints.** They narrate bugs, refer
  to other repositories (cozy, retcon), and cite tracker IDs and "phase N".
- **The chosen option is listed among the alternatives** with "Why not: n/a". Most records
  also include a straw-man "leave it as is" alternative.
- **Amendments are never recorded in the amended record.** All 82 show `accepted`, including
  0010, 0024 and 0030, which are dead, and 0077, which is half dead. Records amended by later
  ones show nothing in their status: 0005, 0007, 0009, 0012, 0017, 0021, 0027, 0031 and 0043.
- **Several records still describe talyn as a working renderer.** talyn was retired by 0078.
  The affected records include 0022, 0023, 0025, 0026, 0076 and 0077.
- **About 14 records aren't decisions.** They are bug fixes (0039, 0055, 0056), single-widget
  behaviours (0045, 0046, 0057), one helper class (0074), one constructor option (0067), or a
  project-scope call (0006).

**Reference docs.**

- **They're organised by subject, not by reader.** Architecture.md, RenderingPipeline.md
  (785 lines) and UserInterface.md each mix "how to use it" with "how it works inside".
- **Editor.md says almost nothing on its own.** Its explanation of the editor sits in links
  to 0013–0018.
- **Some subjects have no home at all:**
  - the smaller api libraries (image, font, asset, event, input, audio, dag)
  - the games
  - imagetool and v3dshell
  - `examples/`, which has no README
  - a glossary
- **The docs contradict each other on:**
  - include style
  - whether a configure needs the Vulkan SDK
  - whether CI renders anything
  - how many config types there are
  - the editor's command count
  - what imagetool can do
- **History and changelog content:** sdlc.md and plans/README.md carry dated changelogs.
  RenderingPipeline.md and UserInterface.md open with "as of" dates and lists of ADRs.

**Code comments.**

- **ADR citations:** 546 across 291 files, 482 of them under `api/`.
  - About 55% are a tag on a sentence that already makes sense.
  - About 25% don't make sense without the ADR ("Not virtual - ADR-0080").
  - About 10% re-argue the decision.
- **The bigger problem is the style:**
  - "which is what" (390 times) and other inverted, stylised phrasing
  - code described as if it had wants ("the time … owes", "a toolkit … wants"), roughly
    150–250 cases
  - about 180 sentences longer than 45 words
- **Other outside references:**
  - "phase N" (9)
  - retcon and cozy (about 15)
  - rigel (1)
  - doc paths named in comments (about 17)
- **Already clean:** `api/brep`, tetris, vertical3d and `api/ecs`.

## The rules this refresh adopts

### Writing (docs and comments alike)

- **Plain words, literal statements.** No aphorisms, no inverted sentences ("X, which is what
  Y"), no personification. Code requires, stores, returns or receives; it doesn't want, know,
  owe or trust.
- **Short sentences, one idea each.** Aim for about 25 words, and split anything over 35.
- **Define a term before using it,** or link to the glossary. Prefer standard terms over
  in-house ones ("seam", "the walk", "tier").
- **Present tense.** Say what is true now. No "used to", no dates, no "phase N", no story of
  how a bug was found.
- **Nothing a reader can't open.** No other repositories, deleted trees or tracker IDs.

### Reference docs

- A doc states how a thing works and the rule a reader must follow, **completely, without
  needing an ADR open.**
- An ADR may be linked as a `Background:` line at the end of a section. It is never the
  explanation.
- Each doc serves one kind of reader. A subject can appear in two docs, one for people who use
  it and one for people who change it. Each fact still has one home per reader, and the other
  doc links to it rather than copying it.

### Code comments

- **No ADR numbers, doc paths, plans, phases, milestones or other repositories.**
  - If the sentence makes sense without the reference, delete the reference.
  - If it doesn't, replace the reference with the rule it stood for, in one sentence.
- **State the invariant, the trap or the constraint.** Don't argue why the design beat its
  alternatives; that belongs in the ADR.
- **A regression test says what it checks,** not the history of the bug it guards against.
- **A sentence repeated across files** lives on the shared function. The other copies shrink to
  a short pointer or are deleted.

### ADRs

**What qualifies.** A record is written for a choice between real alternatives, where the
choice is hard to reverse or constrains other code, and where the reasons can't be recovered
from the code. **Not an ADR:**

- a bug fix
- the behaviour of one class or widget
- a naming or layout choice
- a fact that is reference material
- a choice whose only rival is "leave it as it is"

**Title and file name.**

- The title is "Area: choice", in sentence case, about 8 words or fewer.
- One decision per record: no "and" joining two choices.
- The file name is the title in kebab case.
- Example: `0032-loop-fixed-step-simulation-variable-rate-rendering.md`.

**Numbers never change,** so commit messages that cite a number stay valid.

**Template changes.**

- **Header:** `Status` (proposed / accepted / amended / superseded / retired), `Date`,
  `Amends`, `Amended by`, `Supersedes`, `Superseded by`, and `Documented in`, which names the
  reference doc or header that holds the current rule. Drop `Deciders`.
- **Context:** the constraints and forces, in the present tense, five sentences at most.
- **Decision:** three sentences at most, with no tables and no implementation detail.
- **Alternatives:** rejected options only, each one a competent engineer might actually have
  picked.
- **Consequences:** positive, negative, and when to revisit the decision. No counts, run logs
  or migration notes; those belong in commits.

**Amending.**

- A later record that changes an earlier one updates the earlier record's header
  (`Amended by`), not its body.
- If most of the earlier record is overtaken, the later one supersedes it.
- A correction of fact is edited in place.

**The index** is grouped by area: rendering, offline, UI, engine and input, ECS, assets and
files, editor, build and process.

**A principle that keeps repeating moves to Conventions.md as an api design rule:** "a new
option keeps today's behaviour as the default". Records 0049, 0052, 0053 and 0024 restate it.

## ADR triage

R = retire. The record's content moves to the listed home and the file is deleted. Its index row
stays, saying where the content went.
M = merged into another record, then retired.
S = superseded.
Every kept record is renamed and rewritten to the new template.

| # | Verdict | New title | Notes |
|---|---|---|---|
| 0001 | keep | Rendering: replace OpenGL with Vulkan | |
| 0002 | keep | Vulkan: require version 1.3 | |
| 0003 | keep | Rendering: one engine for 2D and 3D | Drop the stale odyssey/SDL_Renderer interim |
| 0004 | keep | Rendering: submit draw items as data | `Operation` is now `DrawItem` |
| 0005 | keep | 2D: one batched quad pipeline | Amended by 0036 and 0042 |
| 0006 | R | — | Project-scope call. Note it in `plans/completed/Modernization.md` |
| 0007 | keep | CI: render tests on software Vulkan | Amended by 0054 |
| 0008 | keep | Shaders: descriptor sets by update frequency | Amended by 0064 |
| 0009 | keep | Colour: display space, UNORM swapchain | Amended by 0049 and 0066 |
| 0010 | S | Meshes: owned by the app that built them | Superseded by 0065; 0061 removed its premise |
| 0011 | keep | Rendering: lines as a world-space primitive | Update stale class names |
| 0012 | keep | Camera: projection targets Vulkan clip space | The bug-fix detail moves to a comment |
| 0013 | keep | Editor: a mesh is a dag node | |
| 0014 | keep | Editor: pick by CPU ray cast | Most of the Decision moves to Editor.md |
| 0015 | keep | Editor: manipulators edit the object transform | Widget detail moves to Editor.md and the manipulator headers |
| 0016 | keep | Editor: undo records completed changes | |
| 0017 | R | — | → Editor.md and `CommandDirectory.h` |
| 0018 | keep | Editor: projects saved as JSON with exact topology | Fixed-path text is stale |
| 0019 | M → 0034 | — | Menu-bar detail goes to `MenuBar.h` and the UI doc |
| 0020 | keep | UI: themes are data, apps load the images | Fold the inline amendment into the body |
| 0021 | keep | Audio: use SDL3_mixer | The asset→audio edge it describes is now reversed; amended by 0079 |
| 0022 | keep | Offline: shared library with no Vulkan | Amended by 0078; remove the talyn text |
| 0023 | keep | Offline: RIB is the scene format | |
| 0024 | R | — | Never built. Its principle moves to Conventions.md |
| 0025 | keep | Offline: RIB reader calls a typed C++ interface | |
| 0026 | keep | Offline: shaders run over batches of points | |
| 0027 | keep | Build: consume the api as source | Amended by 0048 |
| 0028 | keep | Apps: the shared app shell lives in the api | Update namespaces |
| 0029 | keep | Grid: 8-way movement, symmetric line of sight | |
| 0030 | S | Models: one interleaved array | Superseded by 0069 |
| 0031 | keep | Rendering: passes draw into offscreen targets | Amended by 0068 |
| 0032 | keep | Loop: fixed-step simulation, variable-rate rendering | Its stale risks are now false |
| 0033 | keep | Build: select api libraries through a manifest | |
| 0034 | keep | UI: layout is resolved while drawing | Absorbs 0019 and 0039 |
| 0035 | keep | UI: immediate mode beside the retained tree | |
| 0036 | keep | Text: SDF glyphs through the quad shader | |
| 0037 | keep | 2D: clip with a per-batch scissor | |
| 0038 | keep | UI: the UI routes mouse input first | |
| 0039 | M → 0034 | — | Bug fix to 0034's rule. Auto semantics go to the UI doc |
| 0040 | keep | UI: keyboard focus and text input | The amendment's widget rules go to the UI doc |
| 0041 | keep | Files: write documents atomically | |
| 0042 | keep | Rendering: world-space sprites | Amended by 0082 |
| 0043 | keep | Input: apps see raw events before bindings | Amended by 0081 |
| 0044 | R | — | → Rendering internals doc and `DepthBuffer.h` |
| 0045 | R | — | → `Immediate.h` and the UI doc |
| 0046 | R | — | → `Immediate.h` and the UI doc |
| 0047 | keep | Code: exhaustive enum switches | The rule also goes in Conventions.md |
| 0048 | keep | Includes: name headers from the repository root | |
| 0049 | keep | Swapchain: caller picks the format | |
| 0050 | M → 0054 | — | The two-call shape goes in `Capture.h` |
| 0051 | keep | Frames: in-flight ring separate from presenting | |
| 0052 | keep | Camera: selectable handedness | |
| 0053 | keep | Memory: optional VMA suballocation | |
| 0054 | keep | Testing: golden images hold only spec-exact output | The admitted-content list goes to Testing.md |
| 0055 | R | — | → `TextureAtlas.h` |
| 0056 | R | — | → `camera/Profile.h` |
| 0057 | R | — | → `TextBox.h` and the UI doc |
| 0058 | keep | UI: SDL keyboard adapter in ui/shell | Its Alt 4 reasoning was overtaken by 0081 |
| 0059 | keep | UI: enabled is an inherited flag | The site table goes to the UI doc |
| 0060 | keep | ECS: interpolate from a previous-step component | |
| 0061 | keep | Resources: explicit release, generational handles | Amends 0010 |
| 0062 | keep | Grid: parse terrain, not map files | |
| 0063 | keep | ECS: draw from a transform plus a component per kind | |
| 0064 | keep | Lighting: lit passes use the shared recorder | |
| 0065 | keep | Meshes: shared registry keyed by path | Supersedes 0010 |
| 0066 | keep | Lighting: light in linear, draw to sRGB | |
| 0067 | R | — | → `renderer/Lit.h` and the Rendering doc |
| 0068 | keep | Rendering: order passes by what they read | The format check moves to a comment |
| 0069 | keep | Models: material parts over one vertex buffer | The node-walk fix moves to a loader comment |
| 0070 | keep | Animation: CPU sampling, playback on the fixed step | |
| 0071 | keep | Skinning: joint matrices in one storage buffer | |
| 0072 | keep | Particles: an emitter component owns its particles | |
| 0073 | keep | Files: migrate old documents one version at a time | |
| 0074 | R | — | An instance of 0028. → `ui/shell/Screen.h` and the UI doc |
| 0075 | keep | 2D: a canvas may have its own coordinate space | |
| 0076 | keep | Offline: seeded samples resolved by one shared film | Remove the talyn text |
| 0077 | S | Offline: one shared ray tracer | Superseded by 0078, whose rewrite keeps what is still true |
| 0078 | keep | Offline: moya is the one renderer, ray tracing is a hider | |
| 0079 | keep | Assets: loaders are registered | |
| 0080 | keep | Apps: the engine owns startup and shutdown order | |
| 0081 | keep | Input: key events and commands are separate | |
| 0082 | keep | Textures: owned by the device context | |

In total: 65 kept, 14 retired or merged, 3 superseded.

## Target doc layout

| Path | Reader | Built from |
|---|---|---|
| `README.md` | visitor | Keep it, and route readers by audience. Fix the imagetool line |
| `docs/README.md` | everyone | An index grouped by audience, plus the glossary |
| `docs/contributing/GettingStarted.md` | new contributor | Clone, vcpkg, libnoise, first build, test, run — now spread across README, Build and Dependencies |
| `docs/contributing/{Build,Dependencies,Testing,Linting,Conventions}.md` | contributor | The current docs, with their external-consumer parts moved out. Conventions gains the writing rules |
| `docs/api/README.md` | api user | The libraries, how they depend on each other, a glossary of Engine, Engine3D, Controller, canvas, pass and the rest |
| `docs/api/UsingTheApi.md` | api user outside the tree | NewProject.md, plus the external-consumer sections of Build and Dependencies |
| `docs/api/Engine.md` | api user | App lifecycle, the loop, events and input, config, settings, logging, audio |
| `docs/api/Rendering.md` | api user | The "how to draw" half of RenderingPipeline, plus the camera and colour rules |
| `docs/api/UserInterface.md` | api user | The author half of UserInterface, plus the widget rules from the retired ADRs |
| `docs/api/{ECS,Types,Grid,Assets}.md` | api user | ECSDesign; the geometry, animation and effects parts of Architecture; Architecture's grid section; image, font, asset, models |
| `docs/internals/RealtimeRenderer.md` | renderer maintainer | The "how it works" half of RenderingPipeline |
| `docs/internals/UserInterface.md` | ui maintainer | The maintainer half of UserInterface |
| `docs/OfflineRenderer.md` | moya developer | OfflineRenderers.md, opening with an overview and a glossary |
| `docs/Editor.md` | editor developer | Rewritten so it makes sense on its own, from 0013–0018 and 0023 |
| `docs/Games.md` | games and tools | pong, tetris, voxel, odyssey, imagetool, v3dshell |
| `examples/README.md`, `examples/starter/README.md` | someone reading an example | New |

`Architecture.md` is dissolved into the api docs. `sdlc.md` loses its dated plan list.
`plans/README.md` becomes an index table, and each plan's outcome story moves into that plan's
file. `adr/`, `plans/`, `roadmap/`, `audits/` and `TODO.md` stay where they are.

## Phases

1. **Framework.**
   - The rules above go into `Conventions.md`, `sdlc.md`, `adr/README.md`, `adr/template.md`,
     the ADR skill and `CLAUDE.md`.
   - Update the memory note on comments.
2. **ADR renames.**
   - New file names, index grouped by area, `Amended by` and `Superseded by` headers.
   - Rewrite every link to an ADR across the tree with a script.
3. **Reference docs.**
   - The new layout and the rewrite.
   - This is where retired ADRs' content lands.
   - Resolve the contradictions listed above.
4. **ADR bodies.**
   - Rewrite the kept records to the template and fold in the merges.
   - Retired and merged files are deleted.
5. **Comments.**
   - Sweep in this order: `api/ui`, `api/render/realtime`, `api/render/offline` with moya,
     render tests, then everything else.
   - Build, run the tests and run cpplint after each directory.

Each phase is one or more commits on `feat/motion-and-queries`.

## Outcome

All five phases landed on 2026-10-05.

- **Framework.** The writing, comment, document and ADR rules are in
  [contributing/Conventions.md](../../contributing/Conventions.md), [sdlc.md](../../sdlc.md),
  [adr/template.md](../../adr/template.md) and the ADR skill. CLAUDE.md and the reviewer agent
  follow them.
- **ADRs.**
  - 68 records are renamed and rewritten to the template.
  - 14 records were deleted: 0006, 0017, 0019, 0024, 0039, 0044, 0045, 0046, 0050, 0055, 0056,
    0057, 0067 and 0074. Their content now lives in the reference docs and in headers.
  - 0019 and 0039 were folded into 0034, 0050 into 0054, and the still-true half of 0077 into
    0078.
  - The index is grouped by area, and its statuses match the records' headers.
- **Docs.**
  - `docs/` is split into `contributing/`, `api/` and `internals/`.
  - `docs/OfflineRenderer.md`, `docs/Editor.md` and `docs/Games.md` cover the apps.
  - The examples have READMEs.
  - `Architecture.md` was dissolved into the api docs.
  - The contradictions the review found were settled against the code.
- **Comments.**
  - No source file cites an ADR, a document, a plan or another repository.
  - The sweep changed about 620 files and touched comments only. The exceptions are two test
    names that named other repositories.
  - The build, all 25 test suites and cpplint are clean.

**What came out differently from the plan.**

- The plan put the comment sweep last. It ran alongside the doc rewrite instead, because the
  two touch disjoint files.
- 0034 absorbed 0019 and 0039, rather than 0019 being merged on its own.
- 0071 keeps its link to 0064 as an amendment.

**Found along the way, and moved to [TODO.md](../../TODO.md):**

- pong's data directory is not copied into the build.
- Three bindings name commands that nothing handles.
- moya's `--grid` and `--bucket` options do not override a scene's limits.
