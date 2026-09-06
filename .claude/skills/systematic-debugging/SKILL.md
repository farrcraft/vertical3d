---
name: systematic-debugging
description: Root-cause a bug, a build failure, a crash, or unexpected rendering before proposing any fix. Use on any defect, assertion failure, Vulkan validation message, or "it used to work" — including when the first fix did not hold. Covers the four-phase method and the tools this project actually has for isolating a cause.
license: MIT
origin: Adapted from obra/superpowers (Jesse Vincent), via Jeffallan/claude-skills; retuned for vertical3d
---

# Systematic Debugging

> **No fix without a root cause first.**

Jumping to a fix without understanding the cause is how one bug becomes three. The symptom
you can see is rarely the defect; it is the first place the defect became visible.

Two rules that override the urge to get moving:

- **A fix you cannot explain is not a fix.** If you cannot say in one sentence why the
  change makes the symptom impossible, you have found a correlation, not a cause.
- **Never make the symptom invisible.** Widening a cpplint filter, adding a
  `#pragma warning(disable:)`, catching and dropping an exception, or removing a file from a
  target are all ways of turning a bug into a bug you can no longer see.

**Be honest about the toolkit.** This project has no sanitizer build and no static analysis
beyond cpplint. What it does have is a Boost.Test suite behind ctest and the Khronos
validation layer, whose messages `vulkan::Instance` routes through the logger. Neither
reaches a rendering defect below the recorder, so disciplined reading and bisection still
matter more here than in a repo where you can throw tools at the problem.

---

## Phase 1 — Investigate

**Goal: know exactly what fails, and where, before forming any theory.**

- **Read the whole error.** MSVC prints the error and then the instantiation chain that
  caused it, and the useful half is usually not the first line. Nothing is logged to a file
  by default, so redirect and read the file rather than the console tail:
  `ninja -C out/build/x64-Debug > build.log 2>&1`.
- **Check whether it is the environment rather than the code.** A stale CMake cache, a
  missing `VULKAN_SDK`, an unbuilt libnoise. `docs/Build.md` has each of them, and the tree
  is otherwise clean at every gate `docs/Linting.md` lists.
- **Reproduce it deliberately** and write the steps down. For an app, that means which app,
  which `data/` config, and what you did.
- **Narrow it.** Build one target rather than the tree. For a compile error, `cl /Zs` on a
  single translation unit iterates in seconds where a full build takes minutes — pass the
  same include paths and `/std:c++latest /permissive- /utf-8 /EHsc` the build uses.
- **Look at what changed.** `git diff`, `git log -p -- <file>`, and `git bisect` when there
  is a clean pass/fail command. `ctest --test-dir out/build/x64-Debug -R <suite>` is one
  where the defect has a cpu half; otherwise it is "does this target build" or "does the app
  get past initialisation".
- **Trace backwards from the bad value**, not forwards from the entry point. Ask where the
  value was last correct.

**Do not skip to Phase 3 because the cause seems obvious.** If it were obvious, the bug
would not exist.

---

## Phase 2 — Compare against something that works

Most defects here are a divergence from a pattern the codebase already gets right.

- **pong is the reference app.** It is the only one that builds and the only one already on
  the `api/` framework, so for anything app-shaped — config, asset loading, engine
  initialisation, renderer construction — read how pong does it before theorising about why
  another app does not.
- Find the nearest working equivalent and read it **completely**, not just the line you
  expect to differ. List every difference, then justify each one. The bug is almost always
  in a difference you would have dismissed.
- Check the governing ADR. A rule that looks arbitrary in code often has a reason recorded
  in `docs/adr/`, and violating it is a common cause of "works except in one case".

---

## Phase 3 — Hypothesise and test

- **Write the hypothesis down** as a falsifiable statement: "the extension list is queried
  but never passed to `vkCreateInstance`, so no surface extension is enabled and surface
  creation cannot succeed."
- **Change one variable at a time.** Two changes that jointly fix a symptom tell you nothing
  about which mattered.
- **Add a temporary probe rather than guessing.** spdlog is already wired everywhere:
  `logger_->get()->info("... {}", value)`. A probe that answers the question and is then
  removed beats a session of staring.
- **Try to disprove it.** If the hypothesis is right there is usually a second prediction it
  makes; check that one too.
- If two hypotheses both survive, you have not narrowed enough — go back to Phase 1.

