# ADR-0081: Input: key events and commands are separate

**Date**: 2026-10-05
**Status**: accepted
**Deciders**: Joshua Farr

## Context

A device sent the key it read as an `event::Event` flagged `Type::Source`, on the same sink as
every command, and the event engine listened there, mapped the key and sent its commands from
inside the key's own publish. So every listener on `sink<Event>` heard keys and commands both and
had to filter by the flag at run time; [ADR-0017](0017-a-command-is-a-name-in-a-context.md)
records the editor handling every keypress twice before it learned to. And the order a listener
heard a key and its command in depended on the order things had connected, which a dispatcher
publishes in reverse. Pong's key capture worked by that accident. The review is
[E4](../audits/completed/ApiDesignReview.md).

## Decision

**A key is an `event::Source` and a command an `event::Event`, on two sinks.** `Source` derives
from `Event`, because a binding names both ends in the same terms and a `Mapper` keys on them, but
a listener on `sink<Event>` hears commands and nothing else. **`event::publish()` is the one way a
source is sent**: it sends the key to every listener on `sink<Source>`, then — unless one called
`consume()` — hands it to the event engine, which sends the commands it is bound to. The event
engine listens for that hand-off rather than for the key, so every listener has heard the key
before any hears the command, whatever order they connected in.

## Alternatives Considered

### Alternative 1: One type, commands delivered after the source
- **Pros**: much smaller; the ordering hazard goes.
- **Cons**: every listener still filters by a flag, and the next one to forget hears every key.
- **Why not**: it fixes the order and leaves the double handling ADR-0017 recorded.

### Alternative 2: Queue the commands on the dispatcher and flush after the source
- **Pros**: no hand-off type; the event engine stays a listener on the source.
- **Cons**: a capture cannot drop the commands its key made, because a dispatcher calls a sink's
  listeners last-connected first and the event engine has usually not queued them yet when the
  capture runs. Tried, and the test written for it failed for exactly that reason.
- **Why not**: what a capture needs depends on connection order, which is the hazard being fixed.

### Alternative 3: Two types, the event engine after every listener — **chosen**
- **Pros**: a listener takes keys or commands by the sink it names. The order is structural, and
  consuming a key is one call.
- **Cons**: see below.

## Consequences

### Positive
- No listener in the tree filters by `Type` any more: the editor's guard, the engine's quit
  handler and `GameMenu` lose theirs, and pong's capture is a listener on `sink<Source>` that
  consumes what it captures.
- [ADR-0058](0058-ui-sdl-keyboard-adapter-in-ui-shell.md)'s Alternative 4 rested on a
  listener being unable to take a key before the bindings did. A listener on `sink<Source>` now
  can, though `ui::Keys` still has no reason to move.

### Negative
- `Source` shares its consumed flag between copies, because a dispatcher hands each listener a
  copy of what it was sent and `publish()` asks the one it sent. It is the one piece of shared
  mutable state on an event.
- `Event` still carries `Type`, now redundant with the class. It is left because every ui
  component sets it on the command it sends, and removing it is churn with no consumer.

### Risks
- A source triggered straight onto the dispatcher, rather than through `publish()`, reaches its
  listeners and is never mapped. Every device and test sends through `publish()`; the escape hatch
  is that it is the one function to look for.
