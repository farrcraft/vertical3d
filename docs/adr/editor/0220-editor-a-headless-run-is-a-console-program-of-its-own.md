# ADR-0220: Editor: a headless run is a console program of its own

**Status**: accepted
**Date**: 2026-11-10
**Tags**: editor, apps
**Documented in**: [docs/editor/Scripting.md](../../editor/Scripting.md)

## Context

A script that batches edits, exports and renders has to run on a machine with no display or GPU,
started by a shell, a build or a farm. Whatever starts it reads its output and waits for its
exit status. The editor is a Windows GUI program, so a shell neither waits for it nor sees what it
prints. Blender runs headless from its own executable with a flag. Houdini has `hython`, and Maya
has `mayapy` and a `mayabatch` beside its GUI program.

## Decision

A headless run is a console program of its own, built over the editor's library with no window,
views or device. It takes a script and an optional project, which the script can also open, and
runs the script against it. It waits for any batch the script started, and exits with a failure
when the script stops on a problem or any frame fails.

## Alternatives

### A flag on the editor's program
- **For**: one program to install and to name, as Blender has.
- **Against**: a GUI program on Windows has to attach to its parent's console to print. A shell
  does not wait for it, so a build or a farm cannot read its result without a wrapper.
- **Rejected because**: a result that a shell can read is the reason for the mode.

### The project only on the command line
- **For**: a run's command line says everything it works on.
- **Against**: a script that works on several projects, or makes a new one, needs a run for each.
- **Rejected because**: the script already opens and saves projects through commands, and the
  argument is a convenience over them.

### Exit at the end of the script
- **For**: the run ends when its script does, and a batch keeps the meaning it has in the window.
- **Against**: a batch the script started is ended with the process, so a run cannot render.
- **Rejected because**: rendering on a machine with no GPU is a main use of the mode.

## Consequences

- **Gains**: a build or a farm runs a script and reads its result as with any command-line tool,
  and the program can leave out what only the window needs.
- **Costs**: a second program to build, install and document, and every command a script can run
  has to act or refuse with no views.
- **Revisit when**: the editor runs on a platform where one program can be both a window and a
  console tool.
