# Tooling Debt — The Disabled Checks and the Test Gaps

Drafted 2026-10-16. This plan clears the thirteen clang-tidy checks that are disabled until their
sites are fixed, and closes the test gaps that need no new hardware.
The table under steps 5 to 17 lists each check with the number of sites it was expected to report.

Each entry this plan takes stays in TODO.md until the step that fixes it lands, and is then
deleted there. A check's row is deleted when the check is enabled.

## Decisions

No step needs an ADR in advance. The one open question, the Blender fixture generator, closed in
step 4.

## What blocks what

- Steps 1 to 4 are the test gaps. They are independent of each other and of the checks.
- Steps 5 to 17 are the checks, one step each, smallest first. A small check finishes in one
  session, and each finished check stays enabled from then on.
- **Fixing one check can add sites to another.** Making a variable `const` can expose a narrowing,
  and a renamed variable can change a magic-number site. When a step adds sites to a later row,
  it updates that row's count.
- `misc-include-cleaner` goes last. Removing an include can break a file that relied on it
  through another, so it is the step most likely to touch every library at once.

## Steps

### Step 1 — The weak frame test

**Done.** `a_frame_begun_again_restarts_its_claims` now begins the same slot three times and
claims before each begin. With `turns()` put back to counting only first begins, it fails with
two pairs held where one is expected. `failsfirst.ts` was not the tool for this: it finds new
cases, and this one existed before the change, so the check was made by hand.

### Step 2 — The grade tolerance

