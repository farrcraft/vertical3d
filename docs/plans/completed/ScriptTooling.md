# Script Tooling — Typed, Shared and Tested Gates

The review gates in `scripts/` decide what merges, and they are held to less than the C++ they
check. Two files are past the 1000-line limit or near it, plumbing is copied between gates, the
verdict logic of the largest gate is untested, and no TypeScript in the tree is type-checked.
This plan brings the gates and the rest of the tree's TypeScript to the standard of the C++.

## Decisions

- [ADR-0158](../adr/0158-tooling-typescript-is-type-checked-through-a-package-json.md):
  TypeScript is type-checked through a `package.json` of exact development dependencies. No
  code that runs imports a package.
- Every documented command stays as it is. `node scripts/<gate>.ts` remains the entry point, and
  the code behind a large gate moves into a directory of the same name.

## What blocks what

- Step 1 comes first, so that every later step is type-checked as it lands.
- Step 3 comes before steps 5 to 8, because they move onto the shared modules it adds.
- Step 4 comes before step 6, because prose and boundary share the diff parser it tests.
- Step 9a comes after steps 5 to 7, so that the reads it reports are fixed once.
- Step 10 comes last but one, because it describes the layout steps 3 to 9a leave.

## Steps

### Step 1 — A package.json and the type check

**Done.** `package.json` pins TypeScript 7.0.2 and `@types/node` 24.19.2 as development
dependencies, with a lockfile, and `.npmrc` turns off install scripts. Two projects divide the
files: the root `tsconfig.json` reads every `.ts` file outside the directories it leaves out, and
`vertical3d/data/scripts` checks the editor's scripts with no Node declarations. Both extend a
base of the strict options, the unused-code options and `exactOptionalPropertyTypes`, which
found one optional property in failsfirst. `review-gates.yml` installs with `npm ci` and
type-checks before the gate tests. Dependencies.md states the rule.

`noUncheckedIndexedAccess` reports 171 reads, most of them in the three gates steps 5 to 7
rewrite. Step 9a turns it on once they land.

### Step 2 — Development dependencies stay pinned

**Done.** `scripts/npm.ts` reports a runtime dependency, a version that is not exact, and a
lockfile that disagrees with `package.json` or holds a package from outside the registry. It
reports an `.npmrc` that leaves install scripts on, and a TypeScript file that imports a package
at run time. Hand mutations of each rule fail a test. CI runs it beside `pinned.ts`.

### Step 3 — Shared modules in scripts/lib

**Done.** Every gate runs on the modules in `scripts/lib`:

- `tree.ts` holds the root, one list of third-party directories and one of generated ones, and
  the rule for a test path.
- `git.ts` runs git, throws a `GateError` when a command fails, and names a commit and a merge
  base.
- `cli.ts` runs a gate when Node is started with it, reads its options through `parseArgs`, and
  turns a `GateError` into exit status 2.
- `script.ts` blanks the comments and literals of a TypeScript file.
- `lexer.ts` and `entry.ts` moved there unchanged.

`scripts/vcvars.cmd` enters the developer environment for `build.cmd`, `test.cmd` and
failsfirst. A full failsfirst run on one commit entered it, built the suite and reported.

The npm gate from step 2 read an import inside a string as a real one, and reported its own test
file. It now matches on the code that `script.ts` leaves. Hand mutations of each module fail a
test. Running the gates during this step found a file-local finding already on the branch, in
`Presenter.cxx`, fixed in a commit of its own.

### Step 4 — The diff parser, tested

**Done.** `scripts/lib/diff.ts` reads the lines a change adds, and prose and boundary both read
through it. Tests cover quoted paths, a hunk line that starts `++ `, a missing newline at the end
of a file, untracked and deleted files, and a user's git configuration. Two of them make a
repository and read what git itself prints. The parser no longer counts a hunk's removed lines,
which a hunk with no context lists before its added ones, so they cannot be read as a header.
Over the branch and its last 150 commits, both gates report exactly what they did before.

### Step 5 — failsfirst split and tested

**Done.** `failsfirst.ts` keeps its header, its options, the order of the steps and `main`, in
about 300 lines. The rest moved unchanged into `scripts/failsfirst/`: one module each for the
words it uses, git, finding cases, CMake lists, the developer environment, the worktree, the
build, running a case, and the report. `tests/failsfirst.test.ts` covers each of them without a
build, and holds the cases that were in `gates.test.ts`. A hand mutation of each verdict rule
fails a test.

The tests found that `describeExit` printed an NTSTATUS read as a signed number as a negative
decimal, and it now prints it in hexadecimal. `--list` prints the same as before over the branch
and four commits, and a full run on one commit reports what it did before the split.

### Step 6 — prose split and tested

**Done.** `prose.ts` keeps the line checks, the options and `main`. `scripts/prose/` holds which
files the gate reads, how it rebuilds paragraphs, how it divides sentences, and the rules. The
comments of a TypeScript file come from `lib/script.ts`, which gave the same spans as the gate's
own scanner on every TypeScript file in the tree. `tests/prose.test.ts` checks each rule on
Markdown and on comments, the structure the gate does not read, its exemptions, and the limit to
added lines. A hand mutation of each rule fails a test. Over every file the gate reads, and over
the last 150 commits, it reports what it did before.

