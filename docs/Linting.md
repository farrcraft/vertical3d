
# Linting

Code is linted against the [Google C++ style guide](https://google.github.io/styleguide/cppguide.html) using the Cpplint tool.

This is the tool command for Visual Studio integration:

> pip install cpplint

>  C:\Python311\python.exe c:\Python311\lib\site-packages\cpplint.py --linelength=180 --filter=-runtime/indentation_namespace,-build/namespaces_literals --output=vs7 $(ItemPath)

To run the linter on everything from CLI:

>  C:\Python311\python.exe c:\Python311\lib\site-packages\cpplint.py --linelength=180 --filter=-runtime/indentation_namespace,-build/namespaces_literals --exclude=vendor --exclude=vault --exclude=out --exclude=voxel/src/noise --recursive .

# CI Pipeline

The linter is configured to run as part of a GitHub Actions workflow using [this action](https://github.com/cpplint/GitHub-Action-for-cpplint).
