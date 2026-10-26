# ADR-0118: Assets: a loader is keyed by the kind it produces

**Status**: accepted
**Date**: 2026-10-26
**Documented in**: [docs/api/Assets.md](../api/Assets.md)

## Context

Loaders live in the libraries that decode their files: `asset`, `asset::media` and `audio`, and
an app may add its own. The manager has to find a loader for a request, and a caller has to
get back the class it holds the asset as. A caller already names that class when it loads, so a
key that repeats it carries nothing more. A file can lack an extension, or have one that more
than one loader reads, and two loaders can produce the same class.

## Decision

A loader is registered for the asset class it produces, with the extensions it reads. A load
picks the loader by the file's extension and checks that the result is the class asked for. A
file whose extension no loader reads is loaded by the loader registered for that class. There is
no list of asset types, and an asset does not record one.

## Alternatives

### An open name for each loader, such as "image" or "gltf"
- **For**: names read well in a log and can be written in a data file. Any library adds one
  without editing `asset`.
- **Against**: the name repeats the class the caller names anyway. A typo is a null at run time,
  and two libraries can pick one name without either knowing.
- **Rejected because**: the class is already a key the compiler checks.

### The extension alone
- **For**: no key at all. The manager is a map from extension to loader.
- **Against**: a file with no extension, or a misleading one, cannot be loaded.
- **Rejected because**: a file's name cannot be the only way to say what it holds.

### The closed list of types, kept
- **For**: every type is in one place.
- **Against**: a library or app cannot add a kind without editing `asset`. Its several image types
  have no effect, because the extension picks the decoder.
- **Rejected because**: `asset` has to be extended from outside.

## Consequences

- **Gains**: a library adds a kind by registering a loader. Every image format is one kind. A
  caller names what it holds once.
- **Costs**: a data file cannot name a kind directly, only a file. When two loaders produce one
  class, a file with no extension goes to the one registered last for that class.
- **Revisit when**: a data format has to name the kind of a file apart from its extension.
