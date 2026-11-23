# Production Suite: From Script to Screen

What Vertical3D needs to carry a short film from its first storyboard to its final master, and
in what order. It covers the editor, moya, the processes around them, the libraries they share,
and the outside tools the suite works with. It is written for whoever decides what the
production half of the tree works on next.

This roadmap is the index for that half. [MoyaRebuild.md](MoyaRebuild.md) and
[EditorScripting.md](EditorScripting.md) are parts of it, and keep their own order.

## Glossary

- **DCC**: a digital content creation application, such as the editor, in which an artist builds
  scenes.
- **Composition**: building one scene from several files, each referenced rather than copied,
  with local changes layered over what a reference holds.
- **Override**: a local change to a value that a referenced file holds. The referenced file is
  not changed.
- **Publish**: freezing a numbered version of an asset or a shot, so that other work can depend
  on it.
- **Resolver**: the code that turns a reference to an asset and a version into a file to read.
- **Lookdev**: look development, the work of giving an asset its materials and textures.
- **Layout**: placing assets, cameras and timing for each shot of a sequence.
- **Previz**: a quick realtime render of layout, used to judge cameras and timing.
- **Playblast**: a movie of a shot recorded from a viewport.
- **Filmback**: the size of a camera's sensor, which with the focal length sets the field of view.
- **UDIM**: a numbering of texture tiles, so one surface can use many texture images.
- **Deformer**: an operation that moves a mesh's points, such as a skin or a lattice.
- **Dependency graph**: a graph of nodes in which a node's output is computed from its inputs, and
  recomputed only when an input changes.
