# The Review Gates

This document is for contributors. It lists the review gates, says how they run and where their
code lives, and says how to add one. A gate is a script that checks a rule a compiler cannot,
and CI runs it on every pull request. The rule each gate checks is stated in full in the
document for its subject, which the table links to.

- [The gates](#the-gates)
- [In CI](#in-ci)
- [Running them](#running-them)
- [What a gate prints](#what-a-gate-prints)
- [The environment](#the-environment)
- [Where the code is](#where-the-code-is)
- [Writing a gate](#writing-a-gate)

## The gates

| Gate | Checks | Reads | The rule |
|---|---|---|---|
| `linkrule.ts` | Each app, app library and suite links what it includes, at the right visibility | The tree | [Build.md](Build.md#linking-rules) |
| `librarytable.ts` | The table of libraries has one row for each library in the manifest, in order | The tree | [Build.md](Build.md#the-manifest-is-checked-on-every-configure) |
| `readmes.ts` | Each component has a README with a heading and links that resolve, and a library's README describes it as the table does | The tree | [Conventions.md](Conventions.md#readmes) |
| `summary.ts` | `docs/SUMMARY.md` lists each page under `docs/` once, outside `completed/` directories | The tree | [Conventions.md](Conventions.md#the-documentation-site) |
| `filelocal.ts` | What a source file keeps to itself | The tree | [Conventions.md](Conventions.md#what-a-source-file-keeps-to-itself) |
| `filelength.ts` | No C++ or TypeScript file but a test is over 1000 lines | The tree | [Conventions.md](Conventions.md#size) |
| `dirsize.ts` | No directory holds more than 15 headers unless it is listed as one family | The tree | [Conventions.md](Conventions.md#directories) |
| `namespaces.ts` | Each C++ file is in its directory's namespace | The tree | [Conventions.md](Conventions.md#language) |
| `pinned.ts` | Every action a workflow uses is pinned to a full commit sha | The tree | [Linting.md](Linting.md#ci) |
| `npm.ts` | npm brings only pinned development tools, and no script imports a package | The tree | [Dependencies.md](Dependencies.md#typescript-tooling-from-npm) |
| `prose.ts` | The writing rules | Added lines | [Conventions.md](Conventions.md#writing), [Linting.md](Linting.md#prose) |
| `boundary.ts` | Boundary input converted without a check | Added lines | [Conventions.md](Conventions.md#boundary-input) |
| `failsfirst.ts` | Each test case a change adds fails without the change | Added cases | [Testing.md](Testing.md#checking-that-a-new-test-fails-first) |
| `commentsonly.ts` | A change meant to touch only comments moved no code | Changed files | [Linting.md](Linting.md#comments-only) |

`commentsonly.ts` is not a CI gate, because most changes move code. It is run by hand after a
sweep of comments. [Review.md](Review.md) says which class of review finding each gate answers.

## In CI

[review-gates.yml](../../.github/workflows/review-gates.yml) runs on an Ubuntu runner with Node.
It installs the TypeScript tooling with `npm ci` and type-checks every TypeScript file. Then it
runs the gates' own tests, and then each gate as a step of its own. Every step runs even when
one before it failed, so each gate reports.

The gates that read added lines compare against a base:

- on a pull request, the base is the branch the pull request merges into;
- on a push to `main`, the base is the previous tip of `main`. When the push names no previous
  tip, as on a new branch, or names one the checkout does not have, as after some force pushes,
  the base is the pushed commit's parent. A pushed root commit has no parent, and the step fails
  with a message.

`failsfirst.ts` runs in [ctest.yml](../../.github/workflows/ctest.yml) instead, as that job's
last step on each pull request. It needs the compiler, the packages and the software Vulkan
driver, which that job already has. Its scratch directory is under `RUNNER_TEMP`, which the job
excludes from Defender. A link that a scan broke would mark every new case as failing to build,
and the gate would pass them.

## Running them

From the repository root:

```
npm ci                       # once, for the type checker
npm run typecheck            # every TypeScript file
npm test                     # the gates' own tests
node scripts/linkrule.ts     # any gate, by its file
node scripts/prose.ts --base main
```

A gate that reads the whole tree takes no options. One that reads added lines compares the
working tree, staged or not, with the merge base of `HEAD` and `origin/main`, or `main` when
there is no `origin/main`. `--base REF` names another branch or commit. Untracked files count as
added. [Testing.md](Testing.md#checking-that-a-new-test-fails-first) covers the options of
`failsfirst.ts`, which builds the affected suites and takes several minutes.

Running a gate needs only Node. `npm ci` is needed only for the type check.

## What a gate prints

Each finding is one line on standard output, `path:line: rule: message`, or `path: rule: message`
when it has no line. After the findings comes one line starting `::error::` that says what each
breaks, which GitHub Actions shows on the run.

| Exit status | Means |
|---|---|
| 0 | Nothing found |
| 1 | At least one finding |
| 2 | The gate could not run: an option it does not take, a base that is not a commit, a failed git command, or a Node before 24. One line on standard error says why. |

`failsfirst.ts` prints a table of cases and their results instead, and exits 2 when a case was
not run.

## The environment

The gates support one environment:

- Node 24 or later. An earlier Node that can run TypeScript stops a gate with exit 2 and a
  message. A Node too old to run TypeScript fails to load the gate at all.
- A checkout of this repository with its full history, so a gate can find a merge base.
- The repository root as the working directory, as CI and these documents run them.
- For `failsfirst.ts` only, the Windows developer environment that `scripts\vcvars.cmd` enters.

[Review.md](Review.md) grades a fault in the gates by whether it arises in this environment.

## Where the code is

| Path | Holds |
|---|---|
| `scripts/<gate>.ts` | Each gate's command line: its header, its options and `main`. Node runs it directly. |
| `scripts/<gate>/` | The modules of a gate too large for one file: `failsfirst/`, `prose/`, `linkrule/` |
| `scripts/docs.ts`, `scripts/docs/` | Building the documentation site: the pinned tools, the book's configuration, and the preprocessor that rewrites links |
| `scripts/lib/cli.ts` | Running a gate, reading its options, printing its findings, and the exit status |
| `scripts/lib/git.ts` | Running git, and naming a commit and a merge base |
| `scripts/lib/diff.ts` | The lines a change adds |
| `scripts/lib/tree.ts` | The repository root, and which paths are third-party, generated or tests |
| `scripts/lib/lexer.ts` | The C++ lexer: the code, comments and literals of a C++ file |
| `scripts/lib/script.ts` | The comments and literals of a TypeScript file |
| `scripts/lib/entry.ts` | Whether a gate runs, and the version of Node it needs |
| `scripts/tests/` | One test file for each gate and for each module of `lib/` |
| `scripts/vcvars.cmd` | Entering the developer environment, for `build.cmd`, `test.cmd` and `failsfirst.ts` |

The gates are held to the tree's rules. They are type-checked, no file is over 1000 lines, and
they use only Node's own modules when they run.

## Writing a gate

1. **State the rule first,** in the reference document for its subject. The gate checks the
   rule; the document is where a reader learns it.
2. **Add `scripts/<name>.ts`.** Its header comment says what it reports, rule by rule, and how
   it is run. Its `main` reads options through `options()`, and returns `report()` with its
   findings. The file ends with `runGate(import.meta.url, main)`, so that a test can import it.
3. **Keep the checking pure.** Export functions that take text and return findings, and read
   the tree only in `main`. Use `tracked()`, `changeSince()` and the paths in `lib/tree.ts`
   instead of running git or listing directories again.
4. **Test every rule** in `scripts/tests/<name>.test.ts`, with a case that reports and one that
   does not. Break each rule by hand and check that a test fails. `gates.test.ts` starts every
   gate as a process without being told about a new one.
5. **Add a step** to [review-gates.yml](../../.github/workflows/review-gates.yml), with
   `if: ${{ !cancelled() }}`, and a row to the table above.
6. **Run it on the tree.** The tree is clean at every gate, so a new gate starts clean too: fix
   what it finds in the same change, or list an exception in the gate with its reason.
