# ADR-0047: UI Components — A Switch Over A Component Type Is Exhaustive, And The Compiler Names Every Place One Was Forgotten

**Date**: 2026-09-08
**Status**: accepted
**Deciders**: Joshua Farr

## Context

Adding a component to `api/ui` means editing seven places: `component::Type`, the name it is
known by in a config, the loader's branch that builds one, `ComponentRenderer::paint`,
`Arranger::natural`, `ui::Cursor`'s switch and `ui::Keys`'s. `TODO.md` has carried the
complaint since the widget set was consolidated, and the sharp end of it is not the count —
it is that **the compiler checked none of them against the others**.

Every one of those switches carried a `default:`, so a new enumerator fell into it silently.
A component added to the enum and nowhere else compiled clean, drew nothing, sized itself to
the room it was in, answered no cursor and no key, and could not be named in a config at all.
Nothing said so at build time and nothing said so at run time either; the component simply was
not there. That is a defect that costs an afternoon of looking at a blank panel.

The count itself has been weighed before. `TODO.md` records that making `natural()` virtual on
`Component` was considered and left, because it removes one of the seven rather than the
problem.

## Decision

**A switch over `component::Type` handles every enumerator and carries no `default:`.** MSVC's
C4062 reports an unhandled enumerator in exactly that shape, `/w14062` turns it on, and the
tree's `/WX` makes it an error. Adding an enumerator now fails the build in all seven places at
once, each naming the file, the line and the type.

`default:` is what had to go rather than what had to be added. A `default:` label suppresses
C4062 entirely — the compiler cannot tell a case that was decided from a case that was
forgotten — so the enumerators that do nothing are listed explicitly and grouped under a
comment saying why they do nothing. That grouping is the second gain: reading `ui::command()`
now says which components carry a command and which do not, instead of leaving it to be
inferred from the absence of a case.

**The config's vocabulary is `component::name()`,** an exhaustive switch from a type to the
string a config's `"type"` names it by, with `component::parse()` the reverse. The loader
switches on the parsed type rather than comparing strings, so it is checked like the other six.
A type a config cannot ask for answers empty: a menu item is built by the menu holding it, and
`Undefined` is not a component.

`parse()` walks the enumerators up to `VerticalBox`, which is last because the enum is kept
alphabetical. That bound is the one thing here the compiler cannot check, so `TypeTest` sweeps
wider than the enum and fails if a named type is ever added past it.

## Alternatives Considered

### Alternative 1: Exhaustive switches, enforced by C4062 — **chosen**
- **Pros**: Turns seven silent omissions into seven named build errors, without restructuring
  anything. The diagnostic names the type and the file, so the compiler walks the author
  through the edit rather than leaving them to find it. The explicit no-op groups document
  which components take part in which aspect, which no `default:` can.
- **Cons**: Verbose — five switches now list every enumerator, and a component added to the
  enum touches lines in files it has nothing to do with. It is an MSVC warning number in the
  build, so the guarantee is toolchain-specific; a build with another compiler keeps the
  behaviour and loses the check.
- **Why not**: n/a — chosen.

### Alternative 2: A registry — one table whose row names a component's every aspect
- **Pros**: Genuinely reduces the seven to one. Adding a component is one row, and there is
  nowhere to forget.
- **Cons**: `paint()` and `natural()` are members that read the renderer's and the arranger's
  own state — the style resolver, the text measuring callback, the dressing. A table of free
  functions would need that state handed to it or made reachable, which turns two private
  members into a public surface. The table also has to be built before any component is, which
  is a static initialisation order problem in a library an app links.
- **Why not**: It buys the count at the price of the encapsulation, and the count was never the
  complaint.

### Alternative 3: `natural()` virtual on `Component`, and the same for the rest
- **Pros**: Each component answers for itself, which is where the knowledge belongs, and the
  compiler checks a pure virtual absolutely.
- **Cons**: `natural()` needs the arranger's measuring callback and the style resolver, so the
  component would have to be handed both — every component then knows about styles and text
  measurement. And it removes one of the seven rather than the problem, which is the reason
  `TODO.md` recorded it as weighed and left.
- **Why not**: Already declined once, for a reason that has not changed.

### Alternative 4: A runtime check — assert at startup that every type is handled
- **Pros**: Compiler-independent, and could cover the loader's string vocabulary too.
- **Cons**: Finds at run time what a compiler finds at build time, and only if something runs
  the check. A library consumer that never constructs the type never learns.
- **Why not**: Later and weaker than the thing it replaces.

### Alternative 5: Leave it, and write the seven places down
- **Pros**: No churn. The list already exists in `TODO.md`.
- **Cons**: A list in a document is exactly what did not work — the entry has been open since
  the consolidation and the places went on being missed.
- **Why not**: It is the status quo, and the status quo is the complaint.

## Consequences

### Positive
- Adding a component type is now a guided edit: build, read the seven errors, decide each one.
  Deciding "this component draws nothing" is a case label, which is a decision on the record
  rather than an omission.
- The no-op groups are documentation the compiler keeps honest. `ui::command()` says which
  components carry a command, `paint()` says which draw nothing of their own and why.
- Turning `/w14062` on found a pre-existing gap outside the ui: `voxel`'s `Chunk::hidden()`
  switches over `Voxel::BlockFace` without handling `BLOCK_FACE_ALL` or `BLOCK_FACE_NONE`. The
  two aggregates are not single faces and no caller passes one, so they are now handled
  explicitly and the behaviour is unchanged.
- The config's type names have one home. They were spread across a chain of string literals in
  the loader, where a typo was a component that silently would not load.

### Negative
- **The check is MSVC's.** `/w14062` is a compiler-specific warning number, so the guarantee
  travels only as far as the toolchain does. This tree is MSVC-only in practice, which is what
  makes that acceptable rather than merely tolerable.
- **`/w14062` is on for the whole tree**, not just `api/ui`, so any defaultless enum switch
  anywhere now has to be exhaustive. That is the intent — a switch written without a `default:`
  is one whose author meant it to be complete — but it is a rule the tree did not have before.
- Five switches got longer, and a reader skimming one now walks past a dozen no-op labels to
  reach the cases that do something.
- The places are still all of them. This makes them checked, not fewer. They have since become
  eight rather than seven: `ComponentRenderer::ringed()` arrived with the focus ring of
  [ADR-0040](0040-a-key-goes-to-a-focused-component.md) and is the one place `component::Type`
  and `style::Resolver::Class` meet — and it is checked in that direction only, so a class
  nothing rings is not a build error.

  A registry remains the only thing that would reduce the count, and it was weighed and left:
  `paint()` and `natural()` read the renderer's and the arranger's own state, so a table of free
  functions would make two private members public to save eight case labels. That is the trade
  to revisit if the count grows again, not a gap waiting to be filled.

### Risks
- **`parse()`'s upper bound is the one unchecked assumption left.** It walks to `VerticalBox`
  because the enum is alphabetical; a type added past it would be skipped while everything else
  compiled. `TypeTest` sweeps past the end of the enum to catch that, so the assumption is
  covered by a case rather than by a convention, but it is covered at run time rather than at
  build time.
- A `default:` added back to any of these switches silently removes the check for that file,
  and nothing reports it. The comment above each fallthrough says so, which is the whole of the
  defence.
