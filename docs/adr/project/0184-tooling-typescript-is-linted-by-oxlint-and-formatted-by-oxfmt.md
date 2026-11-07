# ADR-0184: Tooling: TypeScript is linted by oxlint and formatted by oxfmt

**Status**: accepted
**Date**: 2026-11-06
**Tags**: tooling
**Documented in**: docs/contributing/Linting.md

## Context

The type check finds wrong types and unused code, and nothing else reads the tree's TypeScript.
A function's length, a promise left unhandled and the layout of the code are kept by hand. The
tree's type checker is TypeScript 7, which has no programmatic API, and a linter that relies on
that API cannot read the types. npm installs no package's install scripts here, and every tool is
pinned to one version. The gates still run with Node alone.

## Decision

oxlint lints every TypeScript file, with its type-aware rules run by tsgolint on TypeScript 7,
and oxfmt lays every file out. Both come from npm as pinned development dependencies, and CI runs
each next to the type check.

## Alternatives

### ESLint with typescript-eslint, and Prettier
- **For**: the most widely used stack, with the most rules and plugins, and the one most
  contributors already know.
- **Against**: typescript-eslint supports TypeScript only up to 6.0, so its type-aware rules need
  a second compiler beside the one the type check uses. It installs far more packages than the
  other options.
- **Rejected because**: the lint would read the code with a different compiler from the type
  check.

### Biome
- **For**: one binary that lints and formats, with a stable formatter, and no dependency on the
  TypeScript version.
- **Against**: its type-aware rules infer types themselves and cover only part of what the
  compiler infers. Its rules have names of their own rather than ESLint's.
- **Rejected because**: oxlint's type-aware rules read the types from the same compiler as the
  type check.

### dprint for the layout
- **For**: a mature formatter whose output depends on no other tool.
- **Against**: its npm package needs an install script, which the tree turns off, and it lints
  nothing, so a linter is still needed beside it.
- **Rejected because**: it breaks the install rule and solves only half the problem.

### A gate of the tree's own
- **For**: no third-party code, written like the other gates.
- **Against**: with no parser it can only estimate where a function ends, it has no type
  information, and it cannot lay code out.
- **Rejected because**: each rule would be an approximation that a real linter states exactly.

## Consequences

- **Gains**: a function over 100 lines, an unhandled promise and a dozen other defects fail CI,
  and the layout of every file is the formatter's. The lint and the type check read the code
  with the same compiler.
- **Costs**: oxfmt is not yet at version 1.0, so an update can change its output and reformat
  files the change did not touch. Three more packages, each with a binary per platform, must be
  kept pinned. tsgolint follows TypeScript's own versions, so the two are updated together.
- **Revisit when**: typescript-eslint supports TypeScript 7, or oxfmt reaches 1.0 or stops being
  maintained.
