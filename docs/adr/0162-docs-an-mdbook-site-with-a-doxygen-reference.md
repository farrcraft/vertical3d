# ADR-0162: Docs: an mdBook site with a Doxygen reference

**Status**: accepted
**Date**: 2026-11-03
**Documented in**: docs/contributing/Conventions.md

## Context

The documents in `docs/` are Markdown read on GitHub, which has no search across them and no
navigation beyond each page's links. The tree is growing user manuals for the editor and moya
beside its reference and contributor documents, and a reader needs to search all of them at once.
The headers carry doc comments, but nothing turns them into a reference a reader can browse. The
tree's tooling runs no Python, and every tool it uses is pinned and runs the same locally as in
CI. The Markdown must stay readable on GitHub, because most readers meet it there.

## Decision

A site published to GitHub Pages is built by mdBook from the Markdown in `docs/`, which stays the
one source and keeps its relative links. Doxygen builds a reference from the public headers, and
the site links it. Both tools are pinned binaries, run by one script that CI and a developer run
alike.

## Alternatives

### Doxygen for the documents and the reference
- **For**: one tool, one configuration, and links from the documents straight into the reference.
- **Against**: its Markdown rendering and its navigation are built for a reference, not a
  manual, and pages read worse than on GitHub.
- **Rejected because**: the manuals and the contributor documents are most of what the site
  holds, and they would be its weakest part.

### VitePress for the documents, with the Doxygen reference
- **For**: the best rendering and navigation of the options, and it runs on the Node the tree
  already uses.
- **Against**: it brings a large tree of npm packages into a package.json that holds only the
  type checker, all of it third-party code to pin and update.
- **Rejected because**: mdBook gives the same split of manual and reference as a single binary.

### MkDocs or Sphinx
- **For**: the most widely used generators for project documentation, with mature themes.
- **Against**: both are Python, which the tree's tooling does not run.
- **Rejected because**: of the rule against Python, not of their merits.

### A Doxygen reference only, with the documents left on GitHub
- **For**: the least to build and keep, and no change to how the documents are read.
- **Against**: no search across the documents, and nowhere for a user manual to live but the
  repository browser.
- **Rejected because**: it does not serve the manuals the tree is growing.

## Consequences

- **Gains**: one searchable site for every reader, an API reference built from the headers, and
  documents that still read the same on GitHub.
- **Costs**: mdBook needs a `SUMMARY.md` listing every page, which drifts unless a gate checks it.
  The reference is thin until the empty doc comments in the headers are filled. Two more tools
  have to be pinned and updated.
- **Revisit when**: GitHub renders a searchable site from a directory of Markdown, or the site
  needs a feature mdBook cannot give, such as versions of the documents per release.
