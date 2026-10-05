# Conventions

This document is for contributors. It sets out the house style for files, code, comments and
documents. Much of the code is old and is being modernised a piece at a time, so files differ in
how modern they are. **Where this document says nothing, match the files around the one you are
editing.**

## Files

- Headers are `.h`. Implementation files are `.cpp` **or** `.cxx`, and both appear even within
  one directory (`api/render/realtime/*.cpp` beside `api/render/realtime/vulkan/*.cxx`). Use the
  extension of the neighbouring files.
- Every source file and header starts with the copyright block. A header follows it with
  `#pragma once`.

  ```
  /**
   * Vertical3D
   * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
   **/
  ```

- Files use LF line endings. [.gitattributes](../../.gitattributes) sets `* text=auto eol=lf`. If
  your editor saves CRLF, the whole file shows as changed. Convert it back to LF before
  committing.

### Includes

- **Include a header from another directory by its path from the repository root, in angle
  brackets.** For example, `#include <api/render/realtime/Canvas.h>`, or
  `#include <vertical3d/src/scene/Node.h>` for an app's own header in another directory.
- **A subdirectory counts as another directory.** From `api/ui`, write
  `<api/ui/component/Bar.h>`, not `"component/Bar.h"`.
- **Only a header in the same directory is included with quotes:** `#include "Neighbour.h"`.
- **A generated header is the exception and stays quoted.** `#include "shaders/quad.vert.inc"`
  names a file that `v3d_add_shader` writes into the build directory. It is not in the source
  tree, so it has no path from the repository root.

Nothing in the build or in cpplint enforces the include rule, because a quoted relative include
compiles just as well. To check for mistakes, run this over the source directories (not the
repository root, whose `out/` directory holds the vcpkg installs):

```
grep -rn '#include "[^"]*/' api moya pong tetris voxel odyssey vertical3d imagetool v3dshell examples
```

It should report only generated `.inc` headers. Searching for `../` alone is not enough, because
it misses `"subdirectory/Header.h"`.

**Include order matters to cpplint.** cpplint treats any angle-bracket include ending in `.h` as a
C system header. So the order in a file is:

1. The file's own header (in a source file), or `#pragma once` (in a header).
2. The project's own headers, such as `<api/...>`, together with `<vulkan/vulkan.h>`.
3. C++ standard headers, such as `<string>`.
4. Third-party headers such as boost and glm. Their names end in `.hpp` or have no extension, so
   cpplint does not treat them as C headers.

Background: [ADR-0048](../adr/0048-includes-name-headers-from-the-repository-root.md)

### One class per header

- **Each class has its own header, named after the class**, with its out-of-line definitions in
  the source file of the same name.
- A small struct used by one class is still a separate class. An `Allocation` struct goes in
  `Allocation.h`, not in `Allocator.h`.
- A class nested inside another class stays in its owner's header. A helper in a source file's
  anonymous namespace stays in that source file. Neither is visible elsewhere.
- A typedef or an enum goes in the header of the class that gives it meaning.
- When splitting leaves a family of headers that are only used together, such as an AST's node
  types or an app's ECS components, put them in a subdirectory of their own.

### Directories

- **Split a directory when its files serve different purposes, not when it reaches a certain
  size.** `api/grid` has sixteen files that all implement one concept, so it stays one
  directory. `api/ui` is split into `paint/`, `input/`, `shell/` and `style/` because those are
  separate jobs.
- A namespace follows its directory. A class in a subdirectory drops the subdirectory's name
  from its own name: `pipeline::Builder`, not `pipeline::PipelineBuilder`. When the subdirectory
  name is the class's noun, the class keeps it: `device::Device`.

## Language

- **Namespaces mirror the path under `api/`:** `v3d::asset`, `v3d::render::realtime`,
  `v3d::render::realtime::vulkan`. Close a namespace with `};  // namespace <full name>`,
  including the semicolon.
- **Indent with 4 spaces.** Indent access specifiers one space into the class body
  (` public:`, ` private:`).
- **Do not indent a namespace body.** A continuation line at namespace scope also starts at
  column 0. [Linting.md](Linting.md#namespace-indentation) shows examples and explains why.
- **Use `boost::shared_ptr` and `boost::make_shared`**, not the `std` equivalents.
- **Pass a non-trivial type by `const` reference**, including a `boost::shared_ptr` and a
  `std::string`. clang-tidy's `performance-unnecessary-value-param` also accepts a by-value
  parameter that is moved from, but this tree uses the `const` reference in both cases.
- **Log through the spdlog wrapper:** `logger_->get()->info("... {}", value)`. Do not use the
  old `LOG_INFO` and `LOG_ERROR` macros. They appear only in commented-out or non-compiling code.
- Doc comments are `/** **/` blocks. Many trivial members have an empty one.

## Writing

These rules apply to code comments and to every document in [docs/](..) alike.

- **Plain words, literal statements.** Say what a thing does or requires. Avoid aphorisms
  ("a character is not a key"), inverted sentences ("X, which is what Y") and personification.
  Code requires, stores, returns or receives; it does not want, know, owe or trust.
- **Short sentences, one idea each.** Aim for about 25 words. Split anything over 35.
- **Define a term before using it,** or use the standard word for it. In-house shorthand such
  as "seam", "the walk" or "the tier" means nothing to a new reader.
- **Present tense.** Describe the code as it is. No "used to", no "no longer", no dates, no
  "phase N", and no account of how a bug was found. History belongs in git and in the ADRs.
- **Nothing the reader cannot open.** Do not name other repositories, deleted trees such as
  rigel or v3dlibs, or issue-tracker IDs. [audits/](../audits) is where the deleted trees'
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

Each document serves one kind of reader, and [README.md](../README.md) says which. A subject can
appear twice: once for the people who use it and once for the people who change it. Each fact
still has one home per reader, and the other document links to it rather than copying it.

## API design

**A new option keeps today's behaviour as the default.** When a consumer needs the api to
behave differently, add a parameter whose default is the current behaviour, rather than
changing the behaviour or forking the type. The swapchain format, the camera's handedness and
memory suballocation all follow this rule.

**A `switch` over an enum has no `default:`.** MSVC warning C4062 is an error in this tree,
so adding an enumerator fails the build at every switch that does not handle it.

- List every enumerator. Ones that need no action are grouped together under a comment that
  says why.
- Adding a `default:` back silently turns the check off for that switch.
- Where an enum is parsed from text, the parser's upper bound is the last enumerator. Keep the
  bound and the enum in step, and keep a test that checks the round trip.

## Commits

One concern per commit. The message says *why* where the diff does not make it obvious. Where
a commit implements a decision, it names the ADR. [sdlc.md](../sdlc.md) has the rest of the
process.
