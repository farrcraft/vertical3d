# ADR-0217: Editor: a change of rotation order keeps the angles or the pose

**Status**: accepted
**Date**: 2026-11-10
**Tags**: editor, animation
**Documented in**: [docs/editor/Animation.md](../../editor/Animation.md)

## Context

An item holds its rotation as a quaternion, and shows and keys it as three angles turned in one
fixed order. An item that locks in that order cannot be keyed out of it, so each item needs an
order of its own. The angles a script passes, the properties editor shows and the keys hold are
all in that order. A change of order can therefore keep the angles and move the item, or keep the
item where it is and give it new angles. Maya, Houdini and Blender keep the angles when the order
property changes, and offer a separate conversion that keeps the pose and re-keys.

## Decision

An item turns in one of the six orders of x, y and z, saved with the item. One undoable command
changes the order, and keeps the angles and keys by default, so the item moves. Asked to keep the
pose, it gives the item and every rotation key the angles of the same rotation in the new order.

## Alternatives

### Keep the angles always
- **For**: the order is a plain property, as it is in every tool the editor's users know, and a
  script sets an order and then angles without either changing the other.
- **Against**: animation keyed in a locking order cannot be moved to a better one without
  re-keying it by hand.
- **Rejected because**: fixing a lock in keyed animation is the reason an order is changed after
  the fact, and every major tool provides a conversion for it.

### Keep the pose always
- **For**: an item never moves when its order changes.
- **Against**: a script that sets an order and then angles would find its order change rewriting
  angles it is about to replace, and the numbers in the properties editor would change under the
  user.
- **Rejected because**: it makes the common case, choosing an order before keying, surprising.

### The six orders and a quaternion mode
- **For**: a quaternion never locks, and Blender and USD both offer one.
- **Against**: four keyed numbers that have to stay normalised, curves the graph editor cannot
  show as angles, and no way to key more than a half turn between keys.
- **Rejected because**: the six orders cover every item the editor animates, and a quaternion
  mode can be added later as another order.

## Consequences

- **Gains**: an item can be keyed out of any lock, scripts and the properties editor agree on
  what the angles mean, and keyed animation can be moved to another order.
- **Costs**: a new field in every kind's payload, and a conversion that keeps the keyed poses but
  not the motion between them, since angles in another order interpolate along another path.
- **Revisit when**: the editor animates joints or cameras whose turns cross a lock in every order.
  A quaternion mode serves those.
