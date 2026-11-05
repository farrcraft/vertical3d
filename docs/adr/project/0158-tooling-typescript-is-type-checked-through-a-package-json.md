# ADR-0158: Tooling: TypeScript is type-checked through a package.json

**Status**: accepted
**Date**: 2026-11-02
**Tags**: tooling
**Documented in**: docs/contributing/Dependencies.md

## Context

TypeScript in this repository is the language of the review gates, the fixture and asset
generators, and the editor's scripts, which are checked against a declaration file the editor
writes. Node runs a TypeScript file by erasing its types, and it checks none of them. The
compiler that checks them, and the declarations for Node's own modules, come from npm. The gates
are critical infrastructure, and running one must stay as simple as running Node.

## Decision

A `package.json` at the repository root lists the TypeScript tooling as development dependencies
at exact versions, with a committed lockfile, and CI installs it with `npm ci`. CI type-checks
every TypeScript file in the tree, the editor's declaration file included. No code that runs
imports a package, so a gate or a generator still runs with Node alone.

## Alternatives

### No type checking
- **For**: nothing to install, and no third-party code in the tooling.
- **Against**: an annotation that is wrong is never found, and nothing shows that the editor's
  declaration file is valid TypeScript.
- **Rejected because**: the gates decide what merges, and their types are their first check.

### The compiler fetched by npx in CI, with no package.json
- **For**: the tree holds no npm files at all.
- **Against**: no lockfile, so CI may check with versions no developer has run, and there is no
  local check or editor configuration.
- **Rejected because**: a check that cannot be reproduced locally is a check developers meet
  only in CI.

### Deno for all TypeScript tooling
- **For**: type checking, a linter, a formatter and a test runner in one binary, with no
  `node_modules`.
- **Against**: every gate and documented command moves off Node, the formatter cannot be
  configured, and authors of editor scripts and plug-ins work with Node and `tsc`.
- **Rejected because**: it splits the repository's tooling from the toolchain its users have.

## Consequences

- **Gains**: types are checked in every TypeScript file, locally and in CI, at the versions the
  lockfile names. Editors read the same `tsconfig` files CI checks with.
- **Costs**: the tree holds npm files, and a checkout needs `npm ci` before it can type-check.
  The development dependencies are third-party code that must be kept pinned and updated.
- **Revisit when**: Node checks types itself, or a gate needs a runtime dependency, which would
  end the rule that running one needs Node alone.
