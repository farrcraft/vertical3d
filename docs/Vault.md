# Vault

Sub-projects that were archived rather than carried forward. **All of them are deleted**, the
last on 2026-09-04. This is the index of where each went; the surveys and audits are the
detailed records, and a source comes back with `git show <commit>^:<path>`.

* **QuantumXML** - an XML parser, superseded by the JSON config work in `api/config`. Genuinely
  archived: nothing was salvaged, and it is the one tree that had no list to work off.
* **Rigel** - an experimental v3d app implementation, and the earlier prototype of the editor.
  Not simply archived: its behaviour was folded into `vertical3d/` across Phase 6 of
  [plans/Modernization.md](plans/Modernization.md). See [RigelSurvey.md](RigelSurvey.md).
* **Hookah** - the HAL library providing window, keyboard and mouse services. Covered by
  `api/render/realtime/Window` and `api/input`; it went with `v3dlibs/`, whose SDL2 driver was
  the last SDL2 code in the tree. See [V3dlibsAudit.md](V3dlibsAudit.md).
