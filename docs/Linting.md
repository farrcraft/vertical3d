# Linting

Code is linted against the [Google C++ style guide](https://google.github.io/styleguide/cppguide.html) using the Cpplint tool.

> pip install cpplint

This is the tool command for Visual Studio integration:

>  C:\Python311\python.exe c:\Python311\lib\site-packages\cpplint.py --linelength=180 --filter=-whitespace/indent_namespace,-build/namespaces_literals --output=vs7 $(ItemPath)

To run the linter on everything from CLI:

```
cpplint --linelength=180 --filter=-whitespace/indent_namespace,-build/namespaces_literals \
  --exclude=out --exclude=vendor --exclude=vcpkg_installed \
  --exclude=voxel/src/noise --recursive .
```

The first three excludes matter only locally — CI never builds, checks out no submodules and installs no ports, so it has none of those trees. A developer machine has all three, and they hold two orders of magnitude more lintable files than the project does; `vcpkg_installed/` at the repo root is the worst at 80,000-odd third party headers. `--exclude` filters what is linted and not what is walked, so the run costs an `os.walk` of the whole tree either way.

**The tree is clean at this command**, so every finding is a real one.

`whitespace/indent_namespace` is suppressed because the house style indents inside a namespace and the check is Google's rule that it should not. Do not reformat to satisfy it, and do not widen the filter past these two entries.

That check is worth knowing about for a second reason: cpplint accepts a filter naming a category it does not have and silently suppresses nothing. This filter spelled it `runtime/indentation_namespace`, which is what cpplint called it before the rename, and the result was every file in the repo reporting it — which reads as a tree that started failing rather than as a filter that stopped working.

# CI Pipeline

[.github/workflows/cpplint.yml](../.github/workflows/cpplint.yml) installs cpplint with pip on an ubuntu runner and runs the command above. It is separate from [ctest.yml](../.github/workflows/ctest.yml), which builds the tree on a Windows runner and runs the test suites — lint needs nothing but python, where the build needs an MSVC toolchain, the Vulkan SDK, a vcpkg install and both vendor submodules.