- **AOV, LPE**: as [MoyaRebuild.md](MoyaRebuild.md#glossary) defines them.
- **OTIO**: OpenTimelineIO, a file format and library for an edit: its tracks, clips and timing.
- **OpenJD**: Open Job Description, a file format for render jobs that a farm runs.
- **Flipbook**: a player for a sequence of rendered frames, at speed and through a colour
  transform.
- **Slap comp**: a quick composite of a render still in progress, used to judge the render in
  its final form.
- **Deep image**: an image that keeps every sample's depth at each pixel, so renders can be
  combined after the fact without mattes.

## Two halves that stand alone

Vertical3D has two halves.

- **Games.** The api libraries and the realtime engine. Games outside this tree and the demos
  inside it prove them.
- **Production.** Modelling, animation, offline rendering and finishing, for pictures and video
  of cinematic quality. A reference production proves them: a short animated film in a rainy,
  neon-lit street, with a cat, a hovering service robot, and a night that turns to dawn.

The halves meet in the editor. An artist models an asset once, and it goes to a game as glTF or
to a film through moya. Each half stays usable without the other:

- moya and the production libraries need no Vulkan and no SDL.
- A game needs neither moya nor the editor at run time.
- A material, a skin and a mesh authored in the editor reach both halves in one form.

## Where the editor stands

The editor is a capable layout and keyframe tool over simple polygon modelling. It has these
parts:

| Area | Built |
|---|---|
| Modelling | Cube, plane, cylinder and cone. Extrude, inset, split face, split edge and merge. Vertex, edge and face selection with manipulators |
| Curves | Poly, Bezier, Catmull-Rom and NURBS curve items, with points and handles edited in the views |
| Scene | Meshes, curves, cameras, point, distant and spot lights, and groups, in a hierarchy. Hide, show and isolate |
| Animation | Scalar curves on transforms, lens and emission. A timeline, a graph editor and auto key |
| Files | A project directory with a SQLite scene document, autosave and migration. glTF and USD import. RIB export |
| Rendering | Interactive renders through moya's scene interface, and batches of frames through `moya.exe` |
| Scripting | TypeScript on QuickJS against every named command, with a console and a journal |
| Interface | Tiled areas, workspaces, command search, themes and display scaling |

The source is layered with no cycles, and every action is a named command with undo. That gives
scripting, a headless mode and pipeline tools one surface to call. The scene document migrates
between versions. moya's retained scene interface keeps interactive rendering cheap.

### What shapes the next work

Six gaps in the editor's design shape everything below. Each is structural rather than a
missing command.

1. **A project holds one scene and refers to nothing.** A film shares each asset across many
   shots, and each shot changes some of it locally. The editor can only import a copy. This
   blocks every stage after asset building.
2. **A scene has no look.** There are no materials, no UVs and no attributes on a mesh beyond
   its points. A render shows grey geometry, and a game asset cannot carry a texture.
3. **Items are five fixed kinds with fixed keyable paths.** Deformers, constraints and particles
   each need inputs, outputs and an order of evaluation. Nothing in the editor evaluates one
   item from another.
4. **Animation stops at transforms.** `api/animation` has skeletons, skins and clips for games,
   but the editor has no joint, skin or constraint, and no clip above its keys.
5. **The viewports draw their own shaded pass.** The realtime renderer has lit, shadowed and
   skinned passes. A view that used them would show the materials a game sees, and would serve
   as previz.
6. **The only way out is RIB.** There is no glTF or USD export, so the games half receives
   nothing from the editor, and no other tool can read a scene the editor made.

## What established tools do

This section summarises how current tools divide the work. The sections after it are drawn from
it. The tool versions are those current at the time of writing.

### One DCC, many contexts

- **Houdini** is one application with a context for each kind of data: geometry, a USD stage
  for layout and lighting, images in Copernicus, and rigs in APEX. Its 2025 release added
  compositing, baking and rig building as contexts, not as new applications.
- **Blender** is one application with workspaces over one file, including compositing, video
  editing and Grease Pencil storyboarding. Its production tracking and its render farm are
  separate services, Kitsu and Flamenco, reached through add-ons.
- **Foundry and Adobe** sell one application per role: Mari, Katana and Nuke, or Substance
  Painter and Designer. Licensing explains much of that split. Adobe retired its Substance 3D
  Viewer in 2025.
- **Omniverse** builds each of its applications from shared extensions over one USD stage. An
  application there is a packaging of shared code.
- **Unreal** keeps previz, sequencing and final rendering in its editor, and starts a separate
  process to render on other machines.

Services leave the DCC: the farm, the tracker and heavy compute. Stages stay in it as contexts.
A stage becomes a separate application when it never loads a 3D scene and serves a different
person, as finishing does in Flame or Resolve.

### Composition and evaluation

- **USD has no undo.** Every editor that edits USD layers directly builds its own, and shows the
  user which layer an edit lands in. Omniverse and Pixar's Presto each built theirs.
- **Translating loses data.** Maya and 3ds Max keep their own scenes and translate to and from
  USD. Maya's release notes list what a round trip loses: component tags, construction history,
  and parts of variants.
- **Blender** imports and exports USD and keeps its own document. It has no live USD stage.
- **USD's asset resolution** separates an asset's identifier from the file it resolves to.
  OpenAssetIO, at version 1, puts a production tracker behind that resolution.
- **Maya's evaluation** is correct in parallel only when every node declares its dependencies
  exactly. Autodesk still lists cases in which cached playback is wrong.
- **Blender** evaluates a graph between objects, and an ordered stack of modifiers on each one.
  A node graph is one kind of modifier.
- **Houdini's APEX and OpenUSD's OpenExec** keep a rig's evaluation apart from the authored
  scene. OpenExec never writes to the stage, and so far computes transforms only.

### Materials

- **OpenPBR Surface 1.1** is the shared material model. Arnold, Redshift, Omniverse and
  Substance Painter use it, Unreal imports it onto Substrate, and Blender's Principled BSDF
  follows it.
- **MaterialX 1.39** generates shaders from one graph for GLSL, Vulkan GLSL, MSL, OSL, MDL, WGSL
  and Slang. Slang compiles to SPIR-V.
- **Adobe's `openpbr.h`** is a reference OpenPBR BSDF in one header that compiles as C++, GLSL,
  CUDA, MSL and Slang.
- **No product evaluates one material identically in realtime and path traced.** Omniverse,
  Blender and Unreal each pair one definition with two evaluators, and document what the
  realtime one drops, such as thin film or subsurface scattering.
- **glTF's material extensions** map onto OpenPBR's lobes almost one to one. No ratified mapping
  exists.
- **Texture painting** is mostly done in Substance Painter or Mari, which hand texture sets to
  the DCC. DCCs carry basic painting only.
- **xatlas** is an MIT-licensed C++ library that unwraps and packs UVs.

### Characters, effects and scale

- **UsdSkel and glTF skins** carry joints, weights, blend shapes and keyed joints. Neither
  carries IK or constraints, so a rig travels baked. glTF readers need honour only four
  influences per vertex.
- **Grooms** are made in XGen, Yeti, Ornatrix, Houdini or Blender, and exchanged as Alembic
  curves. USD curves import poorly into Unreal, and slow down past millions of curves.
- **Volumes** are exchanged as OpenVDB grids. OpenVDB is Apache-2.0 licensed, and NanoVDB holds
  grids for the GPU.
- **PhysX Flow 2** is a BSD-licensed GPU smoke and gas solver with a Vulkan back end.
- **Instancing** at city scale is done by a point instancer: a list of prototypes and a
  transform for each point, as USD's `PointInstancer` holds them. Clarisse, which specialised in
  this, was discontinued in 2023.
- **Animation layers** are not exposed in Blender's interface, and USD holds no blending of
  clips.

### Denoising and standards

- **Denoising**: OIDN denoises one frame at a time. A temporal OIDN is announced and not
  released. RenderMan ships a temporal denoiser.
- **The ASWF's graduated projects** are OpenEXR, OpenVDB, OpenColorIO, OpenCue, OSL, MaterialX
  and OpenImageIO.

[The pipeline around the suite](#the-pipeline-around-the-suite) covers farms, tracking, review,
compositing, editorial and the specialist modelling tools.

## The pipeline, stage by stage

The reference production follows the usual order of a film. This table says what each stage
needs, where it is done, and what exists.

| Stage | Needs | Done in | Exists |
|---|---|---|---|
| Concept, script | Text, pictures, a beat sheet | Outside tools | Nothing needed |
| Storyboards | Panels with notes, numbered by shot | Outside tools, read as OTIO. Rough boards drawn over the scene in the editor | Nothing |
| Animatic | Panels timed against scratch audio, as an edit | Outside tools, read as OTIO | Nothing |
| Asset modelling | Subdivision modelling, UVs, instancing | Editor | Polygon basics |
| Sculpting | Detail beyond what subdivision gives | Outside tools, imported. In the editor is open | glTF and USD import |
| Lookdev | OpenPBR materials, textures, UDIMs, a lookdev light rig | Editor, moya | Nothing |
| Texture painting | Painted texture sets | Outside tools, imported. In the editor is open | Nothing |
| Rigging | Joints, skins, constraints, IK, deformers | Editor | Nothing |
| Grooming | Hair curves on a surface | Outside tools first, then the editor | Curve items |
| Layout, previz | Shots that reference published assets, physical cameras, a realtime view | Editor | Cameras, keys |
| Animation | Rigs posed and keyed, clips | Editor | Transform keys |
| FX | Particles, rain, volumes, caches | Editor, and outside tools for large simulations | Nothing |
| Lighting | Many lights, light linking, a dome, time of day | Editor, moya | Three light types |
| Rendering | A path tracer, AOVs, a denoiser, a farm | moya, an outside farm | A spectral path tracer with AOVs and a denoiser, on one machine |
| Review | Frames played at speed through a colour transform | Outside players. Snapshots compared in the render view | One picture in the render view |
| Compositing | AOVs and deep images combined | Slap comps in the editor. Final comps in outside compositors, or later the suite's own | Nothing |
| Editorial, delivery | Shots conformed to the edit, encoded masters | Outside tools, through OTIO | Nothing |
| Sound | Music, effects, the mix | Outside tools | Out of scope |

## The tools

The suite holds two applications and the processes around them.

| Tool | What it does | State |
|---|---|---|
| **vertical3d**, the editor | The DCC: modelling, lookdev, rigging, layout and previz, animation, FX, lighting and slap comps, each a workspace | Exists |
| **moya**, the renderer | The path tracer, as a library, a command line and a Hydra delegate. It also bakes textures | Exists, being rebuilt |
| **The headless editor** | The editor with no window, run by scripts and by a farm | Planned in [EditorScripting.md](EditorScripting.md) |
| **imagetool** | Image conversion, sprite sheets, and the tiled, mipmapped textures moya reads | Exists |
| **A pipeline command line** | Publishes versions, resolves references, writes OpenJD jobs, and talks to a tracker | New |

[The pipeline around the suite](#the-pipeline-around-the-suite) says which stages stay outside
it, which tools cover them, and why.

## The pipeline around the suite

A film passes through stages the editor and moya do not cover. For each one, this section names
the outside tools that cover it, and weighs bringing it into the suite against leaving it
outside. Versions, prices and dates are those current at the time of writing. Each tool's row
links the page its facts come from.

### The criteria

Each stage is weighed on these questions. The work of building a feature is not one of them.

1. **Does it work on the scene?** A stage that needs items, selection, undo, the rig or the live
   render gains from running inside the editor.
2. **Does it outlive one session or one machine?** A stage that runs on many machines, keeps
   state for a whole production, or serves several applications at once is a service. A service
   inside a DCC lives and dies with that DCC's session.
3. **Does an open format cross the boundary without loss?** Where EXR, OTIO, OpenJD, Alembic,
   OpenVDB or OpenAssetIO carries everything the stage needs, an outside tool loses nothing.
4. **Is there an open, maintained tool on Windows and Linux?** The suite ships on both, under
   open licences. A stage covered only by closed tools, or by tools without a Linux build, leaves
   a production made with the suite dependent on them.
5. **Does a tight loop give something an outside tool cannot?** Examples are a composite of a
   render still in progress, or hair that follows a surface as it deforms.
6. **Is it a separate role with a mature tool of its own?** Compositors, editors and colourists
   work in tools whose depth a context in a DCC does not reach.

### Render farms

| Tool | Licence | Platforms | Relevant features |
|---|---|---|---|
| [OpenCue](https://docs.opencue.io/docs/concepts/opencue-overview/) 1.37 | Apache-2.0, ASWF | Windows, Linux, macOS | A server, an agent on each host, Python APIs and a REST gateway. Jobs of layers of frames, with dependencies between frames and between jobs. Hosts split into reserved cores and memory. Submitters for Maya, Nuke and Blender |
| [Flamenco](https://flamenco.blender.org/about/) 3.9 | GPL-3.0 | Windows, Linux, macOS | A manager, workers and a Blender add-on over one OpenAPI interface. Job types written in JavaScript. A failed task retried on other workers |
| [Deadline Cloud](https://docs.aws.amazon.com/deadline-cloud/latest/userguide/what-is-deadline-cloud.html) | Closed, a paid AWS service | AWS | Farms, queues and fleets. Jobs are OpenJD templates. Permissions and budgets per user and group |
| [Deadline 10](https://docs.thinkboxsoftware.com/products/deadline/10.4/1_User%20Manual/manual/maintenance-mode-faq.html) | Closed | Windows, Linux, macOS | In maintenance since November 2025: security and critical fixes only |
| [Tractor 2](https://renderman.pixar.com/tractor) | Closed, with RenderMan | Linux, macOS. Windows workers | A job authoring API, allocation tiers and a web dashboard |
| [Afanasy](https://cgru.info/) 3.4 | LGPL-3.0 | Windows, Linux, macOS | Pools, CPU, GPU and memory monitoring, and dependencies between the tasks of two jobs |
| [Qube!](https://www.pipelinefx.com/) 8.1 | Closed | Windows, Linux, macOS | Retries, dependencies, priorities and a Python API |

[Open Job Description](https://github.com/OpenJobDescription/openjd-specifications/wiki) is the
format a job crosses the boundary in. A template holds job parameters, steps, and parameter spaces
such as frame by camera that expand into tasks. It also holds dependencies between steps, and
environments shared by the tasks on one host. Its stated purpose is to let a studio change its
scheduler without rewriting its submitters.

- **For bringing it in:** the editor already runs a batch on one machine.
- **For leaving it outside:** a farm outlives every artist's session, serves every application in
  a studio, and keeps users, permissions and budgets for all of them (criterion 2). Every farm
  above is a server of its own, and a DCC reaches it through a submitter. OpenJD carries a moya
  job without loss (criterion 3), and OpenCue, Flamenco and Afanasy are open on both platforms
  (criterion 4).
- **Where it points:** outside. The editor and the pipeline command line write OpenJD jobs, and
  the editor's batch stays the way to render on one machine.

### Production tracking and asset management

| Tool | Licence | Hosting | Relevant features |
|---|---|---|---|
| [Kitsu](https://github.com/cgwire/zou) 1.1 | AGPL-3.0 server. Paid cloud plans | Self-hosted or cloud | Projects, shots, assets and tasks, which assets each shot uses, versioned previews and review playlists. A Python client |
| [AYON](https://docs.ayon.dev/) | FSL-1.1 server, which becomes Apache-2.0 after two years. Apache-2.0 add-ons | Self-hosted or cloud | REST and GraphQL APIs, a USD resolver, and an add-on for each DCC built on shared publish and load tools |
| [Prism](https://prism-pipeline.com/) 2.1 | A free plan | Not stated | Assets and shots, versions of scenes and exported products, review playlists, and connections to Kitsu, ftrack, Flow and Deadline |
| [ftrack Studio](https://www.ftrack.com/en/studio) | Closed, per user each month | Cloud. Self-hosted only for enterprise | Shots, tasks, review, a Python API and webhooks |
| [Flow Production Tracking](https://developers.shotgridsoftware.com/) | Closed | Cloud | Shots, assets, tasks, versions, review and schedules, with Python and REST APIs |

Two open interfaces put a tracker behind a DCC:

- [OpenAssetIO](https://github.com/OpenAssetIO/OpenAssetIO) 1.0, Apache-2.0, in C++. A host passes
  entity references in place of file paths, and a manager plugin resolves them. Nuke and Katana
  are hosts. AYON and ftrack have managers. It has bridges to USD's resolver and to OTIO.
- [USD's asset resolver](https://openusd.org/release/api/ar_page_front.html), a C++ plugin that
  turns an asset's identifier into a file. Studios use it to resolve their own asset IDs.

The [Blender Kitsu add-on](https://studio.blender.org/tools/addons/blender_kitsu) shows what a DCC
carries. It signs in, links the edit's strips to shots and files to assets, works out the shot
and task from a file's path, and uploads playblasts.

- **For bringing it in:** the editor already keeps a project, and a film's versions follow its
  directory convention.
- **For leaving it outside:** a tracker holds a whole production, across every application and
  every person, for longer than any session (criterion 2). Producers and coordinators use it, and
  never the editor (criterion 6). OpenAssetIO and USD's resolver carry a reference to an asset and
  its version without loss (criterion 3). Kitsu is open and self-hosted (criterion 4).
- **Where it points:** outside. The suite resolves references through an OpenAssetIO-shaped
  interface. Its own manager reads the project directory's versions, so a production with no
  tracker still works, and a Kitsu or AYON manager stands behind the same interface.

### Review

| Tool | Licence | Platforms | Relevant features |
|---|---|---|---|
| [OpenRV](https://github.com/AcademySoftwareFoundation/OpenRV) 4.0 | Apache-2.0, ASWF | Windows, Linux, macOS | Every kind of EXR, including multi-part, layers and multiview. OCIO 2. OTIO import and export. Wipes and difference compares. Caches and audio |
| [xSTUDIO](https://github.com/AcademySoftwareFoundation/xstudio) | Apache-2.0, ASWF | Windows, Linux, macOS | OCIO 2, A/B compares, annotations and audio. Its OTIO timeline was in development on the features page |
| [DJV](https://github.com/darbyjohnston/DJV) 3.7 | BSD-3-Clause | Windows, Linux, macOS | OCIO, OTIO with several media references per clip, wipes, overlays and difference compares, and several audio tracks |
| [mrv2](https://mrv2.sourceforge.io/docs/en/user_docs/overview.html) 1.8 | BSD-3-Clause. Some binary features need a donation | Windows, Linux, macOS | Multi-part, multiview and tiled EXR with layer switching. OCIO 2. OTIO playlists. Wipes and compares. Annotations exported to PDF. A Vulkan build |

- **For bringing it in:** artists compare renders while they light. Karma keeps snapshots of a
  render in a gallery for that.
- **For leaving it outside:** dailies are watched by people who never open the editor, on
  machines with no scene (criterion 6). Four open players read multi-part EXR, OCIO and OTIO on
  both platforms (criteria 3 and 4).
- **Where it points:** outside for dailies. The render view keeps snapshots and shows any AOV, for
  comparisons while lighting.

### Compositing

| Tool | Licence | Platforms | Deep | Cryptomatte | OpenFX |
|---|---|---|---|---|---|
| [Nuke](https://www.foundry.com/products/nuke-family/nuke) 17.1 | Closed, from $3,839 a year. A $499 Indie licence and a limited free one | Windows, Linux, macOS | Yes | Yes | Yes |
| [Fusion Studio](https://www.blackmagicdesign.com/products/fusion) 21.1 | Closed, $295 once. Resolve's free Fusion page is limited | Windows, Linux, macOS | Yes | Yes, from its 3D renderer | Yes |
| [Blender](https://docs.blender.org/manual/en/latest/compositing/types/mask/cryptomatte.html) 5.2 | GPL | Windows, Linux, macOS | No | Yes | No |
| [Copernicus](https://www.sidefx.com/docs/houdini/copernicus/slap_comp.html), in Houdini 22 | Closed | Windows, Linux, macOS | Not stated | Not stated | Yes |
| [Natron](https://github.com/NatronGitHub/Natron) 2.5 | GPL-2.0 | Windows, Linux, macOS | Not stated | Not stated | Yes. Last stable release 2022, and its README asks for maintainers |

A slap comp is a quick composite of a render still in progress, used to judge it in its final
form. Houdini runs one on live Karma renders in the viewport, or in its batch renderer. Blender's
[viewport compositor](https://code.blender.org/2022/07/real-time-compositor/) exists so that
artists do not wait for a full render to start compositing. SideFX positions Copernicus for some
compositing tasks, texture building and slap comps, and its Nuke guide maps Copernicus nodes onto
Nuke's. Foundry bundles editorial and review with compositing in Nuke Studio, and not with 3D.

- **For bringing it in:** a slap comp of moya's live render, light groups and AOVs gives a loop no
  outside compositor gives (criterion 5). No open compositor composites deep images, and the only
  open compositors are Blender's, inside another DCC, and Natron, which is unmaintained
  (criterion 4).
- **For leaving it outside:** final compositing is a role of its own, and Nuke and Fusion carry
  deep compositing, OpenFX plugins and caching at a depth a DCC context has not reached (criterion
  6). EXR carries AOVs, light groups, Cryptomatte and deep samples without loss (criterion 3).
- **Where it points:** slap comps in the editor, built on OpenImageIO, as Houdini and Blender have
  them. Final compositing goes to an outside compositor for now. Whether the suite grows a final
  compositor of its own is a decision: no open, maintained compositor handles deep images.

### Editorial and delivery

| Tool | Licence | Platforms | OTIO |
|---|---|---|---|
| [DaVinci Resolve](https://www.blackmagicdesign.com/products/davinciresolve/compare) 21.1 | Closed. Free, with a $295 Studio version | Windows, Linux, macOS | Reads and writes OTIO |
| [Kdenlive](https://kdenlive.org/news/releases/25.04.0/) | GPL | Windows, Linux, macOS | Reads and writes OTIO through OTIO's C++ library. Effects and transitions are not carried |
| [Blender's sequencer](https://studio.blender.org/blog/opentimelineio-in-blender/) 5.2 | GPL | Windows, Linux, macOS | Through an add-on |
| [Nuke Studio](https://www.foundry.com/products/nuke-family/nuke-studio) | Closed | Windows, Linux, macOS | Conforms from AAF, XML and EDL |
| [FFmpeg](https://ffmpeg.org/ffmpeg-codecs.html) 9.0 | LGPL-2.1 or later | Every platform | Encodes ProRes 4444 and reads the layers and parts of an EXR |

[OTIO](https://opentimelineio.readthedocs.io/en/latest/tutorials/adapters.html) is Apache-2.0.
Its core reads and writes only its own formats. Its adapters for EDL, Final Cut XML and AAF are a
separate, community-supported package.

- **For bringing it in:** the layout workspace already holds a sequence of shots and their frame
  ranges.
- **For leaving it outside:** editing, conform, grading and encoding are roles with tools of their
  own (criterion 6). Their work is on movies and frames, not on a scene (criterion 1). OTIO
  carries the cut without loss (criterion 3). Kdenlive is open and reads OTIO natively, and Resolve
  is free on both platforms (criterion 4).
- **Where it points:** outside. The editor reads and writes OTIO, so the cut and the layout
  workspace's shots stay in step.

### Storyboards

| Tool | Licence | Platforms | Hands on |
|---|---|---|---|
| [Storyboard Pro](https://www.toonboom.com/products/storyboardpro) 27 | Closed, a subscription | Windows, macOS. No Linux | [EDL, AAF and Final Cut XML](https://docs.toonboom.com/help/storyboard-pro-25/storyboard/reference/dialogs/export-edl-aaf-xml-window.html) |
| [Blender Grease Pencil](https://studio.blender.org/tools/artist-guide/pre-production/storyboard) 5.2 | GPL | Windows, Linux, macOS | Boards drawn over 3D scenes and cut in the sequencer. Blender Studio hands editorial individual images |
| [Krita](https://docs.krita.org/en/user_manual/animation.html) 5.3 | GPL-3.0 | Windows, Linux, macOS | Frame-by-frame animation and a storyboard docker. Image sequences and video |
| [OpenToonz](https://github.com/opentoonz/opentoonz) 1.8 | BSD | Windows, Linux, macOS | Image sequences |
| [Storyboarder](https://github.com/wonderunit/storyboarder) 2.1 | Open source | Windows, Linux, macOS | Premiere, Final Cut and Avid XML. Last release 2020 |

- **For bringing it in:** a board drawn over the layout's own cameras and sets turns straight into
  a shot (criterion 5). Blender Studio boards that way. The leading tool is closed and has no Linux
  build (criterion 4).
- **For leaving it outside:** boarding is drawing, by a storyboard artist, mostly before any scene
  exists (criteria 1 and 6). Krita and OpenToonz are open on both platforms, and image sequences
  and OTIO carry the boards and their timing (criterion 3).
- **Where it points:** boards drawn elsewhere come in as image sequences and OTIO. Drawing over the
  scene in the editor serves rough boards and notes in layout. A storyboard application of the
  suite's own is not needed.

### Texture painting

| Tool | Licence | Platforms | Relevant features |
|---|---|---|---|
| [Substance 3D Painter](https://experienceleague.adobe.com/en/docs/substance-3d-painter/using/release-notes/version-12-1) 12.2 | Closed, a subscription only | Windows, macOS, Linux on RHEL and Ubuntu | Layer stacks, smart materials, UDIMs and baking. OpenPBR 1.1 by default, and OpenPBR exported through USD |
| [Mari](https://www.foundry.com/products/mari) 8.0 | Closed, rental only. A free non-commercial version | Windows, Rocky Linux | Thousands of UDIMs at up to 32K, in 32-bit |
| [Blender](https://docs.blender.org/manual/en/latest/sculpt_paint/texture_paint/introduction.html) 5.2 | GPL | Windows, Linux, macOS | Painting on UV images, one image at a time. Texture layers are [in development](https://www.blender.org/roadmap/) |
| [ArmorPaint](https://github.com/armory3d/armortools) | zlib source, paid binaries | Windows, Linux, macOS | Node materials, fill and decal layers, PBR textures exported |
| [3DCoat](https://pilgway.com/store) 2026.11 | Closed, perpetual or rented | Windows, Linux, macOS | Painting with layers and smart materials, beside sculpting and retopology |

- **For bringing it in:** painting works on the scene's own mesh, UVs and materials, under its
  lights (criterion 1). Blender's layered-texture design treats textures and materials as one
  thing, with baking as the way out. The open tools are a texture painter with paid binaries and
  Blender's, which has no layers yet (criterion 4).
- **For leaving it outside:** Painter and Mari are a texture artist's own tools, and handle UDIM
  counts and layer stacks at a scale a DCC's painter does not (criterion 6). Painter's texture sets
  and OpenPBR export cross the boundary without loss (criterion 3).
- **Where it points:** texture sets come in from outside first. Painting in the editor is a
  decision: the open, portable choices are thin, and the stage works on the scene.

### Sculpting

| Tool | Licence | Platforms | Relevant features |
|---|---|---|---|
| [ZBrush](https://www.maxon.net/en/zbrush) 2027 | Closed, a subscription | Windows, macOS, iPad. No Linux | The reference sculpting tool. Geometry handed on through GoZ and standard formats |
| [Blender](https://code.blender.org/2024/11/this-summers-sculpt-mode-refactor/) 5.2 | GPL | Windows, Linux, macOS | Rebuilt in 4.3: five times faster to enter on a 16 million face mesh, and brushes eight times faster. Multires still slows at high levels |
| [Nomad Sculpt](https://nomadsculpt.com/manual/files) | Closed, bought once | Windows, macOS, iOS, Android. No Linux | Multires, a voxel remesher, baking to textures, and glTF, USD and OBJ export |
| 3DCoat | As above | As above | Voxel and surface sculpting |

- **For bringing it in:** sculpted detail ends as subdivision levels and displacement, which moya
  renders from the editor's own surfaces (criterion 1). Only Blender is open and runs on Linux
  (criterion 4).
- **For leaving it outside:** sculpting tens of millions of faces needs a mesh engine of its own.
  Blender's took a rewrite to reach its speed (criterion 6). Meshes and displacement maps cross
  the boundary without loss (criterion 3).
- **Where it points:** meshes and displacement come in from outside first. Sculpting in the editor
  is a decision, for the same reasons as painting.

### Grooming

| Tool | Licence | Hosts | Hands on |
|---|---|---|---|
| [XGen](https://help.autodesk.com/cloudhelp/2022/ENU/Maya-CharEffEnvBuild/files/GUID-28367DFD-A479-4E29-A691-6381CC8AF384.htm) | Closed, in Maya | Maya | Alembic caches with a width per point |
| [Yeti](https://www.cgchannel.com/2025/03/peregrine-labs-ships-yeti-5/) 5.2 | Closed | Maya only | Rendered by Arnold, Redshift, RenderMan, V-Ray and Unreal. Export formats not confirmed |
| [Ornatrix](https://www.cgchannel.com/2026/08/ephere-releases-ornatrix-4-for-cinema-4d/) | Closed | 3ds Max, Maya, Cinema 4D, Unreal | Alembic |
| [Houdini](https://www.sidefx.com/docs/houdini/fur/workflow.html) 22 | Closed | Houdini | Guides groomed, processed, generated, deformed and simulated in one network |
| [Blender](https://docs.blender.org/manual/en/latest/files/import_export/usd.html) 5.2 | GPL | Blender | Curves driven by geometry nodes, with hair dynamics. USD curves |

- **For bringing it in:** a groom follows its surface. Blender's curves deform with an animated
  surface, and Houdini's attributes painted on the skin drive the groom in one network
  (criterion 5). Every closed groomer lives inside a closed DCC, so none can be paired with this
  suite (criterion 4).
- **For leaving it outside:** Alembic carries a finished groom as curves (criterion 3).
- **Where it points:** in the editor, as section 5 orders it, with Alembic grooms read in until
  then.

## Shared foundations

The tools share these libraries. Each is named by what it holds; the names are not decided.

| Library | Holds | Used by |
|---|---|---|
| Composition | References between documents, overrides, a resolver, and USD export of published assets | Editor, pipeline command line |
| Materials | OpenPBR 1.1 parameters as the stored material, and MaterialX graphs for patterns | Editor, moya, realtime renderer |
| Images | EXR with layers and deep samples, and OpenColorIO, moved down from moya to `api/image` | moya, editor, imagetool |
| Mesh attributes | UVs, normals, colours and creases on `api/brep` faces and corners | Editor, glTF and USD export, moya |
| Evaluation | A graph between items and a stack of deformers on each, cached by time | Editor |
| Skins | Joints, weights, blend shapes and clips, shared with `api/animation` | Editor, games |
| Caches | Points, curves and volumes over time, read from Alembic and OpenVDB | Editor, moya |
| Pipeline | Sequences, shots, assets, versions and publishes | Every tool |
| Timeline | An OTIO edit, read and written through OTIO's C++ library | Editor, pipeline command line |

## Decisions it needs

Each of these is an ADR before the work it gates starts. Each names where the tools above point.

- **How a scene is composed.** The editor's document gains references and overrides, or USD
  layers become the document. The tools above point to the first: no USD editor gets undo from
  USD, and translating DCCs lose data on the round trip. USD stays the format a published asset
  is written in.
- **What a material is.** OpenPBR's parameters stored directly, or a MaterialX graph for every
  material. Painter's texture sets and glTF's extensions both map onto the parameters.
- **How the realtime renderer evaluates a material.** Shaders generated by MaterialX, through
  Slang or Vulkan GLSL, or a fixed lit shader that reads OpenPBR's parameters. Either way, the
  lobes it drops are written down and tested.
- **How the editor evaluates.** A full dependency graph, a stack of modifiers on each item, or
  Blender's hybrid of a graph between items and a stack on each. The tools above point to the
  hybrid. Each evaluator declares its inputs, results are cached by time, and evaluation never
  writes to the document.
- **Where an asset's versions live.** Files on disk named by convention, a tracker reached
  through OpenAssetIO, or both behind one resolver.
- **Which outside tools the suite writes for.** One ADR fixes the formats and naming of each
  interchange: OTIO, OpenJD, EXR channel names and Cryptomatte, Alembic and OpenVDB.
- **Whether the suite grows a final compositor.** Slap comps in the editor are not in question.
  A compositor for final frames would fill the one gap no open, maintained tool covers: deep
  images.
- **Whether painting and sculpting come into the editor.** Both work on the scene, and their open,
  portable tools are Blender's and a painter with paid binaries. Painter, Mari and ZBrush work at
  a scale and depth of their own.

## What it needs

### 1. A look

Materials and their inputs, in the editor and in both renderers.

- **Mesh attributes**: UVs, normals, vertex colours and creases, kept through every modelling
  operation and undo.
- **UVs**: automatic unwrap and packing through xatlas, then seams and UDIM tiles edited by hand.
- **Subdivision surfaces**: a mesh item drawn and rendered subdivided, with creases, through
  OpenSubdiv.
- **OpenPBR materials as scene records**, assigned to items and to faces, edited in the Shading
  workspace, with texture sets from Painter or Mari read in.
- **The viewports on the realtime renderer's lit pass**, evaluating the same materials, with a
  test that renders one material in both renderers and compares them.
- **glTF and USD export** of meshes, materials, cameras, lights and keys.

It needs moya's path tracer and materials, sections 4 and 5 of [MoyaRebuild.md](MoyaRebuild.md).

### 2. Shots from shared assets

- **References**: an item that names a published asset and its version, with overrides of its
  values kept in the shot. Undo covers the overrides.
- **A resolver** from an asset and a version to a file, shaped like USD's and OpenAssetIO's.
- **The pipeline command line**: publish a version, list versions, and resolve a reference.
- **A point instancer**: one asset placed many times, and scattered over a surface. moya's
  hierarchy holds the instances once.
- **A headless editor**, as [EditorScripting.md](EditorScripting.md) orders it.

### 3. Characters

- **Joints, skins and weight painting**, stored so that glTF skins and UsdSkel carry them
  without loss.
- **Evaluation**: the graph between items and the deformer stack, with the editor's parenting
  moved onto it.
- **Constraints and IK**: two-bone and chain solvers, aim, parent and point constraints, and a
  switch between FK and IK. Each bakes to keys on joints for export.
- **Deformers**: lattice, blend shapes, and a wrap of one mesh onto another.
- **Clips**: a clip of keys, offset and blended with another, without layers.
- **Rigs and clips exported as glTF skins and animations**, at four influences per vertex, so a
  game plays what the editor animated.

### 4. A world at scale

- **Physical cameras**: a filmback, a focal length, focus distance and aperture, which moya's
  depth of field uses.
- **A layout workspace**: a sequence of shots, each with its camera and frame range, cut from an
  OTIO edit. Image plates and audio from the animatic. Playblasts of each shot.
- **Many lights**: rect, disk, dome and mesh lights in the editor, light linking, and IES
  profiles.
- **Time of day**: a sky model and a sun keyed together.
- **Scenes of tens of thousands of instances** that open, draw and render at interactive rates.

### 5. Effects and hair

- **Caches read in**: Alembic curves for grooms, and OpenVDB sequences for smoke, steam and fog
  from outside simulators.
- **Particles in the editor**, emitted, stepped at a fixed rate and cached to disk.
- **Rain**: particles rendered as motion-blurred curves, a mist volume, and splashes and wet
  surfaces keyed over time.
- **Smoke and steam in the editor**, through PhysX Flow 2 or a solver of moya's own.
- **Grooming in the editor**: guide curves on a surface, hair interpolated between them, and
  clumping and noise.

It needs moya's volumes and hair BSDF, section 8 of [MoyaRebuild.md](MoyaRebuild.md).

### 6. Render and review

- **OpenJD jobs**, written by the editor and the pipeline command line, split by frame, run by
  OpenCue, Deadline Cloud or a local runner.
- **Frames that render the same on every machine**, which moya's stateless sampler gives.
- **Temporal stability**: frames denoised without flicker across a sequence. OIDN works on one
  frame at a time.
- **Movies and OTIO** that the open review players read.
- **Snapshots in the render view**, kept with their settings, compared by wipe, with any AOV
  shown.

### 7. Finishing

- **Slap comps in the editor**: a node graph over moya's live render, its AOVs and light groups,
  through OpenImageIO, applied in the render view and to a batch.
- **EXR for compositors**: moya's AOVs, light groups, Cryptomatte and deep images, named as Nuke,
  Resolve and Blender read them.
- **The cut**: shots and their versions conformed to an OTIO edit, and written back as OTIO.

It needs moya's compositing output, section 6 of [MoyaRebuild.md](MoyaRebuild.md).

### 8. Preproduction

- **Boards read in**: an animatic's OTIO, with its panels and timing, cuts the layout
  workspace's sequence.
- **Panels in layout**: each shot shows its board as an image plate.
- **Drawing over the scene**, as Grease Pencil does, for rough boards and notes in a view.

### 9. Game assets

The games half's share of the editor's work, apart from what sections 1 and 3 export.

- **Baking**: moya renders normal, occlusion and lighting maps from a detailed mesh onto a
  simpler one's UVs.
- **Levels of detail**, made and previewed in the editor and exported with the asset.
- **Collision shapes**, authored on an item and exported with it.

## The order

Section 1 comes first, because every later stage renders materials, and the games half needs its
export. Section 2 comes next, because a production of more than one shot cannot be organised
without it. Sections 3, 4 and 5 can run beside each other once section 2 is done, and section
5's caches can start earlier. Section 5's grooming in the editor needs section 3's evaluation.
Section 6 can start once moya renders frames, and is worth most once shots exist. Section 7
needs moya's compositing output. Section 8 follows section 4's layout workspace. Section 9
follows sections 1 and 3.

```
moya 4–5 ──> 1 A look ──> 2 Shots from shared assets ──┬──> 3 Characters ──> 5 Grooming, simulation
                 │                                     ├──> 4 A world at scale ──> 8 Preproduction
                 │                                     └──> 5 Caches, particles, rain
                 └───────────────────────────────────────────────> 9 Game assets (after 3)

moya 4 ──> 6 Render and review        moya 6 ──> 7 Finishing
```

## Sources

The tools, opened at the time of writing:

- Houdini 21: <https://www.sidefx.com/products/whats-new-in-h21/>
- Copernicus: <https://www.sidefx.com/docs/houdini/copernicus/index.html>
- Blender Studio's tools: <https://studio.blender.org/tools/overview/introduction>
- Omniverse Kit applications: <https://github.com/NVIDIA-Omniverse/kit-app-template>
- Unreal's Movie Render Pipeline:
  <https://dev.epicgames.com/documentation/en-us/unreal-engine/movie-render-pipeline-in-unreal-engine>
- Unreal's cinematics:
  <https://dev.epicgames.com/documentation/unreal-engine/cinematics-and-movie-making-in-unreal-engine>
- Autodesk Flow:
  <https://adsknews.autodesk.com/en/views/what-is-autodesk-flow-and-how-does-it-work/>

Composition and evaluation:

- OpenUSD 26.08: <https://aousd.org/blog/announcing-openusd-v26-08-key-features-and-improvements/>
- The OpenUSD core specification:
  <https://digitalproduction.com/2026/01/05/aousd-releases-openusd-core-specification-1-0/>
- USD's asset resolution: <https://openusd.org/release/wp_ar2.html>
- OpenExec: <https://openusd.org/release/intro_to_openexec.html>
- UsdSkel: <https://openusd.org/dev/api/_usd_skel__intro.html>
- OpenAssetIO: <https://github.com/OpenAssetIO/OpenAssetIO/releases>
- Maya's parallel evaluation:
  <https://download.autodesk.com/us/company/files/2018/UsingParallelMaya.html>
- KineFX: <https://www.sidefx.com/docs/houdini/character/kinefx/overview.html>
- Blender's animation projects:
  <https://developer.blender.org/docs/features/animation/module/projects/>

Materials:

- MaterialX releases: <https://github.com/AcademySoftwareFoundation/MaterialX/releases>
- MaterialX and Slang:
  <https://www.aswf.io/blog/materialx-adds-support-for-slang-shader-generation/>
- Adobe's OpenPBR BSDF:
  <https://digitalproduction.com/2026/03/11/openpbr-bsdf-reference-code-from-adobe/>
- Omniverse's OpenPBR template:
  <https://docs.omniverse.nvidia.com/materials-and-rendering/latest/templates/OpenPBR.html>
- Substance Painter 12.1:
  <https://experienceleague.adobe.com/en/docs/substance-3d-painter/using/release-notes/version-12-1>

Effects:

- OpenVDB's changes: <https://www.openvdb.org/documentation/doxygen/changes.html>
- PhysX Flow: <https://nvidia-omniverse.github.io/PhysX/flow/index.html>
- Grooms in Unreal:
  <https://dev.epicgames.com/documentation/en-us/unreal-engine/using-alembic-for-grooms-in-unreal-engine>

Pipeline services:

- Open Job Description: <https://github.com/OpenJobDescription/openjd-specifications>
- OpenTimelineIO releases: <https://github.com/AcademySoftwareFoundation/OpenTimelineIO/releases>
- xSTUDIO releases: <https://github.com/AcademySoftwareFoundation/xstudio/releases>
- OIDN releases: <https://github.com/OpenImageDenoise/oidn/releases>
- Storyboard Pro's export:
  <https://docs.toonboom.com/help/storyboard-pro-25/storyboard/reference/dialogs/export-edl-aaf-xml-window.html>
- Blender Studio's storyboards:
  <https://studio.blender.org/tools/artist-guide/pre-production/storyboard>
- Kitsu: <https://github.com/cgwire/kitsu>
- The ASWF's projects: <https://tac.aswf.io/process/review_cycle>
