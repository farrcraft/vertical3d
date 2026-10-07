# Audits and surveys

This directory holds the records of code trees that were folded into the current layout or
deleted. It is for anyone who needs to know what an old tree contained, or whether a feature
from it was carried forward.

An audit or survey is written before a tree is folded in or deleted. It lists what the tree
holds, what is worth keeping, and each item that has to land somewhere else first. Once the tree
is gone, the record is the only account of it, so it stays in the repository after its list is
finished. The deleted source can still be recovered with `git show <commit>^:<path>`.

- **While its list is open**, a record sits in this directory.
- **When every item is closed**, it moves to [completed/](completed/).

Every record is now complete. Besides the four tree records below, completed/ holds
[ApiDesignReview.md](completed/ApiDesignReview.md). That is a design review of `api/` rather than
a record of a tree, and [ApiDesignDebt](../plans/completed/ApiDesignDebt.md) worked through its
items.

## The records

| Record | Tree | Recover a source file with |
|---|---|---|
| [V3dlibsAudit.md](completed/V3dlibsAudit.md) | `v3dlibs/`, the old shared libraries, and where each piece landed in `api/` | `git show 68821c4^:v3dlibs/<path>` |
| [LuxaAudit.md](completed/LuxaAudit.md) | `luxa/`, the old ui library, compared with `api/ui` | `git show fe6d114^:luxa/<path>` |
| [RigelSurvey.md](completed/RigelSurvey.md) | `rigel/`, the earlier prototype of the editor | `git show 54d79e8^:rigel/<path>` |
| [VoxelSurvey.md](completed/VoxelSurvey.md) | `voxel/`, scoping its port rather than a deletion | not deleted |

Read the rigel survey before deciding that a rigel feature is already covered.

## Archived sub-projects

These trees were archived rather than carried forward. **All of them are deleted.**

- **QuantumXML**, in `vault/`: an XML parser, replaced by the JSON config code in `api/config`.
  Nothing was salvaged from it. It is the only tree that had no list of items to carry forward,
  so it has no record here.
- **Rigel**: an experimental app, and the earlier prototype of the editor. Its behaviour was
  folded into `vertical3d/` during [Modernization](../plans/completed/Modernization.md) (phase 6
  of that plan).
- **Hookah**: a hardware abstraction library for window, keyboard and mouse services. Its
  replacements are `api/render/realtime/Window` and `api/input`. It was deleted with `v3dlibs/`,
  whose SDL2 driver was the last SDL2 code in the tree.
