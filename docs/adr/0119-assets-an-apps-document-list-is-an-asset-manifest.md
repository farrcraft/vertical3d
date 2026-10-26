# ADR-0119: Assets: an app's document list is an asset manifest

**Status**: accepted
**Date**: 2026-10-26
**Documented in**: [docs/api/engine/Config.md](../api/engine/Config.md)

## Context

An app lists its JSON documents in one file, each under a kind such as `window`, `binding` or
`world`. The engine, the shell, each game and the editor read the kinds they know, and a game
names kinds of its own. Tools that do not run the engine read documents too. The library that
holds the list today has a closed list of kinds, and it also holds the readers for three of those
documents, which belong to the libraries that use them.

## Decision

The list is an `asset::Manifest`, which loads the file it is given and holds each document under
the kind its entry names. A kind is an open name, declared by the library that reads that
document. The engine builds the manifest from `config.json`, and the readers move to the
libraries that use them.

## Alternatives

### The engine holds the list
- **For**: one owner of the app's settings file, as Godot's project settings and Unreal's config
  files have.
- **Against**: a tool that does not run the engine cannot read the list.
- **Rejected because**: a list of documents is an asset concept, and tools need it too.

### A `config` library that keeps only the list
- **For**: the list keeps its own name and library.
- **Against**: a library of one class, whose only dependency is `asset`.
- **Rejected because**: the class fits in the library it already depends on.

## Consequences

- **Gains**: a game's own kinds are no longer a special case. A tool reads an app's documents with
  only `asset`. The library that reads a document holds its reader.
- **Costs**: a misspelt kind is found at run time, not by the compiler. `asset` defines the shape of
  the list file, though not its name.
- **Revisit when**: documents need layering, such as a player's file over the app's, which would
  make the list a settings system rather than a list.
