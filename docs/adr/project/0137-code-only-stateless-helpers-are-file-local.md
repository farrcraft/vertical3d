# ADR-0137: Code: only stateless helpers are file-local

**Status**: accepted
**Date**: 2026-10-30
**Tags**: code
**Documented in**: [Conventions.md](../../contributing/Conventions.md#what-a-source-file-keeps-to-itself)

## Context

A source file needs somewhere to put the functions, constants and types that its header does not
declare. An anonymous namespace gives them internal linkage, so they cannot collide with another
file's, and it keeps their types out of headers. With no limit on what it holds, it also hides
whole stateful classes, mutable globals, and constants declared between the functions that use
them. Nothing in a header shows that any of these exist, and no test can reach the classes. The
C++ Core Guidelines (SF.22) and Google's style guide put every internal entity in an unnamed
namespace, while LLVM's standards keep that namespace as small as possible.

## Decision

Every class, struct, union and enum is declared in a header, and no variable at namespace scope
is mutable. A helper or constant that only one class's code uses is a private static member of
that class. Only stateless functions and constants that no single class owns stay file-local, in
one anonymous namespace at the top of the source file.

## Alternatives

### No anonymous namespaces
- **For**: every function, constant and type a file holds is declared in a header.
- **Against**: each helper's parameter types enter a header, so a header names USD, Vulkan or
  Boost types its users never touch, and they rebuild when a helper changes. A file of free
  functions has to invent a class to hold its helpers.
- **Rejected because**: it trades hidden code for coupling between headers, against every
  established guide.

### One anonymous namespace at the top, holding anything
- **For**: the smallest change from the rule it replaces, and it matches the Core Guidelines.
- **Against**: a stateful class defined in a source file still has no header, cannot be tested on
  its own, and is invisible to a reader of the library's headers.
- **Rejected because**: the hidden classes are the main cost of the rule it replaces.

### Everything a header does not declare in an anonymous namespace
- **For**: internal linkage for all of it, exactly as SF.22 asks.
- **Against**: it hides classes and state, and it says nothing about where in the file they go.
- **Rejected because**: it is the rule whose costs the Context describes.

## Consequences

- **Gains**: a library's headers name every type and all the state it has. A class's helpers are
  found in its declaration. A source file reads from top to bottom: what it uses, then what it
  defines. A gate checks the mechanical parts.
- **Costs**: a type that only one file used now has a header, so the libraries carry more headers
  and some implementation types become visible to their users. A private static helper puts its
  parameter types in its class's header. Whether only one class uses a helper is a judgment that
  the gate cannot check, so review has to.
- **Revisit when**: C++ modules build under both MSVC and Clang in this tree. A module's
  unexported declarations are private without an anonymous namespace or a header.
