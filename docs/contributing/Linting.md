# Linting And Static Analysis

This document is for contributors. It describes the four checks the tree must pass, how to run
each one, and the traps in each.

The four checks are cpplint, the compiler's warnings, MSVC `/analyze` and clang-tidy. **The tree
is clean at all four**, so every finding is new and should be fixed. cpplint runs in CI. The
other three are build options declared in [CMakeLists.txt](../../CMakeLists.txt).

The review gates in `scripts/` check the rules a compiler cannot, such as the writing rules and
the linking rules. [Gates.md](Gates.md) lists them and says how they run.

- [cpplint](#cpplint)
- [The compiler](#the-compiler)
- [MSVC /analyze](#msvc-analyze)
- [clang-tidy](#clang-tidy)
- [Prose](#prose)
- [Comments only](#comments-only)
- [CI](#ci)

## cpplint

cpplint checks the code against the
[Google C++ style guide](https://google.github.io/styleguide/cppguide.html). Install the pinned
version:

```
pip install cpplint==2.0.2
```

CI installs the same version, and the pre-commit hooks in [Gates.md](Gates.md#before-a-commit)
run it on the staged files at that version. This is the community fork of cpplint, which renames
checks between releases. Do not upgrade it without checking the tree again. An upgrade moves the
pin in `cpplint.yml` and in `.pre-commit-config.yaml` together.

Run it on the whole tree from the repository root:

```
cpplint --linelength=180 \
  --exclude=out --exclude=vendor --exclude=vcpkg_installed --recursive .
```

**The expected output is zero errors.**

The excludes, `out`, `vendor` and `vcpkg_installed`, exist only on a developer machine. They hold
far more files than the project, mostly third-party headers. CI installs no packages, so it has
none of them.

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

Background: [ADR-0047](../adr/project/0047-code-exhaustive-enum-switches.md)

## MSVC /analyze

Turn it on with `-DV3D_ANALYZE=ON`, or build the `analyze` or `verify` preset, which turn it on
in a build directory of their own. It is off by default because it makes a build several times
slower. Its findings are the C6xxx and C26xxx warnings, and `/WX` turns them into errors like any
other warning. The tree has no findings.

The build adds `/analyze:external-` with it. CMake passes an imported target's include
directories to MSVC as `/external:I`. Without `/analyze:external-`, the analyser also checks boost
and glm headers. That takes longer than the whole tree and reports findings in code this
repository does not own.

One third-party file is excluded from both analysers, `api/interchange/CgltfImpl.cxx`.

`/analyze-` removes a file from MSVC's analysis, and `SKIP_LINTING TRUE` removes it from
clang-tidy. Do not fix findings in these files, because a fix would be lost when the package is
updated.

## clang-tidy

Turn it on with `-DV3D_CLANG_TIDY=ON`, or build the `tidy` or `verify` preset. It is off by
default for the same cost reason. The check
list is in [.clang-tidy](../../.clang-tidy). The clang-tidy executable comes with the MSVC install,
under `VC/Tools/Llvm/x64/bin`.

`.clang-tidy` enables whole check families and subtracts some checks by name. The tree is clean
at every check that remains. Each subtraction is settled, and the `.clang-tidy` comment says why.
A new finding is fixed, never answered by widening an exclusion or an option.

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

### Fixing one check across the tree

With warnings as errors, a finding stops its target, and every target that depends on it is
not compiled. A first pass then reports only some of the sites. To find every site of one check:

1. Configure an analysis build with `-DV3D_WARNINGS_AS_ERRORS=OFF`, and narrow `.clang-tidy` to
   that one check. The build then runs to the end, and its log holds only that check's sites.
2. Fix the sites. Restore `.clang-tidy`, turn warnings as errors back on, and build again to
   verify.
3. Confirm the analyser compiled each changed file, by finding its `.obj` under the analysis
   build's `CMakeFiles/<target>.dir/`. A build that never ran prints no finding either, and
   reads as clean.

Fixing one check can move sites into another. For example, making a member function static
leaves its callers calling it through an instance, which
`readability-static-accessed-through-instance` reports. Expect more than one pass.

`run-clang-tidy -fix` applies more than the one check. It also applies the compiler's own
fix-its, such as reordered member initialisers. It can apply one edit twice to a header that two
sources include. `misc-const-correctness` writes the `const` after the type. Review its diff
line by line, keep only the check's own edits, and fix the rest by hand.

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
Vulkan SDK and a vcpkg install.

Neither `/analyze` nor clang-tidy runs in CI, because of their cost. They are run locally.

[.github/workflows/review-gates.yml](../../.github/workflows/review-gates.yml) runs the review
gates on each pull request and on each push to `main`. [Gates.md](Gates.md#in-ci) says what it
runs and what each gate compares against.

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

