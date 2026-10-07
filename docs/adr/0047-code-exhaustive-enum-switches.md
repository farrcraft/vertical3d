# ADR-0047: Code: exhaustive enum switches

**Status**: accepted
**Date**: 2026-09-08
**Documented in**: [Conventions.md](../contributing/Conventions.md#api-design)

## Context

Several parts of the tree switch over the same enum in different files. In `api/ui`, adding a
component type means editing the type enum, the config loader, the painter, the layout code and
the input handling. A `default:` label lets a new enumerator fall through silently. A component
added to the enum and nowhere else compiles, draws nothing, takes no input and cannot be named in
a config, and nothing reports it at build time or at run time.

## Decision

A `switch` over an enum handles every enumerator and has no `default:`. MSVC warning C4062
reports an unhandled enumerator in exactly that shape, and the tree enables it with `/w14062` and
makes it an error with `/WX`. Enumerators that need no action are listed explicitly, grouped
under a comment that says why.

## Alternatives

### A registry: one table whose row names each aspect of a component
- **For**: Adding a component becomes one row, with nowhere to forget.
- **Against**: Painting and layout read the renderer's and the arranger's own state, such as the
  style resolver and the text measuring callback. A table of free functions would need that state
  made public. The table must also be built before any component, which is a static
  initialisation order problem in a library an app links.
- **Rejected because**: It reduces the number of places at the cost of exposing private state,
  and the number of places was not the problem. The problem was that nothing checked them.

### Virtual functions on the component, such as a virtual `natural()`
- **For**: Each component answers for itself, and the compiler checks a pure virtual completely.
- **Against**: Every component would need the style resolver and the text measuring callback.
  It also removes only one of the places.
- **Rejected because**: It spreads style and text knowledge into every component to remove one
  switch.

### A runtime check at startup that every type is handled
- **For**: Independent of the compiler, and it could cover string lookups too.
- **Against**: It finds at run time what the compiler finds at build time, and only if something
  runs it. A consumer that never constructs a type never learns.
- **Rejected because**: It is later and weaker than the compiler check.

## Consequences

- **Gains**:
  - Adding an enumerator fails the build at every switch that does not handle it, each error
    naming the file and the line.
  - A case that does nothing is a decision written as a case label, not an omission.
  - The grouped no-op cases document which values take part in which behaviour.
- **Costs**:
  - The check is MSVC's. Another compiler builds the same code without enforcing it.
  - Switches are longer, and a reader passes many no-op labels to reach the cases that act.
  - The number of places to edit is unchanged. They are checked, not fewer.
  - A `default:` added back to any switch removes the check there silently.
  - A loop over enumerator values, such as parsing a name back to a type, depends on the last
    enumerator and is checked only by a test.
- **Revisit when**: the number of switches over one enum keeps growing, which would make a
  registry worth its cost, or the tree starts building with another compiler.
