# ADR-0153: Editor: scripts are TypeScript, run on QuickJS

**Status**: accepted
**Date**: 2026-11-02
**Tags**: editor
**Documented in**: docs/editor/Scripting.md

## Context

The editor's commands need a language that scripts, the console and later plug-ins are written
in. It should be one its users already know, with types an author's editor can check, and it has
to run inside the editor and without a window. A script that arrives inside a project should
reach only what the editor gives it. Figma's plug-ins and Microsoft's Office Scripts are
TypeScript, Adobe's are JavaScript, and Fusion, Lightroom and Modo use Lua.

## Decision

Scripts and console lines are TypeScript limited to syntax that can be erased. The editor strips
the types and runs the JavaScript left on QuickJS-ng, linked into the editor. The editor ships a
declaration file for its commands, against which `tsc` or an author's editor checks a script.

## Alternatives

### Luau
- **For**: Lua's small, fast, C++ runtime with gradual types and a sandbox built in.
- **Against**: fewer authors know it, its type checker is smaller, and it is a second scripting
  language beside the repository's own TypeScript.
- **Rejected because**: TypeScript reaches more authors with better tooling.

### Lua 5.4
- **For**: the most common embedded language in creative tools, with the best C++ binders.
- **Against**: no types, and a sandbox built by hand.
- **Rejected because**: scripts and plug-ins that grow need types an author can check.

### TypeScript on V8
- **For**: the fastest engine, with a full debugger.
- **Against**: a very large dependency with a build of its own.
- **Rejected because**: the editor's scripts drive commands, which do the heavy work in C++.

## Consequences

- **Gains**: one language for the repository's tooling and the editor's scripts. Authors get
  `tsc` and their editor's checking against the declaration file, and a script sees nothing the
  editor did not bind.
- **Costs**: every script and console line is stripped before it runs, and syntax that cannot be
  erased, such as `enum`, is refused. Types are never checked while a script runs. QuickJS
  interprets, so a script that does heavy work in its own code is slow, and its debugging is weak.
- **Revisit when**: scripts need a debugger or do heavy work of their own, which would make V8
  worth its size.
