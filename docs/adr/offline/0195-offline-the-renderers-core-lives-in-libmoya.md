# ADR-0195: Offline: the renderer's core lives in libmoya

**Status**: accepted
**Date**: 2026-11-07
**Tags**: offline, build
**Documented in**: [plans/MoyaFoundation.md](../../plans/MoyaFoundation.md)

## Context

moya's rebuilt core needs a library: the device layer, the kernel code, the film and the scene
interface. Its clients are `moya.exe`, the editor and a Hydra delegate, all built in this tree,
and a pipeline elsewhere reaches moya through the delegate as a plugin, or through files.
`v3dlib_render_offline` is an api library that holds what more than one consumer needs, among
them the RIB reader and writer the editor's export uses without a renderer. `v3dlib_moya` holds
RI's graphics state and the old hiders, and is built with the apps.

## Decision

The new core lives in `v3dlib_moya`, in directories of its own beside RI's state, which becomes
the RIB client of the scene interface. `v3dlib_render_offline` keeps the code that consumers
other than the renderer need: the RIB reader and writer, and the lexer. The packages the core
uses are linked by `v3dlib_moya` alone.

## Alternatives

### New directories in `v3dlib_render_offline`
- **For**: The RIB reader and the core share a library, and the api's selection by
  `V3D_LIBRARIES` reaches it.
- **Against**: One renderer's code would sit in the library meant for shared code, and every
  consumer of the RIB reader would build OpenEXR, OpenColorIO and oneTBB.
- **Rejected because**: No consumer outside this tree builds the renderer from source, so the
  api's selection buys nothing, and the packages cost every consumer.

### A new api library for the core
- **For**: The core starts with nothing of the old in it, and the api's selection reaches it.
- **Against**: It publishes the renderer as part of the api for consumers that do not exist.
- **Rejected because**: The same reason, with a library added.

## Consequences

- **Gains**:
  - ADR-0022's rule holds as it stands: the offline library holds shared code, and the
    renderer's code lives with the renderer.
  - The RIB reader stays free of the renderer's packages.
- **Costs**:
  - Until the old core is removed, `v3dlib_moya` holds two renderers' code, and the new film
    and the old film live side by side.
  - portable.yml's clang job builds only api libraries, so it has to be widened to build and
    test `v3dlib_moya` under the sanitizers.
  - A renderer SDK for other programs, if one is ever wanted, is a separate packaging of this
    library, not a selection from the api.
- **Revisit when**: another repository needs to build moya's core from source.
