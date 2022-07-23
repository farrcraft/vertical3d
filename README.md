# Vertical3D Ecosystem Mono Repo

The Vertical3D ecosystem is a set of libraries I wrote for supporting various 3D applications and 
games that I've toyed with writing over the years.

This a monorepo that contains all of the v3d libraries and the projects that use them. 


## History

A lot of this code can be traced back to the "early aught's" in a land before what we would call 
"modern C++". There was a single namespace, every library was its own repo/package and the build
system used autotools on unix & Visual Studio solutions/projects on Windows.

While that is a very modular design that has worked well for other OSS projects (e.g. KDE, Gnome)
that split functionality across a larger number of smaller packages, it's much more work to maintain.

In the current iteration, everything is being moved into a monorepo, both build systems are being 
entirely replaced with CMake, namespaces are more granular, and code is targetting C++17 or newer.


## Components

* Luxa - GUI library
* Moya - Renderman-compatible rendering engine
* Rigel - An experimental v3d app implementation
* Stark - A utility library for game-related functionality
* Talyn - A raytracing renderer
* Hookah - HAL library - provides core window/keyboard/mouse services
* Odyssey - A game engine implementation


## Linting

Code is linted against the [Google C++ style guide](https://google.github.io/styleguide/cppguide.html) using the Cpplint tool.

This is the tool command for Visual Studio integration:

> c:\Python310\lib\site-packages\cpplint.py --linelength=180 --filter=-runtime/indentation_namespace --output=vs7 $(ItemPath)


## Dependencies

- [libnoise](https://github.com/eXpl0it3r/libnoise) - This is an unofficial fork that adds CMake support.


## TODO

[] Replace all of the old XML config stuff with JSON equivalents
[] refactor common json functionality out of odyssey into a common v3dlib
[] move rigel/libv3dgraph to new v3dlib v3d::dag 
[] update luxa ui loader to use json instead of xml
[] update pong to use json instead of xml
[] update tetris to use json instead of xml
[] update voxel to use json instead of xml
[] make sure rigel libv3dcore/brep doesn't have anything missing from v3dlibs/brep & remove it
[] merge rigel libv3dcommand with v3dlibs/command & remove it
[] Get tests working again
[] integrate tests into github actions
[] integrate cpplint into github actions
[] fix all of the build warnings