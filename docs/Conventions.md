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

- **A header outside the including file's own directory is named by its path from the
  repository root**, in angle brackets: `#include <api/render/realtime/Canvas.h>`, and
  `<vertical3d/src/scene/Node.h>` for an app's own header one directory over. **A subdirectory
  is outside it too** — `<api/ui/component/Bar.h>` from `api/ui`, not `"component/Bar.h"`. Only
  a header in the same directory stays `"Neighbour.h"`.
- **The check is `grep -rn '#include "[^"]*/'`**, which should return only the generated shader
  headers below. Nothing in the build or the linter enforces this: a quoted relative include
  resolves exactly as well as a rooted one, so the only thing that finds a lapse is looking for
  it. Grepping for `../` alone is not enough — it misses every `"subdirectory/Header.h"`.
- **A generated header is the exception, and stays quoted.**
  `#include "shaders/quad.vert.inc"` names a file `v3d_add_shader` writes into
  `${CMAKE_CURRENT_BINARY_DIR}`, which is on that target's include path and is not in the source
  tree at all, so it has no repository-root path to be named by.
- **Where that block goes is not a preference.** cpplint reads an angle-bracket include ending
  in `.h` as a *C* system header, so the project block precedes every C++ system header — after
  the file's own header or its `#pragma once`, above `<string>`. That is also where
  `<vulkan/vulkan.h>` sits.
- Third-party includes in angle brackets — boost, glm — go last, below the project's own. They
  are exempt from the rule above because their names do not end in `.h`, so the linter files
  them with the project's headers rather than with the system's.
- **One class per header**, named after it, with its out-of-line definitions in the source of
  the same name. A small struct beside the class that uses it is still a second class: an
  `Allocation` gets `Allocation.h` rather than a place in `Allocator.h`. A class nested in its
  owner stays there, and so does a helper in a source file's anonymous namespace, since neither
  is visible to anyone else. A typedef or enum goes with the class that defines its meaning.
  When the split leaves a family of headers that only make sense together — an AST's node
  types, an app's ECS components — they go in a subdirectory of their own, under the rule
  below.
- **A directory splits when its files stop sharing a reader, not when it passes a file count.**
  `api/grid` is sixteen files and 1,300 lines and wants nothing done to it, because they are
  one concept; `api/ui` had 36 files above its subdirectories doing five different jobs, and they
  are `paint/`, `input/`, `shell/` and `style/` now. A namespace follows the directory, so a
  class whose name already carries the group word drops it — `pipeline::Builder`, not
  `pipeline::PipelineBuilder`. Where the group *is* the noun, the name stays: `device::Device`.
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
- A `boost::shared_ptr`, a `std::string` or any other non-trivial type is a parameter by `const`
  reference. `performance-unnecessary-value-param` is satisfied by a by-value parameter that is
  moved from as well, and this tree takes the reference in both cases.
- Log through the spdlog wrapper: `logger_->get()->info("... {}", value)`. The older
  `LOG_INFO` and `LOG_ERROR` macros survive only in commented-out or non-compiling code. Do
  not add new uses.
- Doc comments are `/** **/` blocks, often left empty above trivial members.

## Writing

These rules apply to code comments and to every document in [docs/](.) alike.

- **Plain words, literal statements.** Say what a thing does or requires. Avoid aphorisms
  ("a character is not a key"), inverted sentences ("X, which is what Y") and personification.
  Code requires, stores, returns or receives; it does not want, know, owe or trust.
- **Short sentences, one idea each.** Aim for about 25 words. Split anything over 35.
- **Define a term before using it,** or use the standard word for it. In-house shorthand such
  as "seam", "the walk" or "the tier" means nothing to a new reader.
- **Present tense.** Describe the code as it is. No "used to", no "no longer", no dates, no
  "phase N", and no account of how a bug was found. History belongs in git and in the ADRs.
- **Nothing the reader cannot open.** Do not name other repositories, deleted trees such as
  rigel or v3dlibs, or issue-tracker IDs. [audits/](audits/) is where the deleted trees'
  lineage lives.
- **No conversational openers** ("And ...", "So ..."), and no editorialising about how bad an
  alternative would have been.

## Comments

A comment explains the code beside it: a non-obvious invariant, a trap, a unit, or a
constraint the code satisfies. It does not narrate the change that produced the code, justify
the commit, or describe a later phase.

- **A comment stands on its own.** It never cites an ADR, a document, a plan or a milestone.
  When a rule comes from a decision, state the rule in a sentence. The reasoning and the
  rejected alternatives live in the ADR, and a reader who wants them can find it from the
  index.
- **State the rule, not the argument.** "Called once for each whole fixed step since the last
  frame" is a comment. Why a fixed step beat a variable one is not.
- **A test says what it checks**, not the history of the bug it guards against.
- **Write a shared rule once**, on the function or type that owns it. Do not paste the same
  sentence into every caller.
- Doc comments are `/** **/` blocks, often left empty above trivial members.

## Documents

A reference document states how a thing works and the rules a reader must follow, completely.
A reader should never need an ADR open to understand it. An ADR may follow a section as a
single `Background:` line, for a reader who wants the reasoning.

Each document serves one kind of reader, and [README.md](README.md) says which. A subject can
appear twice: once for the people who use it and once for the people who change it. Each fact
still has one home per reader, and the other document links to it rather than copying it.

## API design

**A new option keeps today's behaviour as the default.** When a consumer needs the api to
behave differently, add a parameter whose default is the current behaviour, rather than
changing the behaviour or forking the type. The swapchain format, the camera's handedness and
memory suballocation all follow this rule.

**A `switch` over an enum has no `default:`.** MSVC warning C4062 is an error in this tree,
so adding an enumerator fails the build at every switch that does not handle it.

## Commits

One concern per commit. The message says *why* where the diff does not make it obvious. Where
a commit implements a decision, it names the ADR. [sdlc.md](sdlc.md) has the rest of the
process.
