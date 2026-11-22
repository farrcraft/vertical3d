# CI Workflows — Runs That Fit, Checks by Event, and Audited Workflows

Drafted 2026-11-21 against `1ee4e5a8`. When this plan closes, every workflow fits its time limit
with room to spare, and a pull request learns of a failure in under an hour. A change is checked
on a pull request, on main, on a tag and on a schedule as one record decides. The workflows
themselves are audited for security on every change.

The repository is public and owned by a user, on GitHub's Free plan. Its standard runners cost
nothing, so the budget is time and the limits GitHub sets, not minutes:

* **Runners.** A Linux or a Windows runner has 4 cores, 16 GB of memory and 14 GB of disk. The arm64
  Linux and Windows runners are the same size and also free. Larger runners are billed for public
  repositories.
* **Limits.** 20 jobs run at once. A job runs for at most 6 hours, a matrix holds at most 256 jobs,
  and a concurrency group queues at most 100 runs.
* **The cache.** Each repository has 10 GB. An entry not read for 7 days is removed, and a full
  cache removes the least recently read first. A user's repository can buy more since 2025-11-20.
* **No merge queue.** A merge queue needs a repository owned by an organisation.

The plan follows three records and amends one:

* [ADR-0173](../adr/project/0173-testing-the-suites-also-run-under-the-sanitizers.md): the suites
  also run under the sanitizers, in the Linux job.
* [ADR-0174](../adr/project/0174-testing-the-hand-written-parsers-are-fuzzed.md): the hand-written
  parsers are fuzzed.
* [ADR-0083](../adr/project/0083-review-a-changeset-answers-for-what-it-introduces.md): a changeset
  answers for what it introduces. A check that runs only after a merge weakens this, and step 1
  weighs it.
* [ADR-0178](../adr/project/0178-releases-a-tag-on-main-starts-a-release.md): a tag on main starts a
  release. Step 3 decides what a release's build may reuse.

---

## What the survey found

