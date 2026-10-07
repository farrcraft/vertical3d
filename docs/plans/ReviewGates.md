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

A script checks every comment and document a change touches for the writing rules. It reports
sentences over 35 words, the banned openers, and "which is what" and its kin. It also reports a
line extended past the wrap of its paragraph. It runs in CI beside cpplint and fails the build.

### Step 5 — Gate: build conventions

The link-rule check from the eleventh round becomes a script in CI: each app and suite names the
`v3dlib_*` targets whose headers it includes, and includes the header of every library whose type
it uses.

### Step 6 — Gate: boundary input

- Checked conversions in `api/type`: a float to a count or an index, refusing a value that is not
  finite or does not fit.
- Checked reads in `api/asset` for a JSON value of the expected type.
- A CI check that fails on a bare `static_cast` from a float to an integer type, and on a
  `value_to` that no type test guards, outside the helpers.

Built: `toCount` and `toInteger` in `api/type/Checked.h`, the reads in `api/asset/Json.h`, and
`scripts/boundary.ts`, which checks a changeset's added lines. The branch's guarded JSON reads
and its RIB counts, sizes, frame numbers and handles go through the helpers. Open: the CI step
that runs the script, and the reports it still makes on the branch, which step 8 settles.

### Step 7 — Gate: tests that fail first

A script that reverts a change's non-test files, builds, runs the tests the change added, and fails
when any of them passes. It runs on a branch before review.

### Step 8 — Close the branch

Run every gate on `feat/motion-and-queries` and fix what they find that the branch introduced.
Then run one review of the commits since the eleventh round, against Review.md. The branch merges
when that review is clean.

## State

Steps 1 to 7 are closed. The gates are `scripts/*.ts`, run by Node with no dependencies, and
the link rule, prose and boundary gates run in CI in `review-gates.yml`. A line the boundary
gate reports and that is defined carries a `// checked:` comment naming why.

Step 8 is under way. The branch passes all three CI gates: 317 prose lines were rewritten, and
the boundary gate's reports were fixed or given a reason. Fixing them found three more defects:
a NaN filter width in `Film::add`, a bound that is not a number in moya's buckets, and a GPU
time that wraps in voxel. What is left is the first full run of `failsfirst.ts`, and one review
of the commits since the eleventh round.
