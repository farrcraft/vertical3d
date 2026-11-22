# ADR-0260: Code: a public header never includes an internal one

**Status**: accepted
**Date**: 2026-11-22
**Tags**: code, build
**Documented in**: [Conventions.md](../../contributing/Conventions.md#one-class-per-header)

## Context

A header under `internal/` is meant for its own library's sources alone. The build relies on
that: what an internal header includes is linked PRIVATE, and the configure's visibility check
skips internal headers. A header outside `internal/` that includes one hands it to every user of
that header, so a PRIVATE dependency the internal header gains reaches them unchecked, and an
edit to it rebuilds them. A class whose private members are of internal types has to name those
types in its header somehow. Qt and LLVM keep private headers out of public ones, while Abseil
and Boost let a public header include them and treat the directory only as a promise not to name
what it holds.

## Decision

A header outside `internal/` never includes a header under `internal/`. A class keeps members
of internal types behind one pointer to a private implementation, and forward-declares a type it
names only by pointer or reference. A type that a public header has to hold by value is not
internal, and moves out of `internal/`.

## Alternatives

### Internal means not named, as in Abseil and Boost
- **For**: no indirection, and members stay inline in the object. No existing header changes.
- **Against**: the PRIVATE link rule and the visibility check would have to follow includes from
  header to header. Every user of a public header still compiles against its internal headers
  and rebuilds when they change.
- **Rejected because**: the tree's build rules already give `internal/` a meaning about linking,
  and keeping that meaning needs a check of one file at a time rather than of every include
  chain.

### Forward declarations alone
- **For**: no new object, and no indirection.
- **Against**: a forward declaration serves a member held by pointer or reference. A member held
  by value, or a nested type such as a class's `Node`, cannot be declared that way.
- **Rejected because**: it fits few of the headers that break the rule. It stays the choice
  where it does fit.

### Every type a public header needs moves out of internal/
- **For**: each header shows what it holds, with no indirection.
- **Against**: implementation state that no user should name joins the API reference and the
  library's surface, and its dependencies become PUBLIC.
- **Rejected because**: most of these types are a class's working state. It stays the choice for
  a type that users do name.

## Consequences

- **Gains**: `internal/` means one thing, and a check of each file on its own confirms it. A user
  of a public header neither compiles against nor rebuilds for a library's internal headers. The
  rule that an internal header's dependencies are PRIVATE holds again.
- **Costs**: a class with internal state makes one allocation more when it is built, and reaches
  that state through a pointer. Its state is declared in a second place. It needs a destructor
  defined in its source file, and it is no longer copied without a copy written for it. Some
  types move out of `internal/` and join the reference.
- **Revisit when**: C++ modules build under both MSVC and Clang in this tree. A module's
  unexported partitions keep a class's state private without the pointer.