**Three jobs pass their limits or come near them.** Measured on the runs of
[pull request 32](https://github.com/farrcraft/vertical3d/pull/32) from 2026-11-17 to 2026-11-20:

| Workflow | Job | Limit | Recent runs | Where the time goes |
|---|---|---|---|---|
| portable | clang | 120 min | cancelled at 120, twice | build 43 min; moya's suite under the sanitizers 75 min and unfinished; then an RGB build and the suite again, and the fuzzers |
| ctest | test (ci) | 120 min | 97 min, and cancelled at 120 | build 29 min; tests 47 min, nearly all of it moya's suite in Debug; fails-first 15 min; starter example 3 min |
| portable | whole (clang, gcc) | 240 min | 47 to 153 min | one step in a container, so the split is not reported |
| flatpak | build | 240 min | 9 to 93 min | its module cache, hit or missed |
| ctest | test (ci-release) | 120 min | 30 to 41 min | build 23 min; tests 4 min |
| codeql | analyze (c-cpp) | 60 min | 8 to 13 min | |
| cpplint, docs, review gates | | | under 1 min | |

`portable` has not passed on the branch since 2026-11-15. The clang job ran in 14 minutes on
2026-11-06, before the volumes plan and this plan added most of moya's suite. moya's suite takes
25 minutes in Debug on Linux without the sanitizers, and about three times that with them. The
clang job runs it twice.

**moya's suite is the cost.** It is one ctest test of one binary, so ctest cannot spread it over a
runner's cores. In Release the whole Windows test step takes 4 minutes; in Debug it takes 47. The
suite is run six times on each push: Debug and Release on Windows, Debug in each of `whole`'s two
jobs, and twice under the sanitizers in `clang`.

**The cache is full.** It holds 9.86 GB of its 10 GB. 2.6 GB of that is vcpkg archives keyed by
MSVC 14.51, which no build reads since the toolset moved to 14.52. The Flatpak's module cache is
2.96 GB on its own. A full cache evicts the archives still in use, and the next run builds them
cold.

**Every job runs on every pull request.** Each workflow but `docs` and the gates skips a change to
documents alone, and nothing else is filtered. The fuzzers, the second sanitized suite and the
Flatpak run on every push of a branch. CodeQL also runs weekly. Nothing else runs on a schedule.

**No check is required.** Branch protection on main requires a branch to be up to date before it
merges, and names no status check. A red run does not block a merge.

**The workflows are written carefully.** Every action is pinned to a commit, and
[pinned.ts](../../scripts/pinned.ts) checks it. Permissions are read-only but for the jobs that
publish. Dependabot waits seven days after a release. CodeQL reads the workflows. Four things an
audit tool would flag remain:

* **Checkout leaves its token on disk** in every job, since none sets `persist-credentials: false`.
* **Expressions expand into shell scripts** in `ctest.yml`, `review-gates.yml` and `portable.yml`.
  None takes text an outsider writes, but nothing stops one that does.
* **The Flatpak's container image is pinned by a tag**, not a digest.
* **A release restores caches** that runs on main wrote, and publishes what it builds from them.

---

## Decisions

| ADR | Decides | Step |
|---|---|---|
| 0257 | What runs on a pull request, on main, on a tag and on a schedule | [1](#step-1--the-record-what-runs-when) |
| 0258 | How the workflows are audited | [2](#step-2--the-record-auditing-the-workflows) |
| 0259 | What a release's build may restore from the cache | [3](#step-3--the-record-what-a-release-may-reuse) |

Each record step researches how established projects make the choice, and the owner chooses among
the options before the record is drafted.

---

## The steps

| | What | Where | ADR | State |
|---|---|---|---|---|
| [1](#step-1--the-record-what-runs-when) | The record: what runs when | `docs/adr` | **0257** | accepted |
| [2](#step-2--the-record-auditing-the-workflows) | The record: auditing the workflows | `docs/adr` | **0258** | accepted |
| [3](#step-3--the-record-what-a-release-may-reuse) | The record: what a release may reuse | `docs/adr` | **0259** | accepted |
| [4](#step-4--the-cache-within-its-size) | The cache within its size | `.github/workflows` | — | in progress |
| [5](#step-5--the-cost-of-moyas-suite) | The cost of moya's suite | `moya/tests` | — | open |
| [6](#step-6--the-linux-jobs-split) | The Linux jobs split | `.github/workflows/portable.yml`, `scripts/portable` | 0173, 0174 | open |
| [7](#step-7--the-windows-job-split) | The Windows job split | `.github/workflows/ctest.yml` | — | open |
| [8](#step-8--checks-by-event) | Checks by event | `.github/workflows` | 0257 | open |
| [9](#step-9--the-workflows-audited) | The workflows audited | `.github`, `.pre-commit-config.yaml` | 0258, 0259 | open |
| [10](#step-10--required-checks) | Required checks | the repository's settings | 0257 | open |
| [11](#step-11--the-documents) | The documents | `docs` | — | open |

## What blocks what

```
1 ADR ───────────────────────────────────────────┬──> 8 By event ──> 10 Required checks
4 Cache ──┐                                      │
5 Suite ──┼──> 6 Linux split ──┬─────────────────┘
          └──> 7 Windows split ┘
2 ADR ──┬──> 9 Audited
3 ADR ──┘
                                        (all) ──> 11 Documents
```

Steps 1 to 5 depend on nothing. Steps 6 and 7 need the cache within its size and the suite's
cost measured, since both change what a split saves. Step 8 needs the record of step 1 and the
splits, because it moves jobs that steps 6 and 7 make. Step 9 needs the records of steps 2 and
3. Step 10 names the checks step 8 leaves on a pull request. Step 11 checks that the documents
read as a whole.

## How every step is verified

* **A run on a pull request.** A step that changes a workflow is verified by the run of the pull
  request that carries it. Its jobs pass, and each job's time is recorded in the step's state note
  beside the time before.
* **Nothing is checked less without the record saying so.** Each suite, build and gate that ran
  on a pull request before the plan runs afterwards on the event step 1's record names. A check
  moved off pull requests is named in the record.
* **The gates and the analysers.** A step that changes C++ or TypeScript is clean at every gate,
  at `/W4 /WX`, at `/analyze` and at clang-tidy. A step that adds Boost.Test cases runs
  `failsfirst.ts` on them.
* **The audit.** Once step 9 lands, every workflow is clean at the audit at the severity step 2's
  record sets.

---

### Step 1 — The record: what runs when

**The question.** The events each check runs on: a pull request, a push to main, a release's tag
and a schedule. The answer decides how long a pull request waits, and which defects
can reach main before a check finds them.

**The options:**

* **Everything on every pull request, made faster.** No check moves. Steps 4 to 7 make the jobs fit
  by splitting them across runners. ADR-0083 holds in full. Each push of a branch costs about
  twelve jobs and the longest of them sets the wait. Godot runs its sanitized builds on every pull
  request this way.
* **Fast checks on a pull request, the long ones on main and nightly.** A pull request runs the
  build, the suites once per compiler, the gates and the analysers. A push to main adds the
  sanitized suites, the RGB build, the Flatpak and the fuzzers, and a nightly run repeats them on
  main's tip. LLVM tests a change on its build bots after it merges, and reverts one that breaks
  them. A defect the long checks find reaches main and is fixed there.
* **Fast checks on a pull request, the long ones by a label.** As the second, but a label on a pull
  request asks for the long checks before the merge, and a release always runs them. A change to
  moya or the build would carry the label by rule. Blender builds a pull request when a reviewer
  asks for it this way.

The record also settles where the fuzzers run and for how long, whether a change to documents
alone still builds, and whether an arm64 runner joins any tier. It settles what a nightly
failure does: whether it opens an issue, and who it blocks.

**Drafted** as
[ADR-0257](../adr/project/0257-ci-every-check-runs-on-every-pull-request.md): every check runs on
every pull request that changes more than documents, and again on main. A check that grows too long
is made faster or split across runners, never moved off pull requests. Fuzzing runs briefly on each
pull request and in long batches on a schedule, which amends ADR-0174.

### Step 2 — The record: auditing the workflows

**The question.** What audits the workflows for security, how it runs, and what a finding blocks.
CodeQL's `actions` language reads the workflows already, and `pinned.ts` checks pins.

**The options:**

* **zizmor**, a static analyser for GitHub Actions. Its audits cover tokens left on disk, template
  injection, dangerous triggers, excessive permissions, cache poisoning, unpinned actions and
  images, impostor commits and Dependabot's cooldown. It runs as a pre-commit hook and as an
  action that uploads its findings to code scanning. Its online audits need a token. Its pin
  audit would cover what `pinned.ts` checks.
* **actionlint**, which checks a workflow's syntax, its expressions and its shell scripts through
  shellcheck. It finds mistakes rather than security findings, and could run beside zizmor.
* **CodeQL's actions queries alone**, which run now. They find fewer classes, and cannot run
  locally before a commit.

The record also settles the persona and the least severity that fails, whether the findings go to
code scanning or fail the job, and whether `pinned.ts` stays once the audit checks pins.

**Drafted** as
[ADR-0258](../adr/project/0258-ci-workflows-are-audited-by-zizmor-and-actionlint.md): zizmor
audits the workflows and Dependabot's configuration for security, and actionlint checks the
workflows for correctness. Each runs in pre-commit and in a CI job that fails on a finding. zizmor's
audit of pins replaces `pinned.ts`, and CodeQL's queries for workflows go on running.

### Step 3 — The record: what a release may reuse

**The question.** A release's build restores vcpkg's archives, the Vulkan SDK, lavapipe and NSIS
from the cache, and the Flatpak its modules. A cache is written by any run on main, so whatever a
run on main can write ends up in what a release ships. The question is what a release may restore.

**The options:**

* **Nothing.** A release builds every dependency from source. It is the slowest, and a release is
  rare.
* **Only what main wrote, as now**, with the audit's finding answered in the record. A run on main
  only runs code that was reviewed into main.
* **Only archives a release can check**, such as a tool's installer checked against a pinned hash,
  while vcpkg's archives and the Flatpak's modules are built again.

**Drafted** as
[ADR-0259](../adr/project/0259-releases-a-release-build-restores-nothing-from-the-cache.md): a run
for a release's tag neither restores nor saves any cache, and builds every dependency from source.
A weekly run builds as a release does, so a dependency that stops building from source is found
before a release.

---

### Step 4 — The cache within its size

* **Archives no build can read are deleted**: the vcpkg archives of an old toolset, and the
  Flatpak's caches of an old manifest.
* **A save removes the entry it replaces.** Each key ends in the run's id, so each run that builds
  something stores a whole new copy. A save deletes the older entries under its prefix.
* **The Flatpak's module cache is measured** against what it saves, since it is 2.96 GB of the
  10 GB.

**Tests:** the cache's size after a week of runs, recorded with the hit rate of each restore.

**In progress.** `scripts/prunecache.ts` deletes, after each save of vcpkg's archives, every entry
on the run's ref under the save's prefix but those the run saved. It deletes nothing when the run's
own entry is not listed, so a save that did not land leaves the older ones. The Windows prefix holds
the toolset. On 2026-11-20 the Release job ran on a runner of MSVC 14.51 and the Debug job on one of
14.52. The images are mixed, so the archives of 14.51 are still read, and the survey's word that no
build reads them was wrong. Each toolset keeps its own until the old image is gone and the cache
evicts them. The Linux prefixes are the system alone, since their restore keys stop at the hash. The
jobs that save take `actions: write`. Two entries no restore could read, of old `vcpkg.json` files,
were deleted by hand on 2026-11-22: `vcpkg-Windows-14.51.36231-a82be54e…` and
`vcpkg-whole-clang-Linux-b43f69b7…`, 1.4 GB between them. The cache then held 7.97 GB of its 10. The
Flatpak's module cache stays: a run that restored it built in 10 minutes, and one that did not in
93. It is the only entry of its manifest, so none of it is stale. The step is verified once a week
of runs on a pull request has passed.

### Step 5 — The cost of moya's suite

* **Each case is timed** in Debug, in Release and under the sanitizers, on a runner's four cores.
* **The heaviest cases are made cheaper** where a smaller render, fewer samples or a smaller
  scene keeps what the case checks. A statistical case keeps its confidence.
* **The suite is split into ctest tests by its Boost.Test suites**, so ctest can run them at once
  and report each one's time. Each test's label says whether it renders.

**Tests:** each changed case still fails on the defect it was written for, shown by mutating the
code it checks. moya's suite gives the same results as before.

### Step 6 — The Linux jobs split

* **The RGB build is a job of its own**, beside the spectral one rather than after it.
* **The fuzzers are a job of their own**, which reads the sanitized build's fuzz targets from an
  artifact or builds them alone.
* **`whole` reports its stages** as steps, so the build and each suite have a time of their own.
* **Each job's time limit** is its measured time with room to spare.

**Tests:** a run where every Linux job passes inside its limit.

### Step 7 — The Windows job split

* **The fails-first harness is a job of its own**, beside the Debug job's tests rather than after
  them.
* **The starter example builds in a job of its own**, or stays where its setup is cheapest, as the
  measurements say.
* **The Debug job runs its suites at once**, with the split step 5 makes.

**Tests:** a run where both Windows jobs pass, and the Debug job is measured against 97 minutes.

### Step 8 — Checks by event

Under the tiers step 1 settles.

* **Every check runs on each pull request and each push to main**, as now, with the splits of
  steps 6 and 7.
* **A scheduled workflow fuzzes each target in long batches**, from the corpus the last batch
  kept, and builds as a release does with nothing restored from the cache.
* **A failure of a scheduled run** opens an issue, since no pull request is there to show it.

**Tests:** a run started by hand on the scheduled workflow fuzzes every target and builds with an
empty cache. A crash it finds opens an issue with the input.

### Step 9 — The workflows audited

Under the choices steps 2 and 3 settle.

* **zizmor and actionlint run in pre-commit and in a CI job**, each pinned, and the job fails on a
  finding. zizmor's online audits run in CI with the job's token.
* **Their findings are fixed**: tokens not left on disk, expressions passed to scripts through the
  environment, the container image pinned by digest, and every finding actionlint makes.
* **A release's run restores nothing**, and saves nothing, from the cache.
* **`pinned.ts` is deleted**, with its tests and its pre-commit hook, once zizmor checks pins.

**Tests:** both tools are clean over every workflow, and zizmor over `dependabot.yml`. A workflow
with a token left on disk fails the hook, and so does one that names a job `needs:` cannot find.

### Step 10 — Required checks

* **Branch protection on main names the checks** that must pass before a merge, from those step 8
  leaves on a pull request.

**Tests:** a pull request with a failing required check cannot merge.

### Step 11 — The documents

* **A new reference page, `docs/contributing/CI.md`**, says what runs on each event and how long
  each job takes, what each cache holds and what a release restores, and how the workflows are
  audited. The docs site's summary lists it.
* **[Gates.md](../contributing/Gates.md), [Testing.md](../contributing/Testing.md) and
  [Linting.md](../contributing/Linting.md)** link to it, and lose what it now states.
* **ADR-0173 and ADR-0174** are amended where step 1 moved their checks.
* **Each record's `Documented in` line** names the page that states its rule.
