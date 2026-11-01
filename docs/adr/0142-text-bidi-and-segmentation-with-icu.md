# ADR-0142: Text: bidi and segmentation with ICU

**Status**: accepted
**Date**: 2026-11-01
**Documented in**: [api/Fonts.md](../api/Fonts.md#segmentation)

## Context

Laying shaped runs out in lines needs each line's visual order, from the embedding levels UAX #9
resolves. Editing and wrapping need the places a caret may stop, a word ends, and a line may
break, as UAX #29 and UAX #14 define them. Thai, Lao, Khmer and Myanmar mark no word boundaries,
so their lines break only where a dictionary finds words. The api draws Arabic, Hebrew and Thai,
and a game ships every library it links.

## Decision

Bidirectional levels, reordering, grapheme clusters, words and line breaks all come from ICU.
The tree builds ICU's data from source, filtered to what these need, through an overlay of the
vcpkg port. HarfBuzz still shapes, and ICU never sees a font.

## Alternatives

### SheenBidi with libunibreak
- **For**: both are small and permissively licensed, and game engines such as Defold, SFML and
  JUCE use SheenBidi. SheenBidi also finds script runs.
- **Against**: libunibreak has no dictionary, so a Thai sentence is one unbreakable word, and its
  line breaking does not claim full conformance. Nothing in it grows into normalization, case
  folding or collation.
- **Rejected because**: it lays out the scripts the api ships incorrectly.

### Raqm
- **For**: one call itemizes, resolves bidi and shapes, over SheenBidi or FriBiDi.
- **Against**: it shapes with HarfBuzz itself, which the shaper already does, and it finds no
  line breaks or grapheme clusters.
- **Rejected because**: it duplicates the shaper and still needs a segmenter.

### ICU4X
- **For**: ICU's successor, with smaller data and dictionary and LSTM segmenters.
- **Against**: it is written in Rust, with no vcpkg port, so every platform's build needs a Rust
  toolchain.
- **Rejected because**: it adds a second compiler to the build for what ICU4C already gives.

## Consequences

- **Gains**:
  - Lines in every script the api ships break where that script's readers expect, including
    those that need a dictionary.
  - Bidi and every kind of boundary come from one library, in step with each Unicode release.
  - Normalization, case folding and collation are available when the editor searches and sorts.
- **Costs**:
  - Every app that draws text links ICU, and its data is megabytes even when filtered.
  - An overlay port in the tree has to follow the upstream port's changes.
  - ICU's bidi works in UTF-16, so offsets are mapped to and from the UTF-8 the api holds.
- **Revisit when**: ICU4X can be built without a Rust toolchain, or a library with a smaller
  footprint gains dictionary segmentation.
