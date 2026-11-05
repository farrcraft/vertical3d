# ADR-0171: Process: decision records are filed in folders by area

**Status**: accepted
**Date**: 2026-11-04
**Tags**: process, tooling, docs
**Documented in**: [adr/README.md](../README.md#how-the-records-are-kept)

## Context

The project writes a decision record most days, so the log grows by hundreds a year. In one
directory it is too many files to browse, and the index, the site's summary and each record's
links to the others are kept in step by hand. The area word in each title has drifted into
dozens of labels, many used once. Reference documents cite records by path, and a citation that
breaks loses the reasoning behind a rule.

## Decision

Each record lives in the folder of its area, numbered across every folder, and never moves once
filed. Each carries tags for every subject it binds. A script writes the index by area, the
records by tag and the site's summary from the records' header lines, and checks the records as
a review gate.

## Alternatives

### One directory, with tags and generated indexes
- **For**: no path ever changes, and a decision spanning areas has no folder to choose. Python's
  PEPs and Rust's RFCs keep hundreds to thousands of records this way.
- **Against**: the directory itself becomes unbrowsable, and every view of the log depends on
  the generated pages.
- **Rejected because**: a directory of hundreds of files is too many to work in, whatever
  indexes it.

### Folders numbered within each folder
- **For**: a folder's numbers stay short and dense. It is MADR's default for categories.
- **Against**: a number no longer names one record, so a citation such as ADR-0027 needs its
  folder as well, and the existing citations all become ambiguous.
- **Rejected because**: a decision has to be citable by its number alone.

### Moving a record when its area is split or renamed
- **For**: the folders always match the current areas.
- **Against**: each move breaks every link to the record, in documents that cite it as the
  background of a rule.
- **Rejected because**: a stable link is worth more than a tidy folder. An area that grows too
  large gives new records a folder of their own.

## Consequences

- **Gains**: each folder stays a size that can be browsed. A decision is found by area or by any
  subject it touches. The index, the tags page and the summary cannot drift from the records,
  and a link only one side names is reported.
- **Costs**: a cross-cutting decision is filed under one area and found elsewhere only by its
  tags. Every record carries a Tags line, and the vocabulary in tags.md has to be kept to the
  tags in use. Records filed before a split stay in the older folder, so one area can span two.
- **Revisit when**: a single folder grows past what can be browsed and a split by new records
  alone no longer keeps it in hand.
