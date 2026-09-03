# Architecture Decision Records

Why the codebase is shaped the way it is. A decision earns an ADR when it is hard to
reverse, constrains later phases, or a future reader would otherwise ask "why on earth is
it done this way". Anything smaller belongs in a comment beside the thing it explains.

Copy [template.md](template.md) and add a row below. Superseding does not delete: set the
old record's status and leave the file in place.

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
