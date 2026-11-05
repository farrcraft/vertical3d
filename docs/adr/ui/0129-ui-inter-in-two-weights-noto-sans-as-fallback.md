# ADR-0129: UI: Inter in two weights, Noto Sans as fallback

**Status**: amended
**Date**: 2026-10-30
**Tags**: ui
**Amended by**: [ADR-0151](0151-ui-noto-faces-for-each-script-in-the-tree.md)
**Documented in**: [api/ui/README.md](../../api/ui/README.md#text-is-supplied-by-the-app)

## Context

`TextRenderer` draws every string in one face, Noto Sans Regular, from one atlas of signed
distance field glyphs. With one weight, a title, a section header or the chosen item can differ
from body text only in size and colour. Dense tool text is read at 11 to 13 pixels, where a
face's x-height and spacing decide how readable it is. A font in the tree has to be
redistributable, and Noto Sans is under the SIL Open Font License.

## Decision

The ui draws text in Inter, Regular and SemiBold, both packed into the atlas. A code point Inter
lacks is drawn from Noto Sans Regular. A theme chooses a weight for a role, not a face.

## Alternatives

### Noto Sans in two weights
- **For**: the widest character coverage, and the look changes least.
- **Against**: Noto Sans is drawn for coverage across scripts. Inter is drawn for screen ui at
  small sizes, with a tall x-height and open spacing.
- **Rejected because**: the ui is read at sizes Inter is drawn for, and Noto Sans still covers
  the characters Inter lacks.

### One weight
- **For**: one face, one atlas, nothing to choose.
- **Against**: headers and titles can stand out only by size and colour.
- **Rejected because**: a second weight is the most common way tools and games mark hierarchy.

## Consequences

- **Gains**:
  - Titles, headers and chosen items are set in SemiBold.
  - Small text is set in a face drawn for small sizes.
- **Costs**:
  - The atlas holds three faces, which costs memory and upload time.
  - Measuring a string consults a second face for a character Inter lacks.
  - The tree carries another font under the Open Font License, with its licence file.
- **Revisit when**: the ui has to show a script neither face covers, or a variable font is
  cheaper than packing weights.
