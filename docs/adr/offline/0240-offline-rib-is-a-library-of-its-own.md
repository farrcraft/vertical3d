# ADR-0240: Offline: RIB is a library of its own

**Status**: accepted
**Date**: 2026-11-15
**Tags**: offline, build, interchange
**Amends**: [ADR-0023](0023-offline-rib-is-the-scene-format.md), [ADR-0025](0025-offline-rib-reader-calls-a-typed-handler-interface.md), [ADR-0195](0195-offline-the-renderers-core-lives-in-libmoya.md)
**Supersedes**: ADR-0022
**Documented in**: [api/README.md](../../api/README.md), [offline/Rib.md](../../offline/Rib.md)

## Context

`api/render/offline` was made for code two renderers shared. With one renderer left, it holds
RIB's reader, writer, recorder and requests, the lexer the reader uses, and the pixel filters a
`PixelFilter` request names. moya reads RIB, and the editor writes it for its batches and its
export. moya links OpenEXR, OpenColorIO, oneTBB, OpenSubdiv, MaterialX and OIDN, none of which a
program that only writes RIB needs. A namespace follows its directory, and a library's name says
what it holds.

## Decision

RIB is a library of its own, `api/rib`, built as `v3dlib_rib` in the namespace `v3d::rib`. It holds
the reader, the writer, the recorder, the requests and their lexer, and names a pixel filter by RI's
name alone. The filters' weights belong to moya's film.

## Alternatives

### `api/render/offline` kept, smaller
- **For**: No library boundary moves.
- **Against**: Its name and namespace describe shared rendering code, and it would hold a file
  format. A filter namespace would hold one enumeration and no film.
- **Rejected because**: A library named for a role it no longer has invites code with no other
  home.

### RIB folded into libmoya
- **For**: RIB is moya's format, and the editor links libmoya already.
- **Against**: Every program that writes RIB links the renderer's packages, and the manifest can no
  longer select RIB alone.
- **Rejected because**: A format's reader and writer belong apart from a renderer whenever
  another program uses them, as Aqsis, OSL, MaterialX and USD keep theirs.

## Consequences

- **Gains**:
  - The editor, and any tool of the suite, writes RIB without the renderer's packages.
  - The library's name, directory and namespace say what it holds.
  - Its fuzzer and its Linux build stay those of an api library.
- **Costs**:
  - Every include and name of the RIB code changes once.
  - Two programs in the tree use it, so the boundary serves fewer than it might.
- **Revisit when**: USD replaces RIB as the suite's way out, and nothing but moya reads RIB.
