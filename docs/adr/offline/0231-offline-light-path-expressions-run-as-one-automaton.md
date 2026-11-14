# ADR-0231: Offline: light path expressions run as one automaton

**Status**: accepted
**Date**: 2026-11-13
**Tags**: offline, compositing, lighting
**Documented in**: [offline/Integrator.md](../../offline/Integrator.md)

## Context

A light path expression names the light a channel collects, in the grammar OSL defines and
Arnold, RenderMan and Karma read. Kernel code allocates nothing and calls nothing virtual, and a
GPU's path state is held in arrays whose size is paid on every path. A light sample is matched
before its shadow ray is traced, against the light event it would add. A light group lets a
compositor change a light after rendering, and Arnold, RenderMan and Karma name one as a label on
the light's event, `<L.'key'>`. USD has no core attribute for a light's group.

## Decision

Every light path expression of a render is compiled on the host into one deterministic
automaton, whose accepting states name the channels they add to, as OSL's `accum` library builds
one. A path carries one state of it, and a light sample is matched by stepping a copy of that
state. A light names its group as a string, which the automaton reads as a label on the light's
event.

## Alternatives

### One automaton for each expression
- **For**: Each table stays the size of its own expression.
- **Against**: A path carries one state for every expression, so its state grows with each
  channel a compositor asks for.
- **Rejected because**: Path state is the cost a GPU pays on every path.

### Fixed passes recorded by the path's lobes
- **For**: A few flags on the path, and no tables.
- **Against**: Only the splits written in advance can be asked for. Caustics or one light's
  diffuse light cannot.
- **Rejected because**: Fixed splits are expressions written in advance, which the automaton runs
  as well.

### Light groups as buffers of their own
- **For**: A group's light is added without an expression, as Cycles adds it.
- **Against**: Groups and expressions become two mechanisms, and a group cannot be split by lobe.
- **Rejected because**: A label serves both, as the other renderers show.

## Consequences

- **Gains**:
  - The grammar compositors already write in other renderers.
  - One integer of path state, however many channels there are.
  - A light sample is matched without changing the path's state.
- **Costs**:
  - The automaton can grow with the product of its expressions, so the host refuses a set past a
    limit.
  - Building it on the host has to visit states in a fixed order to give one table on every run.
  - A light's group is moya's own attribute, read from each client's name for it.
- **Revisit when**: USD defines a light group of its own, or a set of expressions a production
  needs compiles past the limit.
