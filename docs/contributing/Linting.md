# Linting And Static Analysis

This document is for contributors. It describes the four checks the tree must pass, how to run
each one, and the traps in each.

The four checks are cpplint, the compiler's warnings, MSVC `/analyze` and clang-tidy. **The tree
is clean at all four**, so every finding is new and should be fixed. cpplint runs in CI. The
other three are build options declared in [CMakeLists.txt](../../CMakeLists.txt).

A fifth check, the prose gate, applies the writing rules. It reads only the lines a change adds.
It runs in CI with the link rule and boundary gates. [Review.md](Review.md) lists every gate.

- [cpplint](#cpplint)
- [The compiler](#the-compiler)
- [MSVC /analyze](#msvc-analyze)
- [clang-tidy](#clang-tidy)
- [Prose](#prose)
- [CI](#ci)
- [The review gates' environment](#the-review-gates-environment)

## cpplint

cpplint checks the code against the
[Google C++ style guide](https://google.github.io/styleguide/cppguide.html). Install the pinned
version:

```
pip install cpplint==2.0.2
```

CI installs the same version. This is the community fork of cpplint, which renames checks between
releases. Do not upgrade it without checking the tree again.

Run it on the whole tree from the repository root:

```
cpplint --linelength=180 \
  --exclude=out --exclude=vendor --exclude=vcpkg_installed \
  --exclude=demos/voxel/src/noise --recursive .
```

**The expected output is zero errors.**

The excludes:

- `out`, `vendor` and `vcpkg_installed` exist only on a developer machine. They hold far more
  files than the project, mostly third-party headers. CI checks out no submodules and installs no
  packages, so it has none of them.
- `demos/voxel/src/noise` is third-party code kept as it was published. The static analysers skip it
  too.

`--exclude` removes files from the lint, not from the directory walk, so a run still walks the
excluded directories.

For Visual Studio's external tools, this command lints the current file:

```
C:\Python312\python.exe C:\Python312\Lib\site-packages\cpplint.py --linelength=180 --output=vs7 $(ItemPath)
```

**Do not add a `--filter`.** Every cpplint check is enforced. cpplint accepts a filter that names
a category it does not have, and then suppresses nothing without saying so. A filter entry whose
check was renamed looks the same as a tree that started failing.

### Namespace indentation

A namespace body is not indented. cpplint cannot tell a continuation line from a declaration, so
a continuation line at namespace scope also starts at column 0:

```
Recorder::Target::Target() noexcept :
image(VK_NULL_HANDLE),
view(VK_NULL_HANDLE) {
}

const char* const charset =
" !\"#$%&'()*+,-./0123456789:;<=>?"
"@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_";
```

cpplint misreads only the constructor of a nested class: `Plain::Plain()` may indent its
initialiser list and `Recorder::Target::Target()` may not. The tree writes both at column 0 so
they look the same.

## The compiler

- **`/W4` with `/WX`.** Warnings are errors whenever this repository is the top-level project.
  That is every build of this repository and no build of a project that nests it. To get past a
  warning temporarily, configure with `-DV3D_WARNINGS_AS_ERRORS=OFF`. Do not edit the flag.
- **`/w14062`.** This warning is not part of `/W4`. It reports an enumerator that a `switch` does
  not handle, and only for a `switch` with no `default:` label. A `switch` over an enum in this
  tree has no `default:`, so adding an enumerator fails the build everywhere it needs handling.
  `api/ui` relies on this for `component::Type`. A `switch` that needs a catch-all case writes
  `default:` and is not checked.

Background: [ADR-0047](../adr/0047-code-exhaustive-enum-switches.md)

## MSVC /analyze

Turn it on with `-DV3D_ANALYZE=ON`. It is off by default because it makes a build several times
slower. Its findings are the C6xxx and C26xxx warnings, and `/WX` turns them into errors like any
other warning. The tree has no findings.

The build adds `/analyze:external-` with it. CMake passes an imported target's include
directories to MSVC as `/external:I`. Without `/analyze:external-`, the analyser also checks boost
and glm headers. That takes longer than the whole tree and reports findings in code this
repository does not own.

Two third-party files are excluded from both analysers:

- `demos/voxel/src/noise/noiseutils.cpp`, excluded in both the voxel app and its test suite, which
  compile the same file.
- `api/asset/media/loader/CgltfImpl.cpp`.

`/analyze-` removes a file from MSVC's analysis, and `SKIP_LINTING TRUE` removes it from
clang-tidy. Do not fix findings in these files, because a fix would be lost when the package is
updated.

## clang-tidy

Turn it on with `-DV3D_CLANG_TIDY=ON`. It is off by default for the same cost reason. The check
list is in [.clang-tidy](../../.clang-tidy). The clang-tidy executable comes with the MSVC install,
under `VC/Tools/Llvm/x64/bin`.

`.clang-tidy` enables four check families and subtracts 7 checks by name. The tree is clean at
the 199 that remain. Each subtraction is settled, and the `.clang-tidy` comment says why. A new
finding is fixed, never answered by widening an exclusion or an option.

`misc-include-cleaner` ignores every header installed outside the tree: vcpkg, the vendored
trees, the Vulkan SDK and the toolchain. A library such as glm or Vulkan is included through its
public header, and the check would otherwise ask for the internal header that declares each
symbol. It checks the tree's own headers and the standard library's. Two pragmas answer it:

- `// IWYU pragma: export` on an include makes the including header provide what that include
  provides. `api/engine/Main.h` exports `Application.h`, so an app's `main` that calls `run<T>`
  uses `Main.h`.
- `// IWYU pragma: keep` keeps an include the check finds unused. A source file that exists only
  to compile its header on its own keeps that include this way.

Each `tests` directory has a `.clang-tidy` of its own. It inherits the root file and turns off
`readability-magic-numbers`, because a test's literal is the value it expects. A new tests
directory needs a copy of that file.

`V3D_WARNINGS_AS_ERRORS` also decides whether a clang-tidy finding stops the build. Turning
either analyser on or off changes the compile commands, so ninja rebuilds what it needs to.

Two traps, both silent:

- **CMake writes a system include directory as one argument, `-external:I<dir>`.** clang-cl
  accepts that option only as two arguments. clang-tidy drops it and every option after it
  without a warning. The symptom is "cannot use 'throw' with exceptions disabled" on every source
  that throws. The build passes `--extra-arg-before=/EHsc` to clang-tidy, which puts `/EHsc`
  ahead of the compile command so it survives.
- **clang-tidy ignores a check name it does not recognise.** `.clang-tidy` enables whole families
  and subtracts checks by name, so a subtracted name that stops matching a check turns that check
  back on.

## Prose

[scripts/prose.ts](../../scripts/prose.ts) checks the writing rules in
[Conventions.md](Conventions.md#writing). It needs Node 24 or later and nothing else. Run it from
the repository root:

```
node scripts/prose.ts
```

It compares the working tree with the merge base of `HEAD` and `origin/main`, or `main` when
there is no `origin/main`. `--base REF` names another branch to compare with. Uncommitted and
untracked files are included, so it can run before a commit.

It reads the lines a change adds to Markdown files and to the comments of C++, GLSL, CMake, batch,
YAML, TypeScript and JavaScript files. It rebuilds the paragraph around each added line and
checks every sentence that overlaps one. It reports:

- a sentence of more than 35 words;
- a sentence that starts with "And" or "So";
- a clause such as "X, which is what Y", or the same with how, where, why or when;
- "X is what Y" where a verb follows "what", as in "the submit is what signals the fence";
- a sentence with three clause joins, or with two when one is a semicolon or a dash. A join is
  ", which", ", so", "so that", "because", "rather than", a semicolon, or a dash between two
  words. All the semicolons of a sentence count as one join, and so do the two dashes around
  an aside. A semicolon at the end of a list item is not counted, and neither is a minus sign
  such as the one in "n - 1";
- two relative clauses on one noun, as in "a Ring, which this drives but which works without
  it";
- "knows", "wants", "owes" or "trusts", or "does not know" and its kin, after a subject that is
  not a person, an app, a caller or a consumer;
- "used to", a date or "phase N", except in [plans/](../plans) and [roadmap/](../roadmap);
- a Markdown line, or a comment line with no code before it, longer than 100 columns;
- a comment line more than 10 columns longer than every other line of its paragraph.

Code fences, code spans, tables, headings, URLs and front matter are not checked. Neither are
`vendor/`, [audits/](../audits), the completed plans and roadmaps, or an ADR that is not new.

Each finding prints as `path:line: rule: detail | sentence`, and the script exits with 1 when
there is one. `--all FILE...` checks whole files instead of a change. The tree is not clean at
`--all`, so use it to try a rule, not as a gate.

## Comments only

[scripts/commentsonly.ts](../../scripts/commentsonly.ts) checks that a change moved nothing but
comments and whitespace. Run it after a change that is meant to touch only comments, such as a
sweep of a directory's comments:

```
node scripts/commentsonly.ts
```

It compares each changed C++, GLSL and CMake file in the working tree with `HEAD`. `--base REF`
compares with another commit, so `--base HEAD~1` checks a sweep that is already committed. It
removes the comments from both versions, collapses whitespace outside literals, drops blank
lines, and reports:

- the first code line that differs, with its text at the base and now;
- a block comment's closing marker left in code, which is usually a comment that closed early;
- a line comment that ends with a backslash, because the compiler joins the next line to it;
- a checked file that the change adds or removes.

Other files are not read. It is not a CI gate, because most changes move code. A comment change
can still break the build in a way the script does not see, so build and run the tests as well.

## CI

[.github/workflows/cpplint.yml](../../.github/workflows/cpplint.yml) installs cpplint with pip on
an Ubuntu runner and runs the command above. It runs on each pull request and on each push to
`main`.

It is a separate workflow from [ctest.yml](../../.github/workflows/ctest.yml), which builds the
tree on a Windows runner and runs the tests. cpplint needs only Python. The build needs MSVC, the
Vulkan SDK, a vcpkg install and the libnoise submodule.

Neither `/analyze` nor clang-tidy runs in CI, because of their cost. They are run locally.

[.github/workflows/review-gates.yml](../../.github/workflows/review-gates.yml) runs four of the
review gates on an Ubuntu runner with Node: the link rule (`scripts/linkrule.ts`), the library
table (`scripts/librarytable.ts`), prose (`scripts/prose.ts`) and boundary input
(`scripts/boundary.ts`). It runs on each pull request and on each push to `main`. The link rule
and the library table check the whole tree. The prose and boundary gates
read only the lines added since a base:

- on a pull request, the base is the branch the pull request merges into;
- on a push to `main`, the base is the previous tip of `main`. When the push names no previous
  tip, as on a new branch, or names one the checkout does not have, as after some force pushes,
  the base is the pushed commit's parent. A pushed root commit has no parent, and the step fails
  with a message.

**Every action a workflow uses is pinned to a full commit sha**, with the release it came from in
a comment:

```yaml
- uses: actions/checkout@11d5960a326750d5838078e36cf38b85af677262 # v4.4.0
```

A tag can be moved to other code after it was reviewed, and a sha cannot. `scripts/pinned.ts`
reads every workflow in `.github/workflows` and every `action.yml` under `.github/actions`, with
either the `.yml` or the `.yaml` extension. It fails on a `uses:` whose value is not a full
lowercase sha, written in a block or a flow mapping, plain or quoted. A local action, named by a
path in this repository, is exempt. To find the sha for a release, read the commit its tag
points at: `gh api repos/<owner>/<repo>/git/ref/tags/<tag>`, followed by `git/tags/<sha>` when
the tag is annotated. Updating an action means changing both the sha and the comment.

The workflow first runs the gates' own tests in `scripts/tests/`, which cover the C++ lexer the
gates share in `scripts/lexer.ts`. Run them locally with `node --test "scripts/tests/*.test.ts"`
from the repository root.

The fails-first gate, `scripts/failsfirst.ts`, runs in
[.github/workflows/ctest.yml](../../.github/workflows/ctest.yml) instead, as that job's last step
on each pull request. It needs the compiler, the packages and the software Vulkan driver, which
that job already has. It compares against the branch the pull request merges into, and fails
the job when a new case passes without the change or was not run. Its scratch directory is under
`RUNNER_TEMP`, which the job excludes from Defender. A link that a scan breaks would mark every new
case as failing to build, and the gate would pass them. Run it locally as well, before review: see
[Testing.md](Testing.md#checking-that-a-new-test-fails-first).

## The review gates' environment

The review gates in `scripts/` support one environment:

- Node 24 or later. An earlier Node that can run TypeScript stops a gate with exit 2 and a
  message. A Node too old to run TypeScript fails to load the gate at all.
- A checkout of this repository with its full history, so a gate can find a merge base.
- The repository root as the working directory, as CI and these documents run them.
- For `scripts/failsfirst.ts` only, the Windows developer environment `scripts\build.cmd` uses.

[Review.md](Review.md) grades a fault in the gates by whether it arises in this environment.
