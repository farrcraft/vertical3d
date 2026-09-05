# Linting

Code is linted against the [Google C++ style guide](https://google.github.io/styleguide/cppguide.html) using the Cpplint tool.

> pip install cpplint

This is the tool command for Visual Studio integration:

>  C:\Python311\python.exe c:\Python311\lib\site-packages\cpplint.py --linelength=180 --filter=-build/namespaces_literals --output=vs7 $(ItemPath)

To run the linter on everything from CLI:

```
cpplint --linelength=180 --filter=-build/namespaces_literals \
  --exclude=out --exclude=vendor --exclude=vcpkg_installed \
  --exclude=voxel/src/noise --recursive .
```

The first three excludes matter only locally — CI never builds, checks out no submodules and installs no ports, so it has none of those trees. A developer machine has all three, and they hold two orders of magnitude more lintable files than the project does; `vcpkg_installed/` at the repo root is the worst at 80,000-odd third party headers. `--exclude` filters what is linted and not what is walked, so the run costs an `os.walk` of the whole tree either way.

**The tree is clean at this command**, so every finding is a real one and a report of zero is the expected result.

`build/namespaces_literals` is the only suppression. Do not add another: cpplint accepts a filter naming a category it does not have and silently suppresses nothing, so an entry that stops working looks exactly like a tree that started failing — which is what `runtime/indentation_namespace` did here for as long as it took the check to be renamed `whitespace/indent_namespace` under it.

## Namespace indentation

A namespace body is not indented. That check cannot distinguish a continuation line from a declaration, so a continuation at namespace scope sits at column 0 as well:

```
Recorder::Target::Target() noexcept :
image(VK_NULL_HANDLE),
view(VK_NULL_HANDLE) {
}

const wchar_t* const charcodes =
L" !\"#$%&'()*+,-./0123456789:;<=>?"
L"@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_";
```

It only misreads a *nested* class's constructor — `Plain::Plain()` may indent its initialiser list, `Recorder::Target::Target()` may not — so the tree writes both flush for consistency rather than by rule.

# CI Pipeline

[.github/workflows/cpplint.yml](../.github/workflows/cpplint.yml) installs cpplint with pip on an ubuntu runner and runs the command above. It is separate from [ctest.yml](../.github/workflows/ctest.yml), which builds the tree on a Windows runner and runs the test suites — lint needs nothing but python, where the build needs an MSVC toolchain, the Vulkan SDK, a vcpkg install and both vendor submodules.
