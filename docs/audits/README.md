# Audits and surveys

A survey or an audit is written before a tree is folded into the current layout or deleted: what
it holds, what is worth keeping, and an itemised list of what has to land elsewhere first. The
document is the only account of that tree once it is gone, so it stays in the repo after the
list is worked off. The source itself comes back with `git show <commit>^:<path>`.

An audit sits here while its list is open and moves to [completed/](completed/) when every item
is closed. All four are complete.

## The records

| Record | Tree | Recover a source with |
|---|---|---|
| [V3dlibsAudit.md](completed/V3dlibsAudit.md) | `v3dlibs/`, the shared libraries, and where each piece landed in `api/` | `git show 68821c4^:v3dlibs/<path>` |
| [LuxaAudit.md](completed/LuxaAudit.md) | `luxa/`, the old ui library, against `api/ui` | `git show fe6d114^:luxa/<path>` |
| [RigelSurvey.md](completed/RigelSurvey.md) | `rigel/`, the earlier prototype of the editor | `git show 54d79e8^:rigel/<path>` |
| [VoxelSurvey.md](completed/VoxelSurvey.md) | `voxel/`, scoping the port rather than a deletion | not deleted |

Read the rigel survey before writing off a rigel feature as already covered.

## The archived sub-projects

Trees that were archived rather than carried forward. **All of them are deleted**, the last on
2026-09-04.

* **QuantumXML**, in `vault/` — an XML parser, superseded by the JSON config work in
  `api/config`. Genuinely archived: nothing was salvaged, and it is the one tree that had no
  list to work off, so it has no record here.
* **Rigel** — an experimental v3d app implementation, and the earlier prototype of the editor.
  Not simply archived: its behaviour was folded into `vertical3d/` across Phase 6 of
  [plans/completed/Modernization.md](../plans/completed/Modernization.md).
* **Hookah** — the HAL library providing window, keyboard and mouse services. Covered now by
  `api/render/realtime/Window` and `api/input`. It went with `v3dlibs/`, whose SDL2 driver was
  the last SDL2 code in the tree.
