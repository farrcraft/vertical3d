# ADR-0102: Editor: scene records are rows with JSON payloads

**Status**: accepted
**Date**: 2026-10-24
**Amends**: ADR-0073
**Documented in**: [editor/Files.md](../editor/Files.md#scene-documents)

## Context

A scene document holds three shapes of data. Every item has the same core: an id, a kind, an
order and a placement. Each kind and, later, each node type of a parametric graph has its own
parameters, and node types will be added often. Topology and other bulk data are large arrays of
fixed shape. Each shape changes over time: a field is added, renamed, or changes meaning, and a
document written before the change still has to be read.

## Decision

The core of each item is a row of typed columns, and its parameters are a JSON object in a
column of the same row. Bulk data are blobs of little-endian arrays in their own table. The
database schema has a version, migrated forward one step at a time. Each record type has a version
of its own, and its payload is migrated forward by that type's own chain of steps.

## Alternatives

### FlatBuffers or Cap'n Proto payloads
- **For**: a schema file that is checked by a compiler, zero-copy reads, and fields that can be
  added or renamed with no migration.
- **Against**: neither helps with a change of meaning, which is the change that needs a
  migration. Parameters that differ for every node type need generated code for each type, or a
  generic list of names and values that gives up the schema. SQLite cannot read inside them.
- **Rejected because**: the payloads are small, so zero-copy buys nothing, and their versioning
  covers only what JSON handles with a default.

### A typed column for every field
- **For**: the database checks every type and reference, and every field can be queried.
- **Against**: parameters that differ for every kind and node type need a table each, or a table
  of names and values, and every new field is a schema migration.
- **Rejected because**: it suits the core, and the columns hold the core, but not the payloads.

### One version for the whole document
- **For**: one chain of steps, as every other document in the tree has.
- **Against**: every new node type, and every change to one, moves the version of every document.
- **Rejected because**: record types change independently of each other and of the schema.

## Consequences

- **Gains**:
  - The database checks the core, and references between items are foreign keys.
  - A new kind or node type is new data, not a schema change, and it migrates on its own.
  - A payload can be read and queried by any SQLite tool with its JSON functions.
- **Costs**:
  - A payload's fields are checked by the editor's reader, not by the database.
  - Two levels of version, the schema's and each type's, each with its own migrations.
  - A blob's layout is fixed by its record type's version, and is stated only in the format
    document.
- **Revisit when**: payloads grow large or are read on a hot path, or are exchanged with another
  process under a strict contract.