### If the first fix did not hold

Stop patching. That is the signal that the model of the system is wrong, not that the fix
was slightly off. Return to Phase 1 and re-derive the cause from the new evidence — the
failed fix is itself data about what the system is really doing.

---

## Phase 4 — Fix and prove

1. **Fix the root cause**, not the symptom, and keep the change minimal.
2. **Verify with the command that failed**, and quote what it said. Building the target that
   broke is usually the whole of the available proof; say so plainly rather than implying
   more. If the fix is in a library, build a consumer of it too.
3. **Run cpplint on the files you touched.** The tree is clean at that command, so
   anything it reports is yours.
4. **State what you did not verify.** Nothing renders yet, so no change under `api/render`
   can be proven to draw correctly. That is a limitation to name, not to paper over.
5. **Record what you found but did not fix.** Loose ends live in `docs/TODO.md` or the open
   plan; a decision that came out of the investigation belongs in `docs/adr/`. A silent
   workaround is the one unacceptable outcome.

---

## What the symptom usually means here

| Symptom | Look first at |
|---|---|
| Nothing renders | **Expected right now.** The Vulkan frame loop does not exist, `Engine3D::renderFrame()` still calls `glClear`, and no GL context is created any more. Not a bug to chase. |
| App exits immediately, no window | `Config::load` rejected `data/config.json`. It requires the indirect `{"configs": [{"type", "file"}]}` form; tetris still has the old inline `keys`/`menu` format, which fails and makes `Engine::initialize` return false. |
| `vkCreateInstance` fails | An extension the loader does not advertise — `Instance::requireExtensions` names the missing one before creation is attempted. |
| "No physical vulkan device supports 1.3" | The driver reports below 1.3, which device selection rejects per ADR-0002. Support tracks driver version more than GPU age; update the driver before suspecting the code. |
| `LNK2019` on `vk*` symbols | The executable links `v3dlib_render` but not `${Vulkan_LIBRARIES}`. |
| `LNK2019` on something that obviously exists | The `.cpp`/`.cxx` is missing from its `CMakeLists.txt`. Every source list in this repo is hand-written. |
| `static_assert` failure inside spdlog's bundled fmt | `/utf-8` missing from that target. The assert is real. |
| `C2259` cannot instantiate abstract class | A pure virtual not overridden because the signature differs rather than being absent — this is the `Operation::run(Context)` versus `run(Context2D)` bug. |
| Crash or corruption after an entity dies | An `entt` reference held across a call that can `destroy()` an entity; storage compaction invalidates it. Re-fetch after the call. |
| Use-after-free on a Vulkan handle | Destruction order. The surface must go before the instance and the window; swapchain images must not be destroyed while the queues may still read them. |
| A resource leaks when construction fails | A constructor that acquired a handle and then threw. Nothing runs the destructor of an object whose constructor threw — see `Swapchain`, which catches, destroys and rethrows. |
| A small edit shows as a whole-file diff | CRLF. `.gitattributes` enforces LF; strip the CRs rather than committing them. |
| `git status` reports files as modified that `git diff` says are identical | Stale index stat cache after a bulk edit. No refresh clears it; `rm .git/index && git reset` rebuilds it and leaves the working tree alone. |
| CMake will not configure after a Visual Studio update | The cached `CMAKE_CXX_COMPILER` points at an MSVC version that no longer exists. Delete `CMakeCache.txt`, `CMakeFiles/`, `build.ninja` and `.ninja_*` — **never** `vcpkg_installed/`. |

## Tools this project has

```bash
ninja -C out/build/x64-Debug <target>     # needs an MSVC Developer environment first
cl /Zs <flags> file.cxx                   # syntax-only, fast iteration on a compile error
cpplint --linelength=180 ... <files>      # the only static analysis here
git diff / git log -p / git bisect        # when there is a clean pass/fail command
```

Plus the Visual Studio debugger, and spdlog for probes.

**The Khronos validation layer is enabled when it is installed**, and `vulkan::Instance`
routes its warnings and errors through the logger — so for a Vulkan defect, read `v3d.log`
before anything else. A silent log means the layer found nothing, not that it is off; a run
with no layer installed is also silent, so confirm the instance logged that validation is on.

**There is no sanitizer build and no clang-tidy.** Do not reach for them, and do not claim a
fix is verified by them. The test suite is real — use it where the defect has a cpu half,
and say so where it does not.