### Step 7 — linkrule split and tested

**Done.** `linkrule.ts` keeps `violations()`, which checks a tree given by its root and its file
lists, and `main`. `scripts/linkrule/` holds the CMake reader and targets, what a C++ file
includes and uses, the tree of owners, and the rules. The 140-line check is one function per
rule. `tests/linkrule.test.ts` checks each rule on a small tree written to a temporary
directory, and a hand mutation of each rule fails a test. With violations planted in the real
tree, the gate reports what it did before.

The tests found that a suite naming `Boost::unit_test_framework` was not reported, although the
rule forbids it. The gate stood in for what `v3d_add_test` links by adding the framework to every
suite's links, so a suite that named it as well looked the same. The package rule now takes the
framework as given to a suite instead.

### Step 8 — One report format, and one test file per gate

**Done.** Every gate prints its findings through `report()` in `lib/cli.ts`, as
`path:line: rule: message` or `path: rule: message`, and ends with one `::error::` line saying
what each finding breaks. Five gates printed no rule name or no such line before.

Each gate's tests are in a file of its own, and `boundary.test.ts` gained a test for each rule
and exemption. `gates.test.ts` now starts every TypeScript file in `scripts/` as a process. It
lists them itself, and each must refuse an unknown option with exit 2. Five gates had no such
test before, so a gate that never ran its command line would have passed CI.

### Step 9 — The tree's rules apply to scripts

**Done.** `filelength.ts` reads every tracked TypeScript file but tests and third-party code, and
the size rule in Conventions.md names TypeScript. Every gate uses single quotes and opens with a
`/** */` header. The fails-first harness and boundary were the exceptions.

Three example scripts in `vertical3d/data/scripts/examples/` ship with the editor. The editor's
type check reads them against `editor.d.ts`, and `the_example_scripts_run` in the editor suite
runs each on a document holding a cube. That check read only the files directly in
`data/scripts` before, so it saw no script at all. It now reads the whole directory, and checks
each script in a scope of its own.

### Step 9a — An index read can miss

**Done.** `noUncheckedIndexedAccess` is on for every TypeScript file. Most reads it reported
became a form that cannot miss, such as `entries()`, destructuring or `at(-1)`. A read the code
already guarantees has a fallback that is never taken. No `!` was added. Every gate prints what
it printed before over the same inputs, and every generator writes the same bytes.

It found that `make_sounds.ts` read past its samples. A quarter second at 22050 samples a second
is not a whole number, so one crossfade sample was `NaN`, and the whole of `well.wav` is
silence. The commit for this step keeps that output, and the next one fixes it.

### Step 10 — One reference page for the gates

**Done.** `docs/contributing/Gates.md` lists every gate, what it checks, what it reads and where
its rule is stated. It also covers how CI runs the gates and picks a base, running them locally,
what a gate prints, the supported environment, where the code is, and how to write a gate. The
environment moved there from Linting.md. Linting.md had said CI ran four gates when it ran nine,
and now points to Gates.md. Review.md, the contributing index and CLAUDE.md link to it. The
documents that state a rule still name the gate that checks it, since the rule lives there.

### Step 11 — Close the plan

**Done.** The open questions moved to TODO.md.

## Outcome

Closed 2026-11-03. The review gates are type-checked, split by job onto shared modules, and
tested rule by rule, and a hand mutation of each rule fails a test. One page describes them.

**Delivered:**

- A `package.json` of exact development tools, the type check in CI, and a gate that keeps npm
  to pinned tools and no script to a runtime package (ADR-0158).
- `scripts/lib/`: one way to run git, read a change, list the tree, read options and report.
- failsfirst, prose and linkrule split into modules, each under 1000 lines, with tests for each
  module. No gate's output changed over the comparisons each step ran.
- One output format and an `::error::` line for every gate, and a test that starts every gate.
- The tree's size rule and `noUncheckedIndexedAccess` over all of its TypeScript.
- Three editor example scripts, type-checked and run by the editor suite.
- `docs/contributing/Gates.md`.

**Different from the plan:**

- The work found seven defects outside its scope, each fixed in its own commit or step:
  - the npm gate read an import inside a string;
  - linkrule never reported a suite that named the test framework;
  - failsfirst printed an NTSTATUS as a negative number;
  - five gates had no test that they run at all;
  - the editor's type check read no script;
  - `Presenter.cxx` broke the file-local rule;
  - `well.wav` was silent.
- Step 3 did most of step 8's plumbing, so step 8 became the output format and the test files.
- `noUncheckedIndexedAccess` was a step of its own, after the splits, so each read was fixed once.

**Left open:** two entries in TODO.md, under Review gates. There is no linter or formatter for
the TypeScript, and no check of a TypeScript function's length.
