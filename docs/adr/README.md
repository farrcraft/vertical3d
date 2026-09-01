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
| [0008](0008-binding-by-update-frequency.md) | Shader Bindings — Descriptor Sets By Update Frequency, Per-Object Data In Push Constants | proposed | 2026-08-31 |
| [0009](0009-colour-authored-in-display-space.md) | Colour — Authored In Display Space, Presented Through A UNORM Swapchain | accepted | 2026-08-31 |
| [0010](0010-meshes-are-owned-by-the-app.md) | Geometry Ownership — Meshes Belong To The App, Not To Resources | accepted | 2026-08-31 |
