# Linting And Static Analysis

Four gates. cpplint runs on every push; the other three are MSVC-side and are declared in
[CMakeLists.txt](../CMakeLists.txt). **The tree is clean at all four**, so every finding is a
new one.

## cpplint

Code is linted against the
[Google C++ style guide](https://google.github.io/styleguide/cppguide.html) using the cpplint
tool.

> pip install cpplint==2.0.2

The version is pinned, and CI installs the same one. This is the community fork rather than
Google's original, and it renames checks between releases. A filter naming a category cpplint
does not have suppresses nothing and reports nothing, so an unpinned bump can turn the tree red
without a line of code changing. The fork is also what decides the namespace rule below, which
is enforced rather than suppressed.

The tool command for Visual Studio integration:

>  C:\Python312\python.exe C:\Python312\Lib\site-packages\cpplint.py --linelength=180 --output=vs7 $(ItemPath)

To run the linter on everything from the command line:

```
cpplint --linelength=180 \
  --exclude=out --exclude=vendor --exclude=vcpkg_installed \
  --exclude=voxel/src/noise --recursive .
```

The first three excludes matter only locally. CI never builds, checks out no submodules and
installs no ports, so it has none of those trees. A developer machine has all three, and they
hold two orders of magnitude more lintable files than the project does — `vcpkg_installed/` at
the repo root is the worst at 80,000-odd third party headers. `--exclude` filters what is
linted rather than what is walked, so the run costs an `os.walk` of the whole tree either way.

`voxel/src/noise` is the fourth exclude, and it is not local. That code is vendored verbatim,
and `/analyze` and clang-tidy skip it for the same reason.

**A report of zero is the expected result.**

There is no `--filter`: every check cpplint has is enforced. Do not add one. cpplint accepts a
filter naming a category it does not have and then silently suppresses nothing, so an entry
that stops working looks exactly like a tree that started failing. That is what
`runtime/indentation_namespace` did here, for as long as it took the check to be renamed
`whitespace/indent_namespace` under it.

### Namespace indentation

A namespace body is not indented. The check cannot distinguish a continuation line from a
declaration, so a continuation at namespace scope sits at column 0 as well:

```
Recorder::Target::Target() noexcept :
image(VK_NULL_HANDLE),
view(VK_NULL_HANDLE) {
}

const wchar_t* const charcodes =
L" !\"#$%&'()*+,-./0123456789:;<=>?"
L"@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_";
```

It only misreads a *nested* class's constructor: `Plain::Plain()` may indent its initialiser
list and `Recorder::Target::Target()` may not. The tree writes both flush for consistency
rather than by rule.

## The compiler

**`/WX` is on** whenever this project is the top level one, so on every build of this
repository and no build of a consumer that has nested it. The tree is clean at `/W4`, so a
warning is a new one. Use `-DV3D_WARNINGS_AS_ERRORS=OFF` to get past it rather than editing the
flag.

**`/w14062` is on**, which is not a `/W4` default. It reports an enumerator a switch does not
handle, and only for a switch carrying no `default:` label — so it bites exactly where the
author meant the switch to be complete, and a switch that wants a catch-all keeps one by
writing `default:`. `api/ui` depends on it: a switch over `component::Type` is exhaustive so
that adding a component fails the build in every place that has to decide about it, per
[ADR-0047](adr/0047-a-component-type-is-checked-by-the-compiler.md).

**`/analyze`** is `-DV3D_ANALYZE=ON`, off by default because it costs several times a plain
compile of the tree. Its findings are the C6xxx and C26xxx numbers, and they reach `/WX` like
any other warning. Nothing in the tree reports at it.

It is paired with `/analyze:external-`, which is not the default. CMake gives an imported
target's include directories to MSVC as `/external:I`, and without that switch a boost or glm
header costs more analysis time than the whole tree and reports findings in code this
repository does not own.

Two files are third party and are exempt from both analysers:
`voxel/src/noise/noiseutils.cpp`, in the app's `set_source_files_properties` and the suite's
since they share the file, and `api/asset/loader/CgltfImpl.cpp`. `/analyze-` takes a source out
of MSVC's analysis and `SKIP_LINTING TRUE` takes it out of clang-tidy's. A local fix to either
file would be lost the next time the port moves.

## clang-tidy

`-DV3D_CLANG_TIDY=ON`, off by default at a similar cost, with the check list in
[.clang-tidy](../.clang-tidy). The binary ships with the MSVC install, under
`VC/Tools/Llvm/x64/bin`.

Four families are enabled and 22 checks subtracted. The tree is clean at the 184 left.
[TODO.md](TODO.md#the-clang-tidy-backlog) carries what each subtraction reports, except the
seven the `.clang-tidy` comment records as settled rather than pending. Removing a line from
that table means fixing what it reports, never widening the exclusion.

`V3D_WARNINGS_AS_ERRORS` decides whether a finding stops the build, for clang-tidy as much as
for the compiler. Toggling either analyser rewrites the compile command, so ninja rebuilds what
it has to on its own.

Two traps, both silent:

- **CMake writes a system include directory as the joined `-external:I<dir>`**, which clang-cl's
  option table has as separate only. clang-tidy discards it *and every option after it* without
  saying so, which surfaces as "cannot use 'throw' with exceptions disabled" on every source
  that throws. The `--extra-arg-before=/EHsc` on the invocation restores it, because an
  extra-arg-before is applied ahead of the command rather than inside it.
- **A check name clang-tidy does not know is not an error**, which is cpplint's `--filter`
  hazard again. `.clang-tidy` enables whole families and subtracts by name, so a name that stops
  meaning anything turns findings on rather than off.

## CI

[.github/workflows/cpplint.yml](../.github/workflows/cpplint.yml) installs cpplint with pip on
an ubuntu runner and runs the command above. It is separate from
[ctest.yml](../.github/workflows/ctest.yml), which builds the tree on a Windows runner and runs
the test suites: lint needs nothing but python, where the build needs an MSVC toolchain, the
Vulkan SDK, a vcpkg install and the vendor submodule. Neither analyser runs in CI. Both are
local-only, because of the cost.
