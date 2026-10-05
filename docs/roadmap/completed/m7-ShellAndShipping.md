# The Shell, Finished

Milestone 7 of [the game engine roadmap](GameEngine.md), **done by
[ShellAndShipping](../../plans/completed/ShellAndShipping.md)**, asynchronous loading aside. **A collection rather than a sequence**:
pieces of the app shell, the ui and the loop that consumers have each worked around, and the
things a game needs on the way to shipping that a demo never does. None of them blocks another
or waits on another milestone, and each is taken when a consumer reaches it. Where a consumer
has named its own trigger, that trigger is given.

The shell belongs to the api ([ADR-0028](../../adr/0028-apps-the-shared-app-shell-lives-in-the-api.md)), so
anything every game writes the same way is a candidate for here, and anything a game writes
differently is not.

## The renderer setup every app writes

pong, tetris, voxel and the editor each build the same objects in the same order: an
`Engine3D`, a `TextRenderer` handed an upload lambda, a `ComponentRenderer` over the text
renderer's measure and write, a `StatisticsOverlay`, and a check each frame that resizes the
canvas when the frame's size changed — which the starter in `examples/` and odyssey write too
([PongRenderer.cxx](../../../pong/src/PongRenderer.cxx#L32), [tetris](../../../tetris/src/Renderer.cxx#L65),
[voxel](../../../voxel/src/Renderer.cxx#L122), [the editor](../../../vertical3d/src/Renderer.cxx#L68)).

**The upload lambda is written four times on purpose**, and that stays. Putting a constructor
back on `TextRenderer` that builds it would put a Vulkan type back in an `api/ui` header, which
[EmbeddingSeams](../../plans/completed/EmbeddingSeams.md) removed. What can be shared is one level
up: a helper on the realtime side that builds the set and owns the resize check, which an app
calls once and which the four renderers then stop writing. odyssey is the one app with none of
it, and adopting the shell is its own change.

## A game space that is not the window

pong's court is 800 by 600 window pixels with a FIXME asking for variables
([PongScene.cxx:196](../../../pong/src/PongScene.cxx#L196)), tetris fits its well with a `Layout`
of its own, and odyssey picks and draws through a hard-coded tile width. Each is a 2D game asking
for its own coordinates mapped onto whatever the window is. An orthographic camera for a 2D pass
is that mapping, and is also the item in
[RenderingPipeline.md](../../api/rendering/README.md#what-is-not-built-yet) about the 2D pass not
reading set 0 — the two are one change.

## Widgets

Each is a gap a consumer has named:

* **A slider.** cozy's settings pages made volume a set of radio buttons because there is no
  `Slider` and a `Scrollbar` took no key then, and recorded four choices as the better control.
  A `Scrollbar` has taken keys since `44f89fa`.
* **A box that wraps.** `Box` neither wraps its children nor sizes itself from them, so cozy's
  inventory grid is built from rows in code. cozy's M5 plan has it deferred.
* **Strips that respect `pickable()`.** `Toolbar` and `MenuBar` pick before consulting it, so a
  strip in a HUD takes clicks meant for the world. cozy works around it by having nothing in its
  HUD pickable, and a test of its own holds it there.
* **A file chooser.** The editor has no "save as", per [Editor.md](../../Editor.md), and a chooser
  is what it is missing.

Each new component touches the places [ADR-0047](../../adr/0047-code-exhaustive-enum-switches.md)
lists, which is the cost of a widget here and worth knowing before taking on four.

## Input

* **A held binding.** A binding fires a command; walking needs a key that is down. cozy reads its
  walk keys as held by name through `Engine::keys()`, so they cannot be rebound, and its M5 plan
  names a keyboard layout that is not QWERTY as the trigger for fixing that.
* **A relative mouse mode.** voxel's mouselook warps the cursor to the centre of the window every
  frame ([Controller.cxx](../../../voxel/src/Controller.cxx#L51)) because the window offers no
  relative mode. SDL3 has one, and the window should expose it.

## Versioned documents

cozy's save is a whole JSON document carrying a version, walked forward by a chain of
migrations one version at a time, which refuses rather than reading a half-migrated document
(its ADR-0003, and `src/Save.h`). Nothing about the walk is cozy's: `engine::Settings` is a
versioned document as well, and every document a game writes outlives the build that wrote it.
`asset::writeDocument` already writes one whole or not at all
([ADR-0041](../../adr/0041-files-write-documents-atomically.md)); reading one forward is
the other half.

retcon is not the second consumer: its saves are Boost.Serialization, by its own design. The
second is `engine::Settings`, which is in this tree.

## Profiling

`engine::Statistics` measures frame pacing and `StatisticsOverlay` shows it; cozy has written a
frame-time graph of its own on top. What is missing is where the time goes: named cpu scopes and
gpu timestamp queries around a pass. retcon's phase 11 is performance, and it is the first
consumer that will need to know which pass is slow rather than that a frame was.

## Asynchronous loading

Nothing in `api/` starts a thread, and the uploader blocks. Every load today is on the main
thread, which is correct and is a hitch the size of the load. A region loaded while walking
toward it ([milestone 2](m2-LargeWorlds.md#regions)) is the first case where that shows, and the
first piece is decoding off the main thread with the upload kept on it, because the decode is
most of the time and the upload is what touches the device.

## Not in this milestone

* **Hot reload.** cozy has a file watcher, and considered sending it here and decided against it
  in its M3 plan. Its debug reload is also the clearest case of
  [milestone 2](m2-LargeWorlds.md#releasing-a-resource)'s leak.
* **What retcon has not adopted.** Bindings as data, the shell's menu and settings persistence
  are all here already; retcon taking them up in its phase 7 is retcon's work.
* **Crash reporting, packaging and macOS.** retcon's phase 11. MoltenVK is a platform this tree
  has never built for, and retcon's known issues already name the first thing it will hit: the
  api targets carry MSVC-only flags such as `/EHsc` on their interface, which reach every
  consumer and which clang reads as filenames. That is a defect to fix when a non-MSVC build is
  attempted, not a milestone.
