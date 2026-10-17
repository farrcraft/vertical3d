# Tooling Debt — The Disabled Checks and the Test Gaps

Drafted 2026-10-16. This plan clears the thirteen clang-tidy checks that are disabled until their
sites are fixed, and closes the test gaps that need no new hardware.
[TODO.md](../TODO.md#the-clang-tidy-backlog) lists each check with its number of sites.

Each entry this plan takes stays in TODO.md until the step that fixes it lands, and is then
deleted there. A check's row is deleted when the check is enabled.

## Decisions

No step needs an ADR in advance. One question is left to the step that meets it, under
[Open questions](#open-questions).

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

**Open.** [TODO.md](../TODO.md#weak-test). `a_frame_begun_again_restarts_its_claims` claims on the
slot it began twice. Run `scripts/failsfirst.ts` to show it fails without the turn counter.

### Step 2 — The grade tolerance

**Open.** [TODO.md](../TODO.md#lit-scenes), "The grade tests allow a one-step tolerance". Read
the identity and inverting grade results from a CI run on a PR. If lavapipe is exact, reduce the
tolerance to zero. If it is not, say beside the case that lavapipe is the reason.

### Step 3 — A pipeline cache test

**Open.** [TODO.md](../TODO.md#tests), "Nothing asserts the pipeline cache". The test counts what
was compiled when the same pipeline is asked for twice and when two pipelines differ in one
field. It compares no picture.

### Step 4 — The glTF fixture generators in TypeScript

**Open.** [TODO.md](../TODO.md#tests), "The glTF fixture generators are Python".

- Port the five hand-written generators in `api/asset/tests/data/` to TypeScript run by Node.
- Each port writes a fixture byte for byte identical to the committed one.
- The Blender generator is the open question below.

### Steps 5 to 17 — One check each

**Open.** Each step follows the same procedure:

- Discover the sites with `.clang-tidy` narrowed to the one check and warnings not treated as
  errors. [Linting.md](../contributing/Linting.md#clang-tidy) describes how to turn clang-tidy on.
- Fix every site. Never widen an exclusion instead.
- Remove the check from `.clang-tidy`'s disabled list and delete its row in TODO.md.
- Verify with the real `.clang-tidy` and warnings as errors, then run `ctest`.
- Update the enabled-check count in [Linting.md](../contributing/Linting.md),
  [TODO.md](../TODO.md#the-clang-tidy-backlog) and `CLAUDE.md`.

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

TODO.md holds the current counts.

- **Step 9** changes function signatures. A fix that reorders parameters changes every caller,
  including callers outside this tree. Prefer a distinct type for one of the swappable
  parameters.
- **Step 13** makes members private. A struct with only public data members is exempted through
  the check's options, which is not an exclusion.
- **Step 16** is mechanical, and clang-tidy's own fix applies it.

### Step 18 — Close the plan

**Open.** Confirm every check is enabled and every entry this plan took is deleted from TODO.md,
write the Outcome section, and move this plan to [completed/](completed/).

## Open questions

- **The Blender fixture generator.** `make_blender_fixture.py` runs inside Blender, which scripts
  only in Python. Either keep it as the one Python file in the tree, or delete it and keep only the
  fixture it wrote. Recommendation: keep it, because a fixture that cannot be regenerated cannot be
  changed.
