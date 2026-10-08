# Architecture Decision Records

An ADR records a decision that shaped the code: the forces behind it, the options that were
rejected, and what it costs. It is background reading. The rule a decision produced is stated
in full in the reference document named in the record's `Documented in` line, and that
document is where to look for how things work.

A decision earns a record when there was a real alternative, the choice is hard to reverse or
constrains other code, and the reasons cannot be read from the code. Bug fixes, the behaviour
of one class, and reference facts do not. [sdlc.md](../sdlc.md#2-decide) has the full test,
and the `architecture-decision-records` skill covers the format. Copy
[template.md](template.md) to start one.

**Statuses.** *accepted* is in force as written. *amended* is in force, but a later record
changes part of it, and its header names which. *superseded* has been replaced outright. Some
numbers have no file. Those records turned out not to be decisions; each row says where the
content went. Numbers are never reused.


## Rendering

| ADR | Decision | Status |
|---|---|---|
| [0001](0001-rendering-replace-opengl-with-vulkan.md) | Rendering: replace OpenGL with Vulkan | accepted |
| [0002](0002-vulkan-require-version-1-3.md) | Vulkan: require version 1.3 | accepted |
| [0003](0003-rendering-one-engine-for-2d-and-3d.md) | Rendering: one engine for 2D and 3D | accepted |
| [0004](0004-rendering-submit-draw-items-as-data.md) | Rendering: submit draw items as data | accepted |
| [0005](0005-2d-one-batched-quad-pipeline.md) | 2D: one batched quad pipeline | amended by 0036, 0042 |
| [0008](0008-shaders-descriptor-sets-by-update-frequency.md) | Shaders: descriptor sets by update frequency | amended by 0064 |
| [0009](0009-colour-display-space-unorm-swapchain.md) | Colour: display space, UNORM swapchain | amended by 0049, 0066 |
| [0010](0010-meshes-owned-by-the-app-that-built-them.md) | Meshes: owned by the app that built them | amended by 0061; superseded by 0065 |
| [0011](0011-rendering-lines-as-a-world-space-primitive.md) | Rendering: lines as a world-space primitive | accepted |
| [0012](0012-camera-projection-targets-vulkan-clip-space.md) | Camera: projection targets Vulkan clip space | amended by 0052 |
| [0031](0031-rendering-passes-draw-into-offscreen-targets.md) | Rendering: passes draw into offscreen targets | amended by 0068 |
| [0036](0036-text-sdf-glyphs-through-the-quad-shader.md) | Text: SDF glyphs through the quad shader | accepted |
| [0037](0037-2d-clip-with-a-per-batch-scissor.md) | 2D: clip with a per-batch scissor | accepted |
| [0042](0042-rendering-world-space-sprites.md) | Rendering: world-space sprites | amended by 0082 |
| 0044 | *Not a decision. Now in docs/internals/realtime/Memory.md and vulkan/frame/DepthBuffer.h* | removed |
| [0049](0049-swapchain-caller-picks-the-format.md) | Swapchain: caller picks the format | accepted |
| [0051](0051-frames-in-flight-ring-separate-from-presenting.md) | Frames: in-flight ring separate from presenting | accepted |
| [0052](0052-camera-selectable-handedness.md) | Camera: selectable handedness | accepted |
| [0053](0053-memory-optional-vma-suballocation.md) | Memory: optional VMA suballocation | accepted |
| 0055 | *Not a decision. Now in api/image/TextureAtlas.h* | removed |
| 0056 | *Not a decision. Now in api/type/camera/Profile.h* | removed |
| [0061](0061-resources-explicit-release-generational-handles.md) | Resources: explicit release, generational handles | accepted |
| [0064](0064-lighting-lit-passes-use-the-shared-recorder.md) | Lighting: lit passes use the shared recorder | amended by 0071 |
| [0065](0065-meshes-shared-registry-keyed-by-path.md) | Meshes: shared registry keyed by path | amended by 0082 |
| [0066](0066-lighting-light-in-linear-draw-to-srgb.md) | Lighting: light in linear, draw to sRGB | accepted |
| 0067 | *Not a decision. Now in docs/api/rendering/Lighting.md and vulkan/renderer/Lit.h* | removed |
| [0068](0068-rendering-order-passes-by-what-they-read.md) | Rendering: order passes by what they read | accepted |
| [0071](0071-skinning-joint-matrices-in-one-storage-buffer.md) | Skinning: joint matrices in one storage buffer | accepted |
| [0075](0075-2d-a-canvas-may-have-its-own-coordinate-space.md) | 2D: a canvas may have its own coordinate space | accepted |
| [0082](0082-textures-owned-by-the-device-context.md) | Textures: owned by the device context | accepted |
| [0085](0085-rendering-a-pass-chooses-whether-depth-is-written.md) | Rendering: a pass chooses whether depth is written | accepted |

## Offline rendering

| ADR | Decision | Status |
|---|---|---|
| [0022](0022-offline-shared-library-with-no-vulkan.md) | Offline: shared library with no Vulkan | amended by 0078 |
| [0023](0023-offline-rib-is-the-scene-format.md) | Offline: RIB is the scene format | accepted |
| 0024 | *Never built. Its principle is in docs/contributing/Conventions.md, under API design* | removed |
| [0025](0025-offline-rib-reader-calls-a-typed-handler-interface.md) | Offline: RIB reader calls a typed handler interface | accepted |
| [0026](0026-offline-shaders-run-over-batches-of-points.md) | Offline: shaders run over batches of points | accepted |
| [0076](0076-offline-seeded-samples-resolved-by-one-shared-film.md) | Offline: seeded samples resolved by one shared film | accepted |
| [0077](0077-offline-one-shared-ray-tracer.md) | Offline: one shared ray tracer | superseded by 0078 |
| [0078](0078-offline-moya-is-the-one-renderer-ray-tracing-is-a-hider.md) | Offline: moya is the one renderer, ray tracing is a hider | accepted |

## User interface

| ADR | Decision | Status |
|---|---|---|
| 0019 | *Merged into 0034* | removed |
| [0020](0020-ui-themes-are-data-apps-load-the-images.md) | UI: themes are data, apps load the images | accepted |
| [0034](0034-ui-layout-is-resolved-while-drawing.md) | UI: layout is resolved while drawing | accepted |
| [0035](0035-ui-immediate-mode-beside-the-retained-tree.md) | UI: immediate mode beside the retained tree | accepted |
| [0038](0038-ui-the-ui-hit-tests-the-mouse-before-the-app.md) | UI: the UI hit-tests the mouse before the app | accepted |
| 0039 | *Merged into 0034* | removed |
| [0040](0040-ui-keyboard-focus-and-text-input.md) | UI: keyboard focus and text input | amended by 0058 |
| 0045 | *Not a decision. Now in docs/api/ui/ImmediateMode.md and api/ui/Immediate.h* | removed |
| 0046 | *Not a decision. Now in docs/api/ui/ImmediateMode.md and api/ui/Immediate.h* | removed |
| 0057 | *Not a decision. Now in docs/api/ui/Keyboard.md and api/ui/component/TextBox.h* | removed |
| [0058](0058-ui-sdl-keyboard-adapter-in-ui-shell.md) | UI: SDL keyboard adapter in ui/shell | accepted |
| [0059](0059-ui-enabled-is-an-inherited-flag.md) | UI: enabled is an inherited flag | accepted |
| 0074 | *Not a decision. Now in docs/api/ui/Setup.md and api/ui/shell/Screen.h* | removed |

## Engine, input and assets

| ADR | Decision | Status |
|---|---|---|
| [0021](0021-audio-use-sdl3-mixer.md) | Audio: use SDL3_mixer | amended by 0079, 0084 |
| [0028](0028-apps-the-shared-app-shell-lives-in-the-api.md) | Apps: the shared app shell lives in the api | accepted |
| [0030](0030-models-one-interleaved-array.md) | Models: one interleaved array | superseded by 0069 |
| [0032](0032-loop-fixed-step-simulation-variable-rate-rendering.md) | Loop: fixed-step simulation, variable-rate rendering | accepted |
| [0041](0041-files-write-documents-atomically.md) | Files: write documents atomically | accepted |
| [0043](0043-input-apps-see-raw-events-before-bindings.md) | Input: apps see raw events before bindings | amended by 0081 |
| [0069](0069-models-material-parts-over-one-vertex-buffer.md) | Models: material parts over one vertex buffer | accepted |
| [0073](0073-files-migrate-old-documents-one-version-at-a-time.md) | Files: migrate old documents one version at a time | accepted |
| [0079](0079-assets-loaders-are-registered.md) | Assets: loaders are registered | accepted |
| [0080](0080-apps-the-engine-owns-startup-and-shutdown-order.md) | Apps: the engine owns startup and shutdown order | accepted |
| [0081](0081-input-key-events-and-commands-are-separate.md) | Input: key events and commands are separate | accepted |
| [0084](0084-audio-events-and-parameters-behind-an-interface.md) | Audio: events and parameters behind an interface | accepted |

## Grid and ECS

| ADR | Decision | Status |
|---|---|---|
| [0029](0029-grid-8-way-movement-symmetric-line-of-sight.md) | Grid: 8-way movement, symmetric line of sight | accepted |
| [0060](0060-ecs-interpolate-from-a-previous-step-component.md) | ECS: interpolate from a previous-step component | accepted |
| [0062](0062-grid-parse-terrain-not-map-files.md) | Grid: parse terrain, not map files | accepted |
| [0063](0063-ecs-draw-from-a-transform-plus-a-component-per-kind.md) | ECS: draw from a transform plus a component per kind | accepted |
| [0070](0070-animation-cpu-sampling-playback-on-the-fixed-step.md) | Animation: CPU sampling, playback on the fixed step | accepted |
| [0072](0072-particles-an-emitter-component-owns-its-particles.md) | Particles: an emitter component owns its particles | accepted |

## Editor

| ADR | Decision | Status |
|---|---|---|
| [0013](0013-editor-a-mesh-is-a-dag-node.md) | Editor: a mesh is a dag node | accepted |
| [0014](0014-editor-pick-by-cpu-ray-cast.md) | Editor: pick by CPU ray cast | accepted |
| [0015](0015-editor-manipulators-edit-the-object-transform.md) | Editor: manipulators edit the object transform | accepted |
| [0016](0016-editor-undo-records-completed-changes.md) | Editor: undo records completed changes | accepted |
| 0017 | *Not a decision. Now in docs/editor/CommandsAndUndo.md and vertical3d/src/command/CommandDirectory.h* | removed |
| [0018](0018-editor-projects-saved-as-json-with-exact-topology.md) | Editor: projects saved as JSON with exact topology | amended by 0041 |

## Build, testing and code

| ADR | Decision | Status |
|---|---|---|
| [0007](0007-ci-render-tests-on-software-vulkan.md) | CI: render tests on software Vulkan | amended by 0054 |
| [0027](0027-build-consume-the-api-as-source.md) | Build: consume the api as source | amended by 0048 |
| [0033](0033-build-select-api-libraries-through-a-manifest.md) | Build: select api libraries through a manifest | accepted |
| [0047](0047-code-exhaustive-enum-switches.md) | Code: exhaustive enum switches | accepted |
| [0048](0048-includes-name-headers-from-the-repository-root.md) | Includes: name headers from the repository root | accepted |
| 0050 | *Merged into 0054* | removed |
| [0054](0054-testing-golden-images-hold-only-spec-exact-output.md) | Testing: golden images hold only spec-exact output | accepted |

## Process

| ADR | Decision | Status |
|---|---|---|
| 0006 | *Not a decision: a project-scope call to keep tetris. Noted in docs/plans/completed/Modernization.md* | removed |
| [0083](0083-review-a-changeset-answers-for-what-it-introduces.md) | Review: a changeset answers for what it introduces | accepted |
