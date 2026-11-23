# ADR-0221: Editor: a batch runs frames at once, each on a share of threads

**Status**: accepted
**Date**: 2026-11-10
**Tags**: editor, offline
**Documented in**: [docs/editor/Rendering.md](../../editor/Rendering.md)

## Context

A batch renders independent frames by running moya.exe on each. moya's old hiders run on one
thread, and its new core's CPU device takes every thread unless it is given a count. One frame at
a time leaves most of a machine idle under the old hiders, and several frames that each take every
thread oversubscribe it under the new core. Houdini's scheduler and Deadline both size local work
as a number of tasks at once and a number of threads for each. How many frames a machine runs is a
property of the machine, and the same project is opened on workstations and farm nodes.

## Decision

A batch runs a number of frames at once, each in its own moya.exe given an equal share of the
machine's threads. The number is a user setting, which a headless run can give on its command
line. Frames on a GPU run one process to the device.

## Alternatives

### One frame at a time, on every thread
- **For**: the least memory, and the order in which Blender and Maya render a sequence by default.
- **Against**: under the old hiders it uses one thread of the machine. Under the new core, the
  parts of a frame that run on one thread, such as reading its scene, leave the rest idle.
- **Rejected because**: the frames are independent, and a machine runs them faster side by side.

### Several frames at once, each on every thread
- **For**: no option for moya.exe, and each frame as fast as it can go.
- **Against**: the processes compete for the same threads, and the switching costs more than it
  gains.
- **Rejected because**: oversubscription slows the whole batch, as every scheduler that sizes
  threads per task is built to avoid.

### The number saved in the document
- **For**: a project renders the same way wherever it is opened, as Blender's threads are saved
  in the file.
- **Against**: a count chosen for a workstation's cores and memory is wrong on a farm node.
- **Rejected because**: the number describes the machine, and a headless run on a farm gives its
  own.

## Consequences

- **Gains**: a batch uses the machine under either core, and a farm node sets its own number.
- **Costs**: moya.exe takes a thread count, memory grows with each frame at once, and a frame
  takes longer alone than it would on every thread.
- **Revisit when**: moya renders a frame on several GPUs, or schedules several frames within one
  process.
