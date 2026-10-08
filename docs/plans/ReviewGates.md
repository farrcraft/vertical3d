# Review Gates — A Changeset Reviews Clean

Drafted 2026-10-06, after eleven review rounds of `feat/motion-and-queries`. The goal is that
every changeset gets a review with no new finding. The review rounds did not converge on that.
Each round found about thirty findings and, after the fifth, about one major. A growing share
came from the fixes themselves: findings in text the fix added, and guarantees a fix wrote down
without a test of them.

[completed/ReviewFixes.md](completed/ReviewFixes.md) records the rounds. The plan rests on three
patterns read from them:

- **The same few shapes recur.** About a third of all findings were boundary input, prose, weak
  tests or build conventions, which a tool can check. [Review.md](../contributing/Review.md) names
  the classes.
- **A fix corrected one site and missed its siblings.** The string check reached six loaders in
  six rounds; the NaN check reached one overload of three; the disabled check reached two paths of
  four.
- **Nothing said which defects belonged to the change.** Reviewers reading whole files reported
  old defects, so no change to an old file could come back clean.

## Decisions

| ADR | Decision | Step |
|---|---|---|
| **0083** | A changeset answers for what it introduces; older defects go in the known-debt list | 1 |

## What blocks what

- Step 1 comes first, because every later step refers to the classes and the rule.
- Step 2 needs step 1's definition to know what is debt and what is a finding.
- Step 3 fixes the two findings that cannot wait for the gates.
- Steps 4 to 7 are the gates. They are independent of each other. Each one, once it runs clean,
  removes its class from what reviewers report.
- Step 8 needs the gates, because the branch's closing review should be the first one run under
  them.

## Steps

### Step 1 — The rule and the classes

**Closed.** [Review.md](../contributing/Review.md) states what a finding is, what a reviewer reads,
what a finding records, the classes, and what a fix has to do. ADR-0083 records why.

- Update the `cpp-reviewer` agent to classify each finding, say whether the change introduced it,
  list sibling sites, and end with what it read.
- Route reviews through Review.md from `CLAUDE.md` and [sdlc.md](../sdlc.md).

### Step 2 — The known-debt list

**Closed.** The eleventh round's open findings are entries under "Known review findings" in
[TODO.md](../TODO.md), each with its class. Each was older than the round that found it, or is in
text the gates will check.

### Step 3 — The two findings that cannot wait

**Closed.** Checking every sibling site, as Review.md asks, found two more defects of the same
pattern. A full-screen source released twice handed its descriptor set back twice. A descriptor
pool was destroyed under frames still using it. Both are fixed with tests that failed
first. `RiFormat` was the sibling of the pixel filter check, and refuses a resolution that cannot
be made.

- A mesh released after its draw items are queued keeps its material until the frame is done, not
  only its buffers and images. The test uses a textured mesh.
- A pixel filter width given through the C interface is checked as the RIB reader checks it, so a
  width that is not a positive finite number cannot reach `Film`.

### Step 4 — Gate: prose

**Closed.** `scripts/prose.ts` checks the lines a change adds to Markdown files and to the comments
of C++, GLSL, CMake, batch, YAML, TypeScript and JavaScript files. It reports sentences over 35
words, the banned openers, "which is what" and its kin, personified code and history. It also
reports a Markdown line or a comment line on its own over 100 columns, and a comment line
extended past the wrap of its paragraph. It runs in CI in `review-gates.yml` and fails the run.

### Step 5 — Gate: build conventions

**Closed.** `scripts/linkrule.ts` checks every app, app library and test suite: each names the
`v3dlib_*` targets whose headers it includes and no others, and includes the header of every
library whose namespace it uses. It also checks link visibility and third-party packages. It
runs in CI in `review-gates.yml`. The configure checks the api libraries.

### Step 6 — Gate: boundary input

**Closed.** `toCount` and `toInteger` in `api/type/Checked.h` convert a float to a count or an
integer, and refuse a value that is not finite or does not fit. `api/asset/Json.h` has reads that
check a JSON value's type. `scripts/boundary.ts` checks a changeset's added lines, outside the
helpers and the test files, for any `value_to` and for a `static_cast` to an integer type whose
operand the text shows to be a float. That second check reads text, not types. It runs
in CI in `review-gates.yml`. The branch's guarded JSON reads and its RIB counts, sizes, frame
numbers and handles go through the helpers.

### Step 7 — Gate: tests that fail first

**Closed.** `scripts/failsfirst.ts` finds the test cases a changeset adds, and builds their
suites in a separate worktree with the changeset's non-test files at their base version. It runs
each new case there, and fails when any of them passes. It runs locally before review, and in
`ctest.yml` on each pull request, after that job's build and on its software Vulkan driver.

### Step 8 — Close the branch

Run every gate on `feat/motion-and-queries` and fix what they find that the branch introduced.
Then run one review of the commits since the eleventh round, against Review.md. The branch merges
when that review is clean.

## State

Steps 1 to 7 are closed. The four gates are `prose.ts`, `linkrule.ts`, `boundary.ts` and
`failsfirst.ts` in `scripts/`, run by Node with no dependencies. All four read C++ through one
lexer, `scripts/lexer.ts`, and the tests in `scripts/tests/` cover it and
each gate's entry points. The tests, the link rule, prose and boundary gates run in CI in
`review-gates.yml`, and the fails-first gate runs in `ctest.yml`. A line the boundary gate
reports and that is defined carries a `// checked:` comment naming why.

Step 8's review is closed. The branch passes the gates: 317 prose lines were rewritten, and the
boundary gate's reports were fixed or given a reason. Fixing them found three more defects: a NaN
filter width in `Film::add`, a bound that is not a number in moya's buckets, and a GPU time that
wraps in voxel.

The closing review of the commits since the eleventh round found 17 findings, one of them Major
in the fails-first harness. They are fixed, and the harness's first full runs pass: every new
test fails or does not build without its change, and the one that passes says why. The review of
that fix commit found 8 Minor findings, in fallback paths and in text written with the fixes.
The review of their fix found 4 Minor findings, all in wording, and its fix left two lexer gaps
that four separate lexers kept reopening. The gates now share one lexer and have tests of their
own. The review of that change found one Major: a gate ran nothing on Node 24.0 and 24.1. The
review of its fix found one more: a gate started through a symbolic link ran nothing. Both are
fixed with tests that start each gate as a process.

Each review of the gates found one more fault in a less common setup. The gates now support one
environment, which Linting.md states, and a fault reachable only outside it is Minor. The review
of the last two commits under that rule found no Major and 5 Minor findings, in the documents,
two comments and one test. They are fixed. What remains of step 8 is the merge.

The review of that fix found a two-line comment whose second line ran to 114 columns. The
extended-line rule needs three lines, so the prose gate did not see it. The gate now also holds a
comment line on its own to 100 columns, and the 10 such lines the branch added are rewrapped.
