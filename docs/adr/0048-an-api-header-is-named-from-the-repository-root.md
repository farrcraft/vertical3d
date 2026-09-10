# ADR-0048: Include Paths — An api Header Is Named By Its Path From The Repository Root, Inside The Tree As Well As Outside

**Date**: 2026-09-08
**Status**: accepted
**Deciders**: Joshua Farr

## Context

[ADR-0027](0027-the-api-is-consumed-as-source.md) gave every api library an include root at the
repository, so an out-of-tree consumer writes `#include <api/image/Image.h>`. It then listed the
asymmetry that leaves as a known negative and declined to remove it: *"Rewriting every in-tree
include to the second form is a whole-file diff across the tree for no build gain, so the
asymmetry is kept and a reader has to learn it."* That was right on its own terms. Three months
of tree later there are **763 relative includes across 337 files** — 206 in api library sources,
256 in api tests, 301 in the nine apps — and exactly one consumer of the include root,
`examples/starter`, which is outside the tree.

What has changed is that a gain other than a build gain has appeared. A `../` count is a fact
about where *both* files sit, so moving a header rewrites every file that names it, in libraries
and apps that were not otherwise involved. Six directories under `api/` now hold more than one
thing — `api/render/realtime/vulkan` at 46 files, `api/ui` at 36, `api/render/offline` holding
both a RIB reader and a shading language — and every one of those splits pays the whole-file diff
ADR-0027 was avoiding, once per split, in files that have nothing to do with it. The asymmetry
also costs readability where names repeat: `api/asset/loader/TextureFont.cpp` includes
`"TextureFont.h"`, `"../TextureFont.h"` and `"../../font/TextureFont.h"` — three different types
with one name, told apart by a `../` count.

## Decision

A header outside the including file's own directory is named by its path from the repository
root: `#include <api/render/realtime/Canvas.h>`. A header in the same directory stays
`"Neighbour.h"`. This holds for api library sources, their tests, and the apps, so there is one
spelling of an api header everywhere rather than one inside the tree and another outside it.

It holds for an app's own headers too — `<vertical3d/src/scene/WireframeVisitor.h>`, not
`"../scene/WireframeVisitor.h"` — so that no relative parent include survives anywhere. An app
file that named api headers one way and its own the other would carry the same two spellings
this record removes, one directory down.
ADR-0027's decision is unchanged; this record reverses the trade-off in its fourth Negative
bullet, which should carry a note pointing here.

## Alternatives Considered

### Alternative 1: `<api/…>` from the repository root, everywhere — **chosen**
- **Pros**: A move becomes a `git mv`, a `CMakeLists.txt` edit and a namespace line, which is what
  makes reorganising the six overgrown directories affordable at all. Every include says which
  library it comes from, which is the whole answer to three `Font2D.h`, three `TextureFont.h`,
  four `Type.h` and six `Engine.h`. The in-tree spelling becomes the one `examples/starter`
  already uses, so the thing a person copies to start a new app agrees with the thing they are
  copying from. Nothing is invented: the include root has been on every api target since
  ADR-0027.
- **Cons**: 763 call sites, and a diff that touches 337 files across two commits. cpplint sorts
  angle-bracket includes into the system-header group, so every converted file's include block is
  regrouped as well as rewritten.
- **Why not**: n/a — chosen.

### Alternative 2: Keep the asymmetry — ADR-0027's answer
- **Pros**: Free. It is the status quo, it compiles, and the two spellings never meet in one file
  because a file is either inside the tree or outside it.
- **Cons**: It prices every future directory move at a tree-wide diff, which is the cost this
  record exists to remove; the six splits would pay it six times rather than once. It leaves the
  documented convention and the practised one disagreeing, and leaves a reader working out which
  `TextureFont.h` is meant from a path fragment.
- **Why not**: The premise it rested on — that there is no gain — was true when nothing needed to
  move. It stopped being true when the directories outgrew themselves.

### Alternative 3: A per-library include root, so a header is `<ui/Component.h>`
- **Pros**: The shortest spelling, and it says the library.
- **Cons**: Sixteen include roots rather than one, each a first-class name in every consumer's
  search path. `<type/Camera.h>`, `<image/Image.h>` and `<log/Logger.h>` are names a vendored or
  system header can plausibly take, and a collision resolves silently to whichever root comes
  first. It also says nothing about which repository a header came from, which is the thing an
  out-of-tree consumer most needs it to say.
