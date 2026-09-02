# TODO


[] Replace all of the old XML config stuff with JSON equivalents
[] update luxa ui loader to use json instead of xml
[] update pong to use json instead of xml
[] update tetris to use json instead of xml
[] update voxel to use json instead of xml
[] decide whether api/brep keeps Edge, HalfEdgeBRep and WingedEdgeBRep - they are rigel's files, reformatted, still namespace v3D, and no CMakeLists builds them (was: "make sure rigel libv3dcore/brep doesn't have anything missing from v3dlibs/brep" - there is no v3dlibs/brep)
[] decide what a Tool is in api/event - rigel's libv3dcommand is Tool and nothing else, and api/event has no equivalent (was: "merge rigel libv3dcommand with v3dlibs/command" - v3dlibs/command is itself being replaced)
[] Get tests working again
[] integrate tests into github actions
[] work out all of the size_t / unsigned int type issues - maybe need to switch to use uint64_t from unsigned int?
[] fix all of the build warnings
[] rework luxa - move into api / re-namespace / modernize ptr usage, etc
[] factor out all SDL calls from apps (e.g. odyssey) and into the api instead
[] replace OpenAL with SoLoud
