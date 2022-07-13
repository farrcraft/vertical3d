# Vertical3D Ecosystem Mono Repo

The Vertical3D ecosystem is a set of libraries I wrote for supporting various 3D applications and 
games that I've toyed with writing over the years.

This a monorepo that contains all of the v3d libraries and the projects that use them. 


## History

A lot of this code can be traced back to the "early aught's" in a land before what we would call 
"modern C++". There was a single namespace, every library was its own repo/package and the build
system used autotools on unix & Visual Studio solutions/projects on Windows.

In the current iteration, everything is being moved into a monorepo, both build systems are being 
entirely replaced with CMake, namespaces are being more granular, and code is slowly being updated
to C++17 or newer.


## Linting

Code is linted against the [Google C++ style guide](https://google.github.io/styleguide/cppguide.html) using the Cpplint tool.

This is the tool command for Visual Studio integration:

> c:\Python310\lib\site-packages\cpplint.py --linelength=180 --filter=-runtime/indentation_namespace --output=vs7 $(ItemPath)
