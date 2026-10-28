# Cameras

This page is for someone writing an app against the api. It covers the cameras in `api/type`:
the profile, the matrices, the handedness, and the isometric orbit. Terms are defined in the
[glossary](README.md#glossary).

The camera classes are in `v3d::type::camera` ([api/type/camera/](../../api/type/camera/)):

- `Profile` holds a camera's settings. They are the eye position, the basis (`up`, `right`,
  `direction`), the rotation and the clipping distances. They are also the vertical field of
  view in degrees, the orthographic zoom, the pixel aspect ratio, the viewport size and the
  handedness.
- `Camera` holds a profile and builds the projection and view matrices from it. `zoom(z)` adds to
  the orthographic half height and stops at `Camera::MINIMUM_ZOOM`. A drag scales its next
  step by the half height, so one that reached zero could never zoom back out.
- `Isometric` is an orbit around a target on the ground, snapped to four azimuths.
- `ArcBall` turns a mouse drag into a rotation for `Camera::rotate()`.

`engine::CameraProfiles` reads named profiles from a config document.
[engine/Config.md](engine/Config.md#camera-profiles) describes the format.

## Clip space and matrices

- **`Camera` builds Vulkan clip space**: y points down the screen, and depth runs from 0 at the
  near plane to 1 at the far plane. This holds for both perspective and orthographic
  projections.
- The camera looks along `+z` of its own basis, so a point in front of it has a positive view
  `z`.
- `project()` and `unproject()` are inverses of each other.
- **`project()` places a point behind a perspective camera as though it were in front**, mirrored
  through the centre. `screenPoint(point, viewport)` returns nothing for such a point, and for a
  viewport with no area. Use it to place a label or a marker over something in the world.
- **The matrices are cached.** Call `createProjection()` and `createView()` after changing the
  profile. `projection()`, `view()` and `ray()` read the cached matrices.
- `ray(point, viewport)` casts a world-space ray from a window pixel. It starts at the near plane
  and has a unit direction. An orthographic camera gives parallel rays.
- `orthoFactorHorizontal()` and `orthoFactorVertical()` are the world units one viewport pixel
  covers. They are zero until the profile has a viewport size, set with `Profile::size()`.

## The basis and the rotation

A profile holds the three basis vectors and the rotation as separate state.

- `lookat(center)` is the one call that sets all four together. Use it to aim a camera.
- Setting `up()`, `right()` or `direction()` directly does not change the rotation.
- `rotation(q)` sets the rotation and does not change the basis vectors.
- `turn(q)` composes a rotation onto the current one and re-derives the basis vectors from the
  result. `Camera::pan()`, `Camera::tilt()` and `Camera::rotate()` all go through `turn()`, so
  a dolly or truck after a turn moves along the view's new axes.

**A view built through `lookat()` matches `glm::lookAt` exactly**, element for element, for the
same hand. `lookat()` keeps the basis matrix it built and `createView()` uses it directly, rather
than rebuilding it from the quaternion. After any other change to the rotation, `createView()`
rebuilds from the quaternion, which agrees to about 2e-6 per element. This matters only to a
consumer that compares rendered frames at zero tolerance. The up vector `lookat()` produces is
unit length to within float rounding, as `glm::lookAt`'s is.

## Handedness

**The default basis is `right = up × direction`.** That is the opposite hand to `glm::lookAt`,
which builds `right = direction × up`. From the same eye, centre and up, the two give opposite
right vectors and the same up vector, so each image is the horizontal mirror of the other. A
mirror also reverses the winding of every front face.

The screen's right is `Profile::right()`. Do not copy a world axis from code written for
`glm::lookAt`: a camera move built on the wrong hand moves the scene the wrong way, and nothing
else looks wrong.

An app whose geometry is wound for `glm::lookAt` chooses the other hand:

```cpp
profile.hand(v3d::type::camera::Profile::Hand::DirectionCrossUp);
profile.lookat(center);   // the hand takes effect here
```

- **Set the hand before `lookat()`.** The basis vectors are not recomputed until `lookat()` or
  `turn()` runs, so a hand set on a camera that is already aimed leaves `right()` reporting the
  old hand.
- The hand is copied with the profile, by `clone()` and by assignment.
- The rotation is a proper rotation in both hands. For `DirectionCrossUp`, `right()` reports the
  negated vector and `createView()` negates view `x`. `pan()`, `tilt()` and the view are
  otherwise unaffected.
- Nothing in this tree uses `DirectionCrossUp`, so `right()` in this tree's code always means
  `up × direction`.
- A renderer that culls by winding has to be told the mirrored hand's front face.
  `renderer::Lit` takes it as a constructor argument, and `lighting::shadowLight()`
  takes the hand itself. See [Lighting.md](rendering/Lighting.md#setting-it-up).

`Isometric` carries its own hand. Its `right()` and `forward()` follow it, and `apply()` writes
it onto the camera along with everything else. Set the hand on the orbit, not on the profile.

Background: [ADR-0012](../adr/0012-camera-projection-targets-vulkan-clip-space.md),
[ADR-0052](../adr/0052-camera-selectable-handedness.md)

## Isometric

`Isometric` ([api/type/camera/Isometric.h](../../api/type/camera/Isometric.h)) places an
orthographic camera on an orbit around a target on the ground.

- `rotate(steps)` turns by quarter turns, counterclockwise for positive steps. `azimuth(index)`
  jumps to one of the four azimuths. Both wrap.
- `start(radians)` sets the angle of azimuth 0 around the vertical, and the other three are
  quarter turns from it. The default of 0 puts the eye at azimuth 0 along +X of the target, so
  every view looks along a world axis and a square tile is drawn as a square.
  `Isometric::DIAGONAL_START`, 45 degrees, puts every view over a ground diagonal, so a square
  tile is drawn as a diamond.
- `pan(delta)` moves the target in the view's own axes: `x` along `right()` and `y` along
  `forward()`, in world units.
- The default elevation is 45 degrees. A true isometric projection is about 35.26 degrees;
  pass `atan(1 / sqrt(2))` to `elevation()` for it.
- `zoom()` is the orthographic half height, clamped to `[2, 40]`. `distance()` only decides
  what the near and far planes cut, and must clear the tallest thing between eye and target.
- `apply(camera)` writes the eye, orientation, zoom, orthographic flag and hand onto a camera.
  It does not set clipping, pixel aspect or viewport size, and does not rebuild the matrices.
