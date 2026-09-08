# Architecture Decision Records

Why the codebase is shaped the way it is. A decision earns an ADR when it is hard to
reverse, constrains later phases, or a future reader would otherwise ask "why on earth is
it done this way". Anything smaller belongs in a comment beside the thing it explains.

Copy [template.md](template.md) and add a row below. Superseding does not delete: set the
old record's status and leave the file in place.

0026 is missing from the sequence rather than lost: it is reserved by
[the open shading plan](../plans/OfflineRenderingPhase3.md), whose first step writes it.

| ADR | Title | Status | Date |
|---|---|---|---|
| [0001](0001-vulkan-replaces-opengl.md) | Rendering Backend — Vulkan Replaces OpenGL Outright | accepted | 2026-08-30 |
| [0002](0002-target-vulkan-1-3.md) | Vulkan Version — Target 1.3 Rather Than 1.0 | accepted | 2026-08-30 |
| [0003](0003-one-realtime-engine.md) | Realtime Engine — One Vulkan Engine Rather Than Separate 2D And 3D | accepted | 2026-08-30 |
| [0004](0004-operations-as-draw-data.md) | Render Submission — Operations Are Draw Data, Not Draw Code | accepted | 2026-08-30 |
| [0005](0005-one-batched-quad-primitive.md) | 2D Drawing — One Batched Quad Primitive With An Optional Texture | accepted | 2026-08-30 |
| [0006](0006-keep-both-pong-and-tetris.md) | App Scope — Both Pong And Tetris Are Kept | accepted | 2026-08-30 |
| [0007](0007-ci-rendering-tests.md) | CI Rendering Tests — Software Vulkan On A Windows Runner | accepted | 2026-08-30 |
| [0008](0008-binding-by-update-frequency.md) | Shader Bindings — Descriptor Sets By Update Frequency, Per-Object Data In Push Constants | accepted | 2026-08-31 |
| [0009](0009-colour-authored-in-display-space.md) | Colour — Authored In Display Space, Presented Through A UNORM Swapchain | accepted | 2026-08-31 |
| [0010](0010-meshes-are-owned-by-the-app.md) | Geometry Ownership — Meshes Belong To The App, Not To Resources | accepted | 2026-08-31 |
| [0011](0011-lines-are-the-second-primitive.md) | Line Drawing — A Second Primitive, In World Space, Through The Pass Camera | accepted | 2026-09-01 |
| [0012](0012-camera-builds-vulkan-clip-space.md) | Camera Convention — `v3d::type::Camera` Builds Vulkan Clip Space | accepted | 2026-09-01 |
| [0013](0013-mesh-is-a-dag-node.md) | Scene Model — A Mesh Is A dag Node With A Transform, And The Scene Belongs To The Editor | accepted | 2026-09-02 |
| [0014](0014-picking-is-a-cpu-ray-cast.md) | Picking — A CPU Ray Cast Against The Brep, With Screen Space Proximity For Components | accepted | 2026-09-02 |
| [0015](0015-manipulators-write-the-object-transform.md) | Manipulators — Handles Write The Object Transform, And Are An Overlay Pass | accepted | 2026-09-02 |
| [0016](0016-undo-records-what-has-already-happened.md) | Undo — A Command Records What Has Already Happened, And A Gesture Is One Of Them | accepted | 2026-09-02 |
| [0017](0017-a-command-is-a-name-in-a-context.md) | Command Dispatch — A Command Is A Name In A Context, And The Directory Is The Editor's | accepted | 2026-09-02 |
| [0018](0018-a-project-is-json-and-stores-topology-verbatim.md) | Project Persistence — A Project Is JSON, And The Topology Is Stored Verbatim | accepted | 2026-09-02 |
| [0019](0019-the-ui-is-laid-out-by-what-draws-it.md) | Menu Bar — The UI Is Laid Out By What Draws It, And Hit Tested Against Those Bounds | accepted | 2026-09-02 |
| [0020](0020-a-theme-is-data-and-the-app-resolves-its-images.md) | Themes — A Theme Is Data, And The App Resolves The Images It Names | accepted | 2026-09-04 |
| [0021](0021-sdl3-mixer-replaces-soloud.md) | Audio Backend — SDL3_mixer Replaces SoLoud, And A Clip Resolves Through The Asset Manager | accepted | 2026-09-04 |
| [0022](0022-offline-rendering-shares-an-api-library.md) | Offline Rendering Layout — Shared Code Lives In `api/render/offline`, And Each Renderer Is A Library With A Driver | accepted | 2026-09-04 |
| [0023](0023-rib-is-the-offline-scene-description.md) | Offline Scene Description — RIB Is What Both Renderers Read, And The Editor Exports To It | accepted | 2026-09-04 |
| [0024](0024-api-type-serves-both-renderers.md) | Shared Types — `api/type` Serves Both Renderers, And A Convention Is A Parameter Rather Than A Fork | accepted | 2026-09-04 |
| [0025](0025-the-rib-reader-dispatches-a-cpp-request-interface.md) | RIB Dispatch — The Reader Hands A Renderer C++ Requests With Typed Parameter Lists | accepted | 2026-09-05 |
| [0027](0027-the-api-is-consumed-as-source.md) | External Consumption — The api Is Taken As Source Through An `add_subdirectory`-able Root, Not As An Installed Package | accepted | 2026-09-05 |
| [0028](0028-an-apps-shell-belongs-to-the-api.md) | App Shell — What Every App Repeats Belongs To The api, Not To Each App | accepted | 2026-09-05 |
| [0029](0029-tile-grids-are-an-api-library.md) | Tile Grids — A Library Of Their Own, 8-Way On The Ground Plane, Asked Rather Than Told What Blocks | accepted | 2026-09-06 |
| [0030](0030-a-model-is-an-interleaved-array-that-names-its-texture.md) | Loaded Geometry — A Model Is One Interleaved Array With A Material That Names Its Texture | accepted | 2026-09-06 |
| [0031](0031-a-pass-draws-into-a-target-it-names.md) | Offscreen Rendering — A Pass Draws Into A Target It Names, And The Recorder Leaves It Readable | accepted | 2026-09-06 |
| [0032](0032-the-loop-simulates-at-a-fixed-step.md) | Game Loop — The Loop Simulates At A Fixed Step And Renders At A Variable One | accepted | 2026-09-06 |
| [0033](0033-a-consumer-selects-the-api-libraries-it-wants.md) | API Selection — A Consumer Names The Libraries It Wants, And A Manifest Expands The Closure | accepted | 2026-09-06 |
| [0034](0034-a-component-has-children-and-a-box.md) | UI Layout — A Component Has Children, And The Draw Walk Resolves Its Box | accepted | 2026-09-06 |
| [0035](0035-an-immediate-mode-layer-over-the-same-canvas.md) | Immediate Mode — A Second Way To Draw A UI, Onto The Same Canvas | accepted | 2026-09-06 |
| [0036](0036-text-is-a-distinct-kind-of-quad.md) | 2D Drawing — Text Is A Distinct Kind Of Quad, And The Primitive Carries Which | accepted | 2026-09-06 |
| [0037](0037-clipping-is-a-scissor-the-batch-carries.md) | Clipping — A Clip Rectangle Is Batch State, And The Device Scissors The Draw | accepted | 2026-09-06 |
| [0038](0038-a-cursor-is-routed-by-the-library-that-drew-it.md) | UI Input — A Cursor Is Routed By The Library That Drew It, And A Press Dispatches A Command | accepted | 2026-09-06 |
| [0039](0039-layout-never-reads-the-box-it-wrote.md) | UI Layout — The Walk Never Reads The Box It Wrote, And Auto Is The Room A Component Is Offered | accepted | 2026-09-07 |
| [0040](0040-a-key-goes-to-a-focused-component.md) | UI Input — A Key Goes To A Focused Component, And A Character Is Not A Key | accepted | 2026-09-07 |
| [0041](0041-a-document-is-written-whole-or-not-at-all.md) | Writing A File — A Document Is Written Whole Or Not At All, And Readably | accepted | 2026-09-07 |
| [0042](0042-a-textured-quad-in-world-space.md) | 2D Drawing — A Textured Quad In World Space Is A Third Primitive, Ordered By Its Caller | accepted | 2026-09-07 |
