# ADR-0081: Input: key events and commands are separate

**Status**: accepted
**Date**: 2026-10-05
**Amends**: [ADR-0043](0043-input-apps-see-raw-events-before-bindings.md)
**Documented in**: [api/engine/Input.md](../api/engine/Input.md)

## Context

A device reports a key or button edge, and the event engine maps it to the commands an app's
bindings name. When keys and commands share one event type on one dispatcher sink, every listener
hears both and has to filter by a flag at run time, and a listener that forgets handles every
keypress twice. When the event engine maps a key from inside the key's own delivery, whether a
listener hears the key or its command first depends on the order things connected, and an EnTT
dispatcher calls the last-connected listener first. A key capture, such as a rebinding screen,
needs to take a key before the key makes any command.

## Decision

A key is an `event::Source` and a command an `event::Event`, delivered on two sinks, so a listener
on `sink<Event>` hears commands and nothing else. `event::publish()` is the one way to send a
source: it delivers the key to every listener on `sink<Source>` and then, unless one called
`consume()`, hands it to the event engine to send the commands it is bound to. Every listener
therefore hears the key before any hears its command, whatever order they connected in.

## Alternatives

### One event type, with commands delivered after the key
- **For**: a much smaller change, and the ordering problem goes.
- **Against**: every listener still filters by a flag, and the next one to forget hears every key.
- **Rejected because**: it fixes the order and leaves the double handling.

### Queue the commands on the dispatcher and flush them after the key
- **For**: no hand-off step, and the event engine stays an ordinary listener on the key.
- **Against**: a capture cannot drop the commands its key made, because the dispatcher calls
  listeners last-connected first and the event engine has usually not queued them yet when the
  capture runs. A test written for this approach failed for exactly that reason.
- **Rejected because**: what a capture can do would still depend on connection order.

## Consequences

- **Gains**:
  - A listener takes keys or commands by the sink it names, and no listener filters by type.
  - The order is structural, and capturing a key is one `consume()` call.
  - A listener on `sink<Source>` can take a key before the bindings do, without overriding
    `onEvent()`.
- **Costs**:
  - `Source` shares its consumed flag between copies, because the dispatcher hands each listener a
    copy and `publish()` reads the original. It is the one piece of shared mutable state on an
    event.
  - `Event` still carries a type field that the class now makes redundant, because every ui
    component sets it on the command it sends.
  - A source triggered straight onto the dispatcher, rather than through `publish()`, reaches its
    listeners and is never mapped to a command.
- **Revisit when**: a source needs to be sent from somewhere that cannot call `publish()`, or the
  redundant type field is removed from `Event`.
