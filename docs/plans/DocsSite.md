# Docs Site — A Searchable Site, a Reference and User Manuals

The documents in `docs/` are read only on GitHub, with no search across them, no reference built
from the headers, and no guide for someone using the editor or moya rather than changing them.
This plan publishes the documents as one site with an API reference beside them, adds the user
manuals, and puts pictures of the apps in the documents and the top-level README.

## Decisions

- [ADR-0162](../adr/0162-docs-an-mdbook-site-with-a-doxygen-reference.md): mdBook builds the
  site from `docs/`, Doxygen builds the reference from the public headers, and both are pinned
  binaries run by one script.
- The Markdown in `docs/` stays the one source, and it still reads correctly on GitHub. Nothing
  in a document is written for the site alone.
- Documents are grouped by reader first, as `docs/README.md` groups them now. Within a group, a
  page is a tutorial, a how-to guide, a reference or an explanation, and the index says which.
- Everything under a `completed/` directory stays out of the site. Those records are frozen, and
  their links are not repaired.

## What blocks what

- Step 1 comes first, because every later step is checked on the site it builds.
- Step 2 comes after step 1, so that the reference is linked from a site that exists.
- Step 3 comes after steps 1 and 2, so that the first publish holds both.
- Step 4 comes after step 3, because it states the rules for what steps 1 to 3 built.
- Step 6 comes before step 7, because most pictures belong in the manuals.
- Step 8 comes last, because the top-level README shows pictures from step 7.
- Step 5 depends only on step 2, and can run beside steps 3 to 8.

## Steps

### Step 1 — The site builds from docs/

**Done.** `node scripts/docs.ts` fetches mdBook 0.5.4 into `out/tools/`, checks the archive
against its pinned SHA-256, and builds `out/docs/` from the configuration in
`scripts/docs/book.toml`. `docs/SUMMARY.md` lists every page outside `completed/` directories,
grouped by reader as `docs/README.md` groups them. The preprocessor `scripts/docs/outside.ts`
points each link the site cannot serve at the file on GitHub, sends a link to a directory to its
README, and stops the build on a link that names nothing. A link added by hand that named nothing
stopped the build. `scripts/summary.ts` checks `SUMMARY.md`, and CI runs it. Conventions.md states
the rules. The script has run on Windows only, and step 3 runs it on Linux.

### Step 2 — The reference builds from the headers

The same script fetches Doxygen at a pinned version and builds a reference from the public
headers of each api library into the site, under `reference/`. A header in an `internal/`
directory or under `tests/` is left out. The site links the reference from its index, and each
api page links its library's part of it.

### Step 3 — The site is published

A workflow builds the site on every push to `main` and publishes it to GitHub Pages. Each action
it uses is pinned to a commit, as `pinned.ts` requires. A pull request builds the site without
publishing it, so a broken link fails the pull request.

### Step 4 — The rules are written down

Conventions.md, under "The documentation site", states how the site is built since step 1. It
gains how the reference is built and how the site is published, and CLAUDE.md routes to it.

### Step 5 — The public headers are documented

Doxygen reports each public class and function with no doc comment, and each empty `/** **/`
block. The comments are filled library by library, following the rules for comments in
Conventions.md. When a library has none left, the script fails on a new one in that library.

### Step 6 — User manuals for the editor and moya

`docs/manual/editor/` is for someone modelling with the editor: a tutorial that builds a first
scene, then how-to guides for modelling, animating, rendering and saving, and a reference of the
menus and key bindings. `docs/manual/moya/` is for someone rendering with moya: a tutorial that
renders a first scene, the command line, and a reference of the RIB requests and the shading
language moya reads.

What the editor and offline pages hold for a user moves into the manuals, such as the bindings
table in `docs/editor/README.md`. Those pages keep what a developer needs, and link the manual.

### Step 7 — Pictures of the apps

Each picture is made by a script or by a command the app has, so it can be made again when the
app changes. moya's pictures are rendered from scenes in the tree. A picture of a running app
comes from the renderer's capture of a frame, not from a capture of the screen. Each picture
names, beside it, the command that made it.

The editor manual, each demo's section in Demos.md and the moya manual show the apps they
describe.

### Step 8 — The top-level README

The README shows a picture of the editor, of moya's output and of the demos. It links the site.
It drops its History section, which the writing rules keep out of documents.