**Open.** [TODO.md](../TODO.md#lit-scenes), "The grade tests allow a one-step tolerance". Read
the identity and inverting grade results from a CI run on a PR. If lavapipe is exact, reduce the
tolerance to zero. If it is not, say beside the case that lavapipe is the reason.

### Step 3 — A pipeline cache test

**Done.** `the_same_pipeline_is_compiled_into_the_cache_once` reads the cache's data size after
each build. The field that differs is the vertex stage, through a second test shader,
`flat.vert`. On the authoring GPU, a different cull mode, depth compare, topology or colour
format also added an entry. Whether lavapipe keys on fixed function state is up to it, so the
case does not rely on that. The case fails when `build()` passes no cache. It has not yet run on
lavapipe, so the first CI run on a PR checks it, alongside step 2.

### Step 4 — The glTF fixture generators in TypeScript

**Done.** The five hand-written generators are TypeScript, sharing `gltf_writer.ts`. Each writes
its fixtures byte for byte identical to the committed ones, which the Python generators also
reproduced before they were deleted. Python writes a whole float as `1.0` and JavaScript as `1`,
so the writer marks which numbers are floats and writes them through `JSON.rawJSON`. The Blender
generator stays, as the open question recommended. [Testing.md](../contributing/Testing.md)
names it as the one Python file in the tree.

### Steps 5 to 17 — One check each

**Done.**

- Steps 5 to 8 shared one discovery pass and one verification pass. Several files carried sites
  of more than one check.
- They found 51, 89, 31 and 435 sites, not the counts in the table. The table's counts for steps
  9 to 17 are likely off by as much, so a step's own discovery pass gives its real count.
- clang-tidy's own fix applied step 8. `run-clang-tidy -fix` over a compile database exported
  from `out/build/tidy` applies one check's fixes without rebuilding.
- clang-tidy does not fix a site inside a macro argument, such as a `BOOST_CHECK_EQUAL`. The
  verification pass finds those, and they are fixed by hand.
- Step 9 found 251 sites. Every one was fixed with a type, as decided when the step began.
  The RenderMan entry points keep the standard's signatures and carry a NOLINTNEXTLINE each,
  because cpplint cannot read a NOLINTBEGIN block that names another tool's check.
- The usual fix is a struct of named fields, built at the call with designated initialisers.
  A pair that is one value takes a glm vector, and an output pair becomes a returned struct.
  Vulkan's own structs serve the pipeline builder, and audio's Voice became a scoped enum.
- Signatures across api/ changed, so a consumer outside the tree has to follow them.
- Step 10 found 76 distinct enums. The table's 303 counted each header once per source that
  included it. Each enum took the base clang-tidy suggested, 72 of them `std::uint8_t`.
  clang-tidy has no fix for this check, so a script applied it.
- The check also reports a named base that is wider than needed. `Feature` and `DeviceType`
  narrowed to `std::uint8_t`. `audio::Voice` is an id whose values come from a counter rather
  than from its one enumerator, so it keeps `uint32_t` under a NOLINTNEXTLINE.
- No enum was in a GPU-visible struct, written as raw bytes, or declared ahead of its
  definition, so the smaller size changed no layout.
- Boost.Test streams a one-byte unscoped enum as a character. The 35 assertions that would
  have printed one now compare without printing their operands, and
  [Testing.md](../contributing/Testing.md#traps) records the trap.
- Step 11 found 1755 sites. 1740 were the structs that `BOOST_AUTO_TEST_CASE` declares, one
  per case. Each of the 218 test files now puts everything after its includes in one anonymous
  namespace. 169 of them already had one for their helpers, which now runs to the end of the
  file. The other 15 sites were helpers in five sources, now in anonymous namespaces too.
  clang-tidy's fix makes a definition `static`, which the tree does not use, so a script
  applied this one.
- Step 12 found 1313 sites. clang-tidy's fix applied them, and a script then moved each
  `const` in front of its type, since the fix writes `float const x`. The fix run also
  applied clang's own fixes, such as reordered member initialisers, and those were dropped.
  Where two sources reached one header it inserted `const` twice, and on an array of pointers
  it made the pointee `const` a second time. Those 25 lines and one guard object declared
  with its struct were fixed by hand.
- Step 13 found 12 sites once a class whose data members are all public was exempted, which
  `.clang-tidy` now does through the check's option. Ten were protected members of api base
  classes: the loggers of `asset::Loader`, `image::Reader` and `image::Writer`, the context and
  dispatcher of `input::Device`, the registries of `ecs::System` and `engine::Engine`, and the
  name and type of `asset::Asset`. Each is private behind a protected accessor of the same name,
  and `Asset` already had public ones. `Engine::registry()` returns the registry writable, so an
  app still owns its entities as before. The other two were test doubles that mixed public and
  private data.
- A class outside the tree that derives from one of those bases has to call the accessor.
- Step 14 found 6069 sites, 5398 of them in tests. A test's literal is the value it expects, so
  each tests directory has a `.clang-tidy` that turns the check off there. The check's options
  ignore 24, 255, powers of two, 0.5 and 2.0, which left about 390 sites in libraries and apps.
  Each became a named constant. Where several numbers are one value, they are one constant: a
  colour, the ui's shared palette in `ui/paint/Palette.h`, a coefficient table. A count is
  derived with `std::size` or `sizeof` instead of written. The shared offline renderer
  constants are in `render/offline/Units.h`. `RtBound` keeps the standard's `[6]` under a
  NOLINTNEXTLINE.
- Step 15 found 1529 sites, 468 of them in tests. The check's options set the minimum to two
  letters and allow the conventional single letters: `x`, `y`, `z`, `w`, `u`, `v`, `s` and `t`,
  plus the defaults `i`, `j`, `k`, `n` and `e`. That left 391 sites, mostly `a`, `b`, `c`, `p`,
  `m`, `d`, `e` and `f`, and each was renamed by hand for what it holds.
- Step 16 found 13573 sites: 12712 float `f` suffixes, 860 `u` and one `ull`. The check's
  default skips a literal inside a macro argument, which would have left every Boost.Test
  assertion lowercase, so `.clang-tidy` turns that off. clang-tidy reports a macro argument's
  literal where it is written, so one script uppercased every reported suffix. The reference
  documents' examples and `examples/starter` follow the same spelling.
- Step 17 found 5754 missing and 780 unused includes. Most named a third party library's
  internal header, such as `glm/detail/type_vec3.hpp` for `glm::vec3`. The check reads no
  mapping from those to the public headers, so `.clang-tidy` ignores every header installed
  outside the tree, by location. That left 1468 insertions and 89 removals in the tree's own
  headers and the standard library's.
- clang-tidy's fix writes the tree's headers in quotes, so a script applied them instead, in
  the include order cpplint requires. `Main.h` exports `Application.h`, and the two sources
  that only compile their header keep it with a pragma.
- No removed include was needed, and the tree built at the first pass.

Each step follows the same procedure:

- Discover the sites with `.clang-tidy` narrowed to the one check and warnings not treated as
  errors. [Linting.md](../contributing/Linting.md#clang-tidy) describes how to turn clang-tidy on.
- Fix every site. Never widen an exclusion instead.
- Remove the check from `.clang-tidy`'s disabled list and delete its row in TODO.md.
- Verify with the real `.clang-tidy` and warnings as errors, then run `ctest`.
- Update the enabled-check count in [Linting.md](../contributing/Linting.md),
  TODO.md and `CLAUDE.md`.

| Step | Check | Sites |
|---|---|---|
| 5 | `readability-implicit-bool-conversion` | 69 |
| 6 | `bugprone-narrowing-conversions` | 111 |
| 7 | `readability-braces-around-statements` | 111 |
| 8 | `readability-math-missing-parentheses` | 131 |
| 9 | `bugprone-easily-swappable-parameters` | 233 |
| 10 | `performance-enum-size` | 303 |
| 11 | `misc-use-internal-linkage` | 526 |
| 12 | `misc-const-correctness` | 939 |
| 13 | `misc-non-private-member-variables-in-classes` | 1303 |
| 14 | `readability-magic-numbers` | 1883 |
| 15 | `readability-identifier-length` | 2483 |
| 16 | `readability-uppercase-literal-suffix` | 4156 |
| 17 | `misc-include-cleaner` | 3346 |


- **Step 9** changes function signatures. A fix that reorders parameters changes every caller,
  including callers outside this tree. Prefer a distinct type for one of the swappable
  parameters.
- **Step 13** makes members private. A struct with only public data members is exempted through
  the check's options, which is not an exclusion.
- **Step 16** is mechanical. A script uppercases each suffix the check reports.

### Step 18 — Close the plan

**Open.** Confirm every check is enabled and every entry this plan took is deleted from TODO.md,
write the Outcome section, and move this plan to [completed/](completed/).

## Open questions

- **The Blender fixture generator.** `make_blender_fixture.py` runs inside Blender, which scripts
  only in Python. Either keep it as the one Python file in the tree, or delete it and keep only the
  fixture it wrote. **Closed in step 4: kept**, because a fixture that cannot be regenerated cannot
  be changed.