- **Why not**: It trades a real collision risk for four saved characters, and ADR-0027 already
  settled the root at the repository for the reason that still holds.

### Alternative 4: A `v3d/` prefix, matching the namespace
- **Pros**: The include path and the namespace would read the same — `<v3d/ui/Component.h>` for
  `v3d::ui::Component` — which is the convention most C++ libraries follow.
- **Cons**: It needs either a mirrored header tree, which is a build step and a second copy of
  every header to keep in step, or renaming `api/` to `v3d/`, which moves every file in the
  repository and breaks every path in every document, plan and audit. ADR-0027 installs nothing,
  so there is no install step where a mirror would otherwise be built for free.
- **Why not**: The cost is a tree-wide rename or a permanent build step, and the gain is that one
  path segment matches a namespace. `api/` already says what the directory is.

### Alternative 5: Angle brackets across libraries, relative within one
- **Pros**: A much smaller diff — cross-library includes are the minority — and the intra-library
  relative paths are the short ones that are least confusing.
- **Cons**: It does not buy what this record is for. Moving `Cursor.h` into `api/ui/input/` still
  rewrites every file in `api/ui` that names it, which is most of the library, and four of the
  six directory splits are entirely intra-library. It also leaves two spellings, now separated by
  a rule about library boundaries that a reader has to know to apply.
- **Why not**: It keeps the cost the decision exists to remove, in exchange for a smaller one-time
  diff.

## Consequences

### Positive
- A file move stops rewriting files that did not move. This is what makes the six directory
  splits reviewable: after the conversion each is a rename plus a build-file edit, and a reviewer
  can confirm that nothing else changed.
- An include names its library. The three `TextureFont.h` in
  `api/asset/loader/TextureFont.cpp` become `<api/asset/TextureFont.h>` and
  `<api/font/TextureFont.h>` beside a local `"TextureFont.h"`, and which is which stops being an
  inference.
- One spelling in the tree and out of it, so `examples/starter` and the nine apps stop
  demonstrating opposite conventions to whoever copies one.
- The include root acquires an in-tree consumer. It has had none since ADR-0027 created it, which
  means nothing here would notice if it broke.

### Negative
- A whole-file diff across 337 files, and any branch open across it conflicts. Two commits — one
  for `api/`, one for the apps — is the mitigation, not an escape.
- The include block of every converted file is regrouped as well as rewritten. cpplint reads an
  angle-bracket include ending in `.h` as a **C** system header, so `<api/…>` must precede every
  C++ system header — `<boost/shared_ptr.hpp>` may sit last because it does not end in `.h`, and
  `<api/log/Logger.h>` may not. [Linting.md](../Linting.md) allows no `--filter`, so the position
  is a constraint on the conversion rather than a warning to silence. It puts `<api/…>` where
  `examples/starter` and every `<vulkan/vulkan.h>` in the tree already sit.
- `git blame` gets a line of noise on every converted include, and `git log -L` across one of
  them now crosses a mechanical commit.
- Longer lines. `<api/render/realtime/vulkan/PipelineBuilder.h>` is 46 characters where
  `"../vulkan/PipelineBuilder.h"` was 28, and after the directory splits it is longer still.

### Risks
- The conversion misses files that still compile, because a relative include that resolves is
  indistinguishable to the build from a converted one. The escape hatch is a grep rather than the
  compiler: `#include "../` returns nothing under `api/` after the first commit and nothing in
  the apps after the second, and that check is cheap enough to keep running afterwards.
- A future file is written with a relative include, because nothing enforces this. Mitigated by
  the same grep and by [Conventions.md](../Conventions.md) carrying the rule; a cpplint check
  would not catch it, since `"../x.h"` is valid Google style.
- The include root itself moves, and every one of the 763 includes is wrong at once rather than
  the current handful. ADR-0027 fixed the root at the repository and this record depends on that
  holding; if an installed package is ever added per that record's Alternative 2, `<api/…>` is
  the prefix it would install under, so the dependency is in the direction that helps.
