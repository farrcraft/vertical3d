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

Not started. Prose and boundary read added lines through the one parser from step 3. Tests cover
quoted paths, a hunk line that starts `++ `, a missing newline at the end of a file, untracked
files, and deleted files.

### Step 5 — failsfirst split and tested

Not started. Split `failsfirst.ts` by job: finding cases, editing CMake lists, the worktree, the
developer environment, configure, build, running and classifying, the report. Test `classify`,
`failedSources`, `missingSources`, `dropSource` and the options without a build.

### Step 6 — prose split and tested

Not started. Split `prose.ts` into comment spans, paragraphs, sentences and rules. Test each rule
on Markdown and on comments, and the Markdown structure the gate skips: front matter, fences,
tables, link-only lines, and the exemptions for plans and ADRs.

### Step 7 — linkrule split and tested

Not started. Split `linkrule.ts` into the CMake reader, targets, the include walk and the rules.
Test each of the six rules against a small fixture tree.

### Step 8 — One report format, and one test file per gate

Not started. Step 3 put every gate on the shared modules and one option parser. Every gate also
reports in one format, and `gates.test.ts` is split into one test file per gate.

### Step 9 — The tree's rules apply to scripts

Not started. `filelength.ts` reads `scripts/**/*.ts`, with tests exempt. One quote style and one
form of file header across the gates. Example editor scripts are checked against `editor.d.ts`.

### Step 9a — An index read can miss

Not started. Turn on `noUncheckedIndexedAccess` in `tsconfig.base.json`, and handle each read
that can miss: a regular expression group, an array element, or a map entry read by index.

### Step 10 — One reference page for the gates

Not started. `docs/contributing/Gates.md` lists every gate, what it checks and how to run it, how
to write a new one, and the shared modules. Linting.md, Conventions.md, Build.md and Testing.md
link to it instead of repeating it. The list of gates CI runs is corrected.

### Step 11 — Close the plan

Not started.

## Open questions

- Whether to add a linter or a formatter. Each is a separate decision, and the support of
  `typescript-eslint` for TypeScript 7 is not settled.
- Whether a function-length rule for TypeScript is worth a gate, since no tool in the tree
  measures one.
