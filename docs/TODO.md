# TODO

Loose ends that are not phased work. Everything scheduled lives in
[plans/Modernization.md](plans/Modernization.md), whose "What is left" section carries the
open items the phases closed around.

[x] work out all of the size_t / unsigned int type issues - api/brep names an index with one type, `brep::Index`, as of 2026-09-04. It is uint32_t rather than uint64_t: a half edge holds four and a mesh is mostly half edges. Nothing else in the tree mixed the two.
[x] fix all of the build warnings - a clean build reported 72 at MSVC's default /W1 and reports none as of 2026-09-04. Raising to /W3 or /W4 has never been tried and would find more.
[] factor out all SDL calls from apps and into the api instead - three left: `odyssey/Odyssey.cpp` and `odyssey/engine/Engine.cpp` include `SDL3/SDL.h` directly, and `voxel/src/Controller.cxx` reaches through `window_->sdl()` for `SDL_GetWindowFlags`
[x] decide whether api/brep keeps Edge, HalfEdgeBRep and WingedEdgeBRep - decided 2026-09-04. `Edge` and `WingedEdgeBRep` are ported and built, with suites; `HalfEdgeBRep` is deleted, because `BRep` is what it became.

## Done

[x] Replace all of the old XML config stuff with JSON equivalents - the config layer is JSON throughout, and pong, tetris, voxel and odyssey are all on the indirect `{"configs": [...]}` form
[x] update pong / tetris / voxel to use json instead of xml - 2026-08-31, odyssey 2026-09-01
[x] update the ui loader to use json instead of xml - `ui::Engine::load` reads JSON, and per ADR-0020 a theme is JSON too
[x] Get tests working again
[x] integrate tests into github actions
[x] rework luxa - move into api / re-namespace / modernize ptr usage, etc - done as `api/ui`, and `luxa/` is deleted; see [LuxaAudit.md](LuxaAudit.md)
[x] replace OpenAL with SoLoud - `v3dlib_audio` is soloud, and no OpenAL call is left in the tree
[x] decide what a Tool is in api/event - settled by [ADR-0017](adr/0017-a-command-is-a-name-in-a-context.md): `Tool` stays in the editor, because no game holds a gesture open across events and one consumer is not a library
