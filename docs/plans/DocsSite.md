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
  page is a tutorial, a how-to guide, a reference or an explanation. Each manual's first page
  says which.
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

**Done.** `scripts/docs.ts` fetches Doxygen 1.18.0 at its pinned SHA-256 and runs it after
mdBook, with `scripts/docs/Doxyfile`. The reference covers every header under `api/` but those
under `internal/` and `tests/`, with every public declaration listed whether documented or not.
`docs/README.md` and `docs/api/README.md` link it, and each library's README links its namespace.
The links use the published address, which answers once step 3 publishes.

Doxygen reports 13 errors in existing doc comments, such as a `@param` that names no parameter in
`FileChooser`, `Prompt` and `SelectList::widest`. Step 5 fixes them. The warning for a parameter
with no `@param` is off until step 5 turns it on.

### Step 3 — The site is published

`.github/workflows/docs.yml` builds the site on each pull request, and builds and publishes it to
GitHub Pages on each push to `main`. Each action it uses is pinned to a commit, and `pinned.ts`
passes.

Open: the repository's Pages source has to be set to GitHub Actions before the first publish. The
workflow has not run yet, so the build on Linux is unproven until it does.

### Step 4 — The rules are written down

**Done.** Conventions.md, under "The documentation site", states how the site and the reference
are built, what `SUMMARY.md` holds, how links are rewritten, and how the site is published.
CLAUDE.md routes to it.

### Step 5 — The public headers are documented

**Done.** The first run reported 60 types with no comment, 185 functions that documented some
parameters but not all, and 13 comments that contradicted their declarations. All are fixed, in
comments only. Six other false comments came to light and were corrected, such as two that said
displacement shaders do not run. Conventions.md states the rules under "Doc comments in a public
header".

Each public type has a doc comment, a function that documents one parameter documents each, and
no doc comment contradicts its declaration. `scripts/docs.ts` runs Doxygen a second time with no
output to find undocumented types, and fails on any break of the three. Members are not all
documented: an empty block above a trivial member stays, as Conventions.md allows.

### Step 6 — User manuals for the editor and moya

**Done.** The editor manual has a tutorial, a page of concepts, eight how-to guides, and a
reference of the menus and of the bindings. The moya manual has a tutorial with its scene in
`first.rib`, an explanation of how moya renders, nine how-to guides, and a reference of the
command line, RIB and the shading language. Every example scene in it was rendered by moya. Each
manual's first page gives each page's type. The bindings table and the user-facing parts of
the offline pages moved into the manuals, and the developer pages link them. Writing them found
eleven false statements in the developer pages, now corrected, and six defects, now in TODO.md.

`docs/manual/editor/` is for someone modelling with the editor: a tutorial that builds a first
scene, then how-to guides for modelling, animating, rendering and saving, and a reference of the
menus and key bindings. `docs/manual/moya/` is for someone rendering with moya: a tutorial that
renders a first scene, the command line, and a reference of the RIB requests and the shading
language moya reads.

What the editor and offline pages hold for a user moves into the manuals, such as the bindings
table in `docs/editor/README.md`. Those pages keep what a developer needs, and link the manual.

### Step 7 — Pictures of the apps

**Done.** `node scripts/pictures.ts` makes every picture. moya's are rendered from scenes in
`docs/manual/moya/`, with the shaders the pages print. The editor and each demo write their own
window with `--capture`, which ADR-0163 adds to every app's command line, and the editor builds
its scene from a script given with `--script`. The moya pages, the editor tutorial and each
demo's section in Demos.md show them, each with a line naming what it was made from. Taking the
editor's picture found that it crashed on every quit, which is fixed.

Each picture is made by a script or by a command the app has, so it can be made again when the
app changes. moya's pictures are rendered from scenes in the tree. A picture of a running app
comes from the renderer's capture of a frame, not from a capture of the screen. Each picture
names, beside it, the command that made it.

The editor manual, each demo's section in Demos.md and the moya manual show the apps they
describe.

### Step 8 — The top-level README

The README shows a picture of the editor, of moya's output and of the demos. It links the site.
It drops its History section, which the writing rules keep out of documents.
