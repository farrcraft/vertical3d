# ADR-0133: Input: the most specific chord wins a key

**Status**: accepted
**Date**: 2026-10-30
**Tags**: input
**Documented in**: [api/engine/Input.md](../../api/engine/Input.md#chords)

## Context

A binding has to be able to name a key with modifiers held, such as Ctrl+Z, because a tool's
users expect its usual shortcuts. The editor also holds a modifier for a camera move and drags
with a plain mouse binding while it is down, so the plain binding has to keep firing under a
modifier. A chord and a plain binding on the same key must not both fire, or Ctrl+Z would also
send what Z alone is bound to. A rebinding is stored, saved and compared as one source name.

## Decision

A source name may put the modifier words ctrl, shift and alt before its key, as in
"ctrl+shift+z", each matching either side of the keyboard. A press fires the bindings on its
key whose modifiers are all held, and of those only the ones naming the most modifiers. A chord
claims its key, while a plain binding still fires under any modifier no chord on that key
claims. A chord's repeats and release go to what its press fired.

## Alternatives

### Exact match
- **For**: each combination of key and modifiers maps to one binding, as Blender's keymap and
  Unreal's input chords match, so a rebinding screen shows one owner per combination.
- **Against**: a plain binding stops firing whenever a modifier is held. Every binding that has
  to fire under one, such as the editor's drag, needs a flag to say it matches any modifiers,
  and one that lacks the flag fails without any message.
- **Rejected because**: the editor's camera moves depend on a plain drag firing under a held
  modifier, and a flag that is easy to forget breaks them silently.

### Any match fires
- **For**: the simplest rule, and Godot's default: every binding whose modifiers are held fires.
- **Against**: Ctrl+Z fires both undo and whatever Z alone is bound to. Godot offers an exact
  match as well for this reason.
- **Rejected because**: a chord has to stop its key's plain binding firing.

## Consequences

- **Gains**:
  - Ctrl+Z, Ctrl+Shift+Z and Ctrl+S work as users expect, and a held modifier still drives the
    camera with a plain drag.
  - A chord is one string, so rebinding, saving and reporting a source need no new form, and a
    menu can show a shortcut as it is written.
- **Costs**:
  - Which binding fires depends on which others share the key, so a binding added later can
    take a combination from one that fired before.
  - A modifier key's own bindings still fire when it starts a chord, so a held mode bound to Ctrl
    is on while Ctrl+Z is pressed.
  - The modifiers do not tell left from right.
- **Revisit when**: a binding needs a modifier on one side only, or a sequence of chords such as
  Ctrl+K then Ctrl+C.
