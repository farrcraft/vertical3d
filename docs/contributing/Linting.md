# Linting And Static Analysis

This document is for contributors. It describes the four checks the tree must pass, how to run
each one, and the traps in each.

The four checks are cpplint, the compiler's warnings, MSVC `/analyze` and clang-tidy. **The tree
is clean at all four**, so every finding is new and should be fixed. cpplint runs in CI. The
other three are build options declared in [CMakeLists.txt](../../CMakeLists.txt).

A fifth check, the prose gate, applies the writing rules. It reads only the lines a change adds.

- [cpplint](#cpplint)
- [The compiler](#the-compiler)
- [MSVC /analyze](#msvc-analyze)
- [clang-tidy](#clang-tidy)
- [Prose](#prose)
- [CI](#ci)

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
  --exclude=voxel/src/noise --recursive .
```

**The expected output is zero errors.**

The excludes:

- `out`, `vendor` and `vcpkg_installed` exist only on a developer machine. They hold far more
  files than the project, mostly third-party headers. CI checks out no submodules and installs no
  packages, so it has none of them.
- `voxel/src/noise` is third-party code kept as it was published. The static analysers skip it
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

const wchar_t* const charcodes =
L" !\"#$%&'()*+,-./0123456789:;<=>?"
L"@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_";
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

- `voxel/src/noise/noiseutils.cpp`, excluded in both the voxel app and its test suite, which
  compile the same file.
- `api/asset/media/loader/CgltfImpl.cpp`.

`/analyze-` removes a file from MSVC's analysis, and `SKIP_LINTING TRUE` removes it from
clang-tidy. Do not fix findings in these files, because a fix would be lost when the package is
updated.

## clang-tidy

Turn it on with `-DV3D_CLANG_TIDY=ON`. It is off by default for the same cost reason. The check
list is in [.clang-tidy](../../.clang-tidy). The clang-tidy executable comes with the MSVC install,
under `VC/Tools/Llvm/x64/bin`.

`.clang-tidy` enables four check families and subtracts 20 checks by name. The tree is clean at
the 186 that remain. [TODO.md](../TODO.md#the-clang-tidy-backlog) lists what each subtracted
check reports, except seven that the `.clang-tidy` comment marks as settled. To remove a line from
that list, fix what the check reports. Do not widen the exclusion.

`V3D_WARNINGS_AS_ERRORS` also decides whether a clang-tidy finding stops the build. Turning
either analyser on or off changes the compile commands, so ninja rebuilds what it needs to.

Two traps, both silent:

- **CMake writes a system include directory as one argument, `-external:I<dir>`.** clang-cl
  accepts that option only as two arguments. clang-tidy drops it and every option after it
  without a warning. The symptom is "cannot use 'throw' with exceptions disabled" on every source
  that throws. The build passes `--extra-arg-before=/EHsc` to clang-tidy, which puts `/EHsc`
  ahead of the compile command so it survives.
- **clang-tidy ignores a check name it does not know.** `.clang-tidy` enables whole families and
  subtracts checks by name, so a subtracted name that stops matching a check turns that check
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

It reads the lines a change adds to Markdown files and to the comments of C++, GLSL, CMake, batch
and YAML files. It rebuilds the paragraph around each added line and checks every sentence that
overlaps one. It reports:

- a sentence of more than 35 words;
- a sentence that starts with "And" or "So";
- a clause such as "X, which is what Y", or the same with how, where, why or when;
- "knows", "wants", "owes" or "trusts", or "does not know" and its kin, after a subject that is
  not a person, an app, a caller or a consumer;
- "used to", a date or "phase N", except in [plans/](../plans) and [roadmap/](../roadmap);
- a Markdown line longer than 100 columns;
- a comment line more than 10 columns longer than every other line of its paragraph.

Code fences, code spans, tables, headings, URLs and front matter are not checked. Neither are
`vendor/`, [audits/](../audits), the completed plans and roadmaps, or an ADR that is not new.

Each finding prints as `path:line: rule: detail | sentence`, and the script exits with 1 when
there is one. `--all FILE...` checks whole files instead of a change. The tree is not clean at
`--all`, so use it to try a rule, not as a gate.

## CI

[.github/workflows/cpplint.yml](../../.github/workflows/cpplint.yml) installs cpplint with pip on
an Ubuntu runner and runs the command above. It runs on each pull request and on each push to
`main`.

It is a separate workflow from [ctest.yml](../../.github/workflows/ctest.yml), which builds the
tree on a Windows runner and runs the tests. cpplint needs only Python. The build needs MSVC, the
Vulkan SDK, a vcpkg install and the libnoise submodule.

Neither `/analyze` nor clang-tidy runs in CI, because of their cost. They are run locally.
