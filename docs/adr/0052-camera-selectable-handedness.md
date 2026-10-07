# ADR-0052: Camera: selectable handedness

**Status**: accepted
**Date**: 2026-09-12
**Amends**: [ADR-0012](0012-camera-projection-targets-vulkan-clip-space.md)
**Documented in**: [api/Types.md](../api/Types.md)

## Context

[ADR-0012](0012-camera-projection-targets-vulkan-clip-space.md) fixed the camera's basis as
`right = up × direction`. `glm::lookAt` uses `right = direction × up`. From the same eye, centre
and up the two give opposite right vectors and the same up vector. Each image is therefore the
horizontal mirror of the other, and a mirror reverses the winding of every front face. An app
whose meshes are wound for `glm::lookAt` and that culls back faces sees nothing through this
camera, so it cannot adopt `type::camera` without rewinding all its geometry. A mirror is not a
rotation, so no quaternion can represent the `glm::lookAt` basis.

## Decision

A `Profile` carries a hand: the default is ADR-0012's basis, and the other is `glm::lookAt`'s.
The rotation is always built from the default basis; the other hand negates the reported right
vector, and `Camera::createView()` negates view x to match. A camera behaviour that computes its
own axes, such as `Isometric`, carries the hand too, so its pans move the way the image is drawn.

## Alternatives

### Document the convention and offer no choice
- **For**: One camera, one convention, matching the one renderer, as ADR-0012 decided. Nothing
  to build.
- **Against**: An app wound the other way cannot use `type::camera` or `Isometric` at all, and
  writes and maintains its own camera beside them.
- **Rejected because**: Every app wound the common way pays that cost permanently, to avoid a
  branch in one function.

### Switch the basis to `glm::lookAt`'s and update this tree
- **For**: One convention again, and the one most outside code uses.
- **Against**: It silently changes the meaning of every camera in the tree. The editor's
  orthographic views and its picking through `project()` and `unproject()` would all need
  checking, and no app here asks for the change.
- **Rejected because**: The app that needs the other hand is the one that knows it, so it is
  the one that should name it.

## Consequences

- **Gains**:
  - An app whose geometry is wound for `glm::lookAt` can use `type::camera` and `Isometric`.
  - The hand is copied with the profile by `clone()` and assignment.
  - The rotation stays a true rotation in both hands, so `pan()`, `tilt()`, `roll()` and the
    view, which read the rotation, behave the same either way.
- **Costs**:
  - `Profile::right()` cannot be interpreted without the hand. The default hides this from
    every app in the tree, which also makes it easy to forget.
  - `Isometric::right()` and `forward()` follow the hand, so a pan built on one hand and read
    through the other moves the scene the wrong way, and nothing else looks wrong.
  - The hand takes effect at the next `lookat()`. Set on a camera that is already aimed, it
    changes nothing until then.
- **Revisit when**: the renderer's own convention moves to `glm::lookAt`'s hand, at which point
  the default would change.
