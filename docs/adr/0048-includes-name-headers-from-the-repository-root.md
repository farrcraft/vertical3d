# ADR-0048: Includes: name headers from the repository root

**Status**: accepted
**Date**: 2026-09-08
**Amends**: [ADR-0027](0027-build-consume-the-api-as-source.md)
**Documented in**: [Conventions.md](../contributing/Conventions.md#includes)

## Context

Every api library has an include root at the repository root
([ADR-0027](0027-build-consume-the-api-as-source.md)), so a consumer outside the tree writes
`#include <api/image/Image.h>`. Inside the tree, files used relative paths such as
`"../../api/image/Image.h"`. A relative path depends on where both files sit, so moving a header
rewrites every file that names it, including files in other libraries. Several directories under
`api/` need splitting, and each split would pay that cost. Relative paths also hide which header
is meant when names repeat: one file can include three different `TextureFont.h`.

## Decision

A header in another directory is included by its path from the repository root, in angle
brackets, such as `#include <api/render/realtime/Canvas.h>`. Only a header in the same directory
is included with quotes. The rule covers api sources, their tests and the apps, including an
app's own headers, so no relative parent include remains anywhere.

## Alternatives

### Keep relative includes inside the tree
- **For**: No change. It compiles, and a file is either inside the tree or outside it, so the two
  spellings never meet in one file.
- **Against**: Every directory move costs a diff across the tree. A reader has to work out which
  header is meant from a `../` count.
- **Rejected because**: The planned directory splits would each pay that diff.

### A per-library include root, such as `<ui/Component.h>`
- **For**: The shortest spelling, and it still names the library.
- **Against**: Many include roots instead of one, each on every consumer's search path. Names
  like `<type/Camera.h>` or `<log/Logger.h>` can collide with a vendored or system header, and a
  collision resolves silently to whichever root comes first. It also does not say which
  repository a header came from.
- **Rejected because**: The collision risk outweighs a few saved characters.

### A `v3d/` prefix matching the namespace
- **For**: The include path and the namespace read the same, as in many C++ libraries.
- **Against**: It needs either a mirrored header tree or renaming `api/` to `v3d/`. A mirrored
  tree is a build step and a second copy of every header. The rename moves every file and breaks
  every path in every document. Nothing is installed, so no install step could build the mirror.
- **Rejected because**: It costs a tree-wide rename or a permanent build step, to make one path
  segment match a namespace.

### Root paths across libraries, relative paths within one
- **For**: A much smaller change, and intra-library relative paths are short.
- **Against**: Moving a header inside a library still rewrites most of that library, and most of
  the planned splits are within one library. It keeps two spellings, separated by a rule about
  library boundaries.
- **Rejected because**: It keeps the cost the decision exists to remove.

## Consequences

- **Gains**:
  - Moving a file no longer rewrites files that did not move. A directory split is a rename, a
    build-file edit and a namespace change.
  - An include names its library, so headers with the same name are told apart.
  - The tree and `examples/starter` use one spelling, and the in-tree build exercises the
    include root.
- **Costs**:
  - Converting the tree was a large mechanical diff, which adds noise to `git blame`.
  - cpplint treats an angle-bracket include ending in `.h` as a C system header. As a result,
    `<api/...>` must come before every C++ system header, which constrains include order.
  - Include lines are longer.
  - Nothing in the build or cpplint enforces the rule, because a relative include compiles
    just as well. A grep is the only check.
- **Revisit when**: the include root moves, which would invalidate every include at once. If an
  installed package is ever added, `<api/...>` is the prefix it would install under.
