# TODO

Loose ends that are not phased work. Everything scheduled lives in
[plans/Modernization.md](plans/Modernization.md), whose "What is left" section carries the
open items the phases closed around.

[] work out all of the size_t / unsigned int type issues - maybe need to switch to use uint64_t from unsigned int?
[] fix all of the build warnings
[] factor out all SDL calls from apps and into the api instead - three left: `odyssey/Odyssey.cpp` and `odyssey/engine/Engine.cpp` include `SDL3/SDL.h` directly, and `voxel/src/Controller.cxx` reaches through `window_->sdl()` for `SDL_GetWindowFlags`
[] decide whether api/brep keeps Edge, HalfEdgeBRep and WingedEdgeBRep - they are rigel's files, reformatted, still namespace v3D, and no CMakeLists builds them

## Done

[x] Replace all of the old XML config stuff with JSON equivalents - the config layer is JSON throughout, and pong, tetris, voxel and odyssey are all on the indirect `{"configs": [...]}` form
[x] update pong / tetris / voxel to use json instead of xml - 2026-08-31, odyssey 2026-09-01
[x] update the ui loader to use json instead of xml - `ui::Engine::load` reads JSON, and per ADR-0020 a theme is JSON too
[x] Get tests working again
[x] integrate tests into github actions
[x] rework luxa - move into api / re-namespace / modernize ptr usage, etc - done as `api/ui`, and `luxa/` is deleted; see [LuxaAudit.md](LuxaAudit.md)
[x] replace OpenAL with SoLoud - `v3dlib_audio` is soloud, and no OpenAL call is left in the tree
[x] decide what a Tool is in api/event - settled by [ADR-0017](adr/0017-a-command-is-a-name-in-a-context.md): `Tool` stays in the editor, because no game holds a gesture open across events and one consumer is not a library
