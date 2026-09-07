# Conventions

House style. Much of this code traces back to the early 2000s and is being modernised
incrementally, so how modern a given file is varies widely. **Match the immediate neighbours**
where this document does not say otherwise.

## Files

- Headers are `.h`. Implementations are `.cpp` **or** `.cxx`, mixed even within a directory
  (`api/render/realtime/*.cpp` alongside `api/render/realtime/vulkan/*.cxx`).
- Every source and header opens with the copyright block, followed by `#pragma once` in a
  header:

  ```
  /**
   * Vertical3D
   * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
   **/
  ```

- Third-party includes in angle brackets — boost, glm — go last, below the project's own.
- [.gitattributes](../.gitattributes) enforces LF (`* text=auto eol=lf`). An editor that saves
  CRLF turns a small change into a whole-file diff, so strip the CRs rather than committing
  them.

## Language

- Namespaces mirror the `api/` path: `v3d::asset`, `v3d::render::realtime`,
  `v3d::render::realtime::vulkan`. Close them with `};  // namespace <full name>` — the
  trailing semicolon is part of the style.
- 4-space indent. Access specifiers are indented one space into the class body (` public:`,
  ` private:`).
- **A namespace body is not indented**, and a continuation line at namespace scope sits at
  column 0 with it. [Linting.md](Linting.md#namespace-indentation) has the detail and the
  reason.
- Use `boost::shared_ptr` and `boost::make_shared`, not the `std` equivalents.
- Log through the spdlog wrapper: `logger_->get()->info("... {}", value)`. The older
  `LOG_INFO` and `LOG_ERROR` macros survive only in commented-out or non-compiling code. Do
  not add new uses.
- Doc comments are `/** **/` blocks, often left empty above trivial members.

## Comments

**Comments explain the code, not the change.** No history ("this used to", "the block that was
here"), no justification for why a commit exists, no roadmap for a later phase. Why a decision
was made belongs in [adr/](adr/), and what is coming belongs in [plans/](plans/); a comment
repeating either will go stale where nobody reads it. Write comments for a non-obvious
invariant, a trap, or a constraint the code satisfies.

Three habits keep reappearing:

- **No provenance from another tree.** "which is rigel's rule", "what `v3dlibs` did here". Those
  trees are deleted, so such a reference names something no reader can open. Keep the rule and
  drop the attribution. [audits/](audits/) is where a port's lineage lives.
- **Cite an ADR, do not summarise it.** "per ADR-00NN" followed by a paragraph re-deriving the
  argument is the restatement the ADR exists to prevent. Say which record settles it, then
  state only the invariant a caller has to honour.
- **Plain register.** No conversational openers ("and ...", "so ..."), no personification, no
  editorialising about how bad the alternative would be. A comment is a note to the next
  reader.

The same rules govern the documents in this directory. Each describes the tree as it stands,
not what changed or when; dated narrative and per-commit history belong in git, the ADRs and
the plans.

## Commits

One concern per commit. The message should say *why* where the reason is not obvious from the
diff, since a diff already says what changed. Where a commit implements a decision, say which
ADR settles it rather than restating the ADR. [sdlc.md](sdlc.md) has the rest of the process.
