---
name: code-comments
description: Write, review, or sweep code comments in this repository. Use when writing more than a line or two of docblock or inline comment, when reviewing the comments a change adds or leaves behind, and when asked to tidy, audit or sweep the comments of a file, directory or library. Covers where the rules are, the checks that enforce them, and how to run a comments-only sweep.
---

# Code comments

## The rules

The rules are in `docs/contributing/Conventions.md`, and this skill does not repeat them. Read
two sections before writing or judging a comment:

- **`#writing`** covers plain words, short sentences, present tense, and nothing the reader
  cannot open. It applies to comments and documents alike.
- **`#comments`** covers what earns a comment, and what must stay out of one. A comment never
  cites an ADR, a document, a plan or a milestone. It does not count things kept elsewhere. It
  does not carry history. Temporary code names the condition that ends it.

Two checks enforce part of this. `node scripts/prose.ts` checks the writing rules on the lines a
change adds. `node scripts/commentsonly.ts` checks that a change moved only comments.
`docs/contributing/Linting.md` describes both.

## Writing a comment

1. Decide whether the code needs one. A comment records what the code cannot say: a unit, a
   range, ownership, what null or empty means, an ordering requirement, a trap. A comment that
   restates the next line is deleted, not reworded.
2. Write the rule, not the argument for it. The argument lives in the ADR.
3. Check every claim against the code before keeping it. Grep for any function, type or file
   the comment names.
4. Run `node scripts/prose.ts`, and fix what it reports.

For example, a comment in the house's old register:

```cpp
// the renderer wants the frame index here rather than at submit, which is what keeps the
// three per-frame buffers from being overwritten while the GPU still reads them (ADR-00NN)
```

The same fact, stated plainly:

```cpp
// set before recording, so that no per-frame buffer is written while the GPU reads it
```

## Sweeping existing comments

A sweep revises the comments of a file, a directory or a library, and changes nothing else.

1. **Verify every claim before rewording it.** The most valuable findings in a sweep are
   comments that are false: they name a removed function, say nothing reads a value that
   something now reads, or describe a behaviour the code no longer has. Grep before trusting.
   When a false comment points at a real defect, record the defect in `docs/TODO.md` rather
   than fixing the code in the sweep.
2. **Change comments only.** A sweep that also touches code is two changes in one.
3. **Check that only comments moved:**

   ```
   node scripts/commentsonly.ts
   node scripts/prose.ts
   ```

   The first compares the working tree with `HEAD` and reports any code line that differs. Pass
   `--base REF` once the sweep is committed. Then build and run the tests anyway, because a
   comment can still break the build in a way the script does not see.
4. **Report false comments apart from the rewording.** List each comment that was factually
   wrong, what it claimed, and what the code does. That list is what a reviewer most needs.
