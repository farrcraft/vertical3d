# Release Hardening — The Loose Ends a Player Would Meet

Drafted 2026-10-16. This plan fixes the defects in [TODO.md](../TODO.md) that a player of a
released game could meet, and the review findings already recorded as known debt. None of them
waits on a feature. Each is small, and most are independent of the others.

Each entry this plan takes stays in TODO.md until the step that fixes it lands, and is then
deleted there. The known review findings stay listed because a reviewer reads that list first, as
[Review.md](../contributing/Review.md) says. Each step fixes its defects with a test that fails
first, as Review.md requires, or records why no such test can be written.

## Decisions

| ADR | Decision | Step |
|---|---|---|
| [0089](../adr/0089-paths-narrow-strings-are-utf-8-under-a-utf-8-code-page.md) | Paths: narrow strings are UTF-8, under a UTF-8 code page | 1 |
| [0090](../adr/0090-input-commands-are-handled-through-one-registry.md) | Input: commands are handled through one registry | 3 |
| **New** | Why SDL3 replaced SDL2, recorded after the fact | 7 |

## What blocks what

- Step 1 is the only step with a decision, so it starts with the ADR.
- Step 4's `Loader` fix reads through `api/asset/Json.h`. Step 4's two config readers use the same
  helpers, so they land together.
- Every other step is independent and can land in any order.

## Steps

### Step 1 — Paths outside the ANSI code page

**Closed.** ADR-0089 keeps every narrow path in UTF-8 and has `v3dlib_log` carry a manifest that
sets the process code page to UTF-8, so every executable that links the logger reads a narrow
path as UTF-8. No signature changed. `a_path_outside_the_ansi_code_page_is_written_and_read` in
the engine suite runs `run<T>` from a directory named in Japanese: the log is written there and
an asset is found there. It failed on both checks before the manifest.

### Step 2 — A minimised window waits

**Closed.** While the window is minimised, `eventLoop()` waits for the next event and neither ticks,
simulates nor renders. The swapchain logs that the window has no area only when that changes.
`engine_waits_while_the_window_is_minimized_test` minimises a windowless engine with a pushed
event and restores it from a timer. Pong minimised for five seconds wrote one log line and used no
CPU, where before two seconds wrote twelve thousand lines.

### Step 3 — Bindings to commands nothing handles

**Closed.** No list of handled commands existed to test the bindings against, so the step took a
decision first. ADR-0090 moves the editor's command directory into the api as `event::Commands`.
The engine owns one and runs the handler each command names, and every app registers its commands
in its constructor, including the ones it reads with `held()`. The game menu registers its own
through `GameMenu::declare()`.

- pong's and tetris's F1 send `ui::fullscreen`, which the engine handles. pong's escape sends
  `ui::showGameMenu`, as tetris's and voxel's do.
  `Window::fullscreen()` does the switch, and a windowed case in the render suite switches a
  window to full screen and back with the frame loop drawing clean through both.
- odyssey's Space binding is removed, because odyssey moves on a flat grid.
- Each game's suite builds its engine without a window and checks `data/mappings.json` with
  `Engine::unhandled(document)`. Put back, the old bindings fail pong's, tetris's and odyssey's.
- After `start()` the engine logs every binding that nothing handles.

### Step 4 — Boundary input

**Closed.** Each fix has a test that fails without it.

- `Camera::zoom` stops at `Camera::MINIMUM_ZOOM`. The sibling site, the camera profile reader,
  refuses a zoom below it.
- `rib::Arguments` reads an `integer` parameter through `const int*`. It was the only reader of
  the C interface's value arrays.
- `Polygon::clip` and `Polygon::split` share `meeting()`, which takes the end nearer the plane
  when `intersectEdge` finds no meeting on an edge that `classify` puts across it. The test edge
  came from a search over random planes with the tree's own `Plane`. Before the fix the clip
  put a vertex 5.5 units from the edge, and the split hit its assert.
- The ui `Loader` reads every member through checked reads. A required member that is missing or
  of the wrong type fails the load, and an optional one is logged and ignored. `doc.at()` on
  `themes`, `containers`, `components`, `items`, `menus` and `buttons` threw on a missing key
  as well, and is checked the same way. Before the fix, 32 checks in the new cases failed.
- `asset::readInt` reads an integer an `int` can hold. The binding param and the window size go
  through it, and the window size moved into `config::windowSize()` so a test reaches it
  without a window.

### Step 5 — Contracts

**Closed.** Each fix has a test that fails without it. A sixth entry, pong's `bounce` clip, joined
the section after the plan was drafted and is fixed here too.

- `Keys::press` takes the key's `repeat` flag, and a repeat of `return` or `space` is taken and
  does nothing.
- The centred menu and the dropped panel share one colour rule for an item, `itemInk()`.
- `style::Overlay` records what a theme wrote over a dressing, and the next theme puts it back
  first. The immediate layer's theme had the same defect and uses it too.
- `ViewLayout` names a profile's second view "Perspective 2" and so on, and each view's pass is
  named after the view.
- `Playback::advance` runs the fade when the clip is `none`.
- pong's sounds document files `bounce` under `hit.wav`. pong has never had a sound of its own
  for a wall. A scene test plays a hit, a bounce, a point and a win, and checks the document holds
  every clip it heard.

### Step 6 — Cleanup on failure

**Open.** [TODO.md](../TODO.md#cleanup): the direct allocator's leak when no memory type suits a
buffer or image, and `Presenter`'s semaphores when its constructor throws part way. Both are fixed
by owning each handle as soon as it is made.

### Step 7 — Drift

**Open.** [TODO.md](../TODO.md#drift), and the missing SDL3 record under
[Documentation](../TODO.md#documentation).

- Correct each stale comment and document sentence the drift entry lists.
- Write the ADR recording why SDL3 replaced SDL2, and add it to the index.

### Step 8 — Close the plan

**Open.** Confirm every entry this plan took is deleted from TODO.md, write the Outcome section, and
move this plan to [completed/](completed/).
