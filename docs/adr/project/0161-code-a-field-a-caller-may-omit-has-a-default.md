# ADR-0161: Code: a field a caller may omit has a default

**Status**: accepted
**Date**: 2026-11-03
**Tags**: code
**Documented in**: [contributing/Conventions.md](../../contributing/Conventions.md#language)

## Context

A record in this tree is built with a designated initialiser that names only the fields it sets,
and the rest take their defaults. Under `-Wextra`, which the build gives Clang and GCC, both
report an initialiser that leaves out a field with no default member initialiser. A field with
one, even an empty `{}`, is exempt. MSVC has no such warning, so code that passes every gate on
Windows fails the Clang build on Linux. clang-tidy's `readability-redundant-member-init` reports
that same `{}` as redundant, because it judges only the value.

## Decision

A record field that an initialiser may leave out has a default member initialiser, `{}` when
the type's default is the value wanted. A field without one is one every initialiser has to
name. `readability-redundant-member-init` is turned off, since it would remove the mark.

## Alternatives

### Turn the warning off for Clang and GCC
- **For**: no source changes, and clang-tidy keeps all its checks.
- **Against**: GCC has no flag for the designated case alone, so turning it off there also
  loses the check on positional initialisers. Neither compiler would report a forgotten field.
- **Rejected because**: a consumer building with GCC or Clang loses a check that finds real
  mistakes, to keep one that finds only redundancy.

### Name every field at each initialiser
- **For**: both the compilers and clang-tidy are satisfied with no configuration change.
- **Against**: every initialiser grows to list fields it does not set, and adding a field to a
  record means editing every place that builds one.
- **Rejected because**: naming only the fields that are set is how the tree builds a record.

## Consequences

- **Gains**: a header says which fields are optional and which every caller sets, and both
  non-MSVC compilers enforce it at full strength.
- **Costs**: clang-tidy no longer reports a redundant initialiser, in a constructor's list as
  well as in a class. The rule is enforced only by the Clang build on Linux, which covers the
  libraries that build there, so a record elsewhere can break it unnoticed until a consumer
  builds that library off MSVC.
- **Revisit when**: clang-tidy can exempt in-class initialisers from the check, or MSVC gains an
  equivalent warning.
