# Types and maths

This page is for someone writing an app against the api. It covers `api/type`: geometry
queries, random numbers, checked conversions, typed flags and transforms. Terms are defined in
the [glossary](README.md#glossary).

The cameras in `api/type/camera` are in [Cameras.md](Cameras.md). The libraries beside this one
have pages of their own: models in [Models.md](Models.md), and skeletons, animation and particle
effects in [Animation.md](Animation.md).

- [What the library is](#what-the-library-is)
- [Geometry queries](#geometry-queries)
- [Random numbers](#random-numbers)
- [Flags](#flags)
- [Transform](#transform)

## What the library is

`v3dlib_type` depends on glm and nothing else. It holds no device, window or file handle, so
everything in it can be used and tested headless, and the offline renderer uses it as well as
the realtime one. The namespaces are `v3d::type` and, below it, `geometry` and `camera`.

It does not load files, and it does not draw. [Assets.md](Assets.md) covers loading, and
[rendering/](rendering/README.md) covers drawing.

## Geometry queries

The queries are in `v3d::type::geometry` ([api/type/geometry/](../../api/type/geometry/)).

| Type | What it answers |
|---|---|
| `Ray` | Where it meets an `AABBox`, a triangle or a `Plane`. |
| `Plane` | The signed distance to a point, and which side of it a point or a box is on. |
| `Frustum` | Whether a box is inside, outside or crossing the six planes of a view-projection matrix. |
| `AABBox` | An axis-aligned 3D box. Whether it contains a point or overlaps another box. |
| `Bound2D` | A 2D rectangle. Whether it contains a point or overlaps another rectangle. |

### Ray

- The constructor normalises the direction, so a distance along the ray is in world units.
- `transformed(matrix)` moves a ray into another space. The direction is transformed as a
  vector and the origin as a point, and **neither is renormalised**. A distance found in a
  mesh's own space is then comparable with one found in world space. To test a world ray against
  a mesh, pass the inverse of the mesh's matrix.
- `intersects(box, &distance)` is a slab test. A ray that starts inside the box hits it at
  distance zero.
- `intersects(a, b, c, &distance)` tests a triangle from either side. The overload with `u` and
  `v` also returns barycentric weights: the hit is `a + u * (b - a) + v * (c - a)`. Use them to
  interpolate a normal or a texture coordinate.
- **A point within a hundred-thousandth of a triangle's edge, as a weight, is inside it.**
  Rounding can put a point on an edge two triangles share just outside both, and a ray through it
  would then meet neither. Each triangle reaches that little past its edges, so a mesh has no
  cracks.
- `intersects(plane, &distance)` is true only when the ray crosses the plane at or ahead of its
  origin. A ray parallel to the plane, or one that would have to run backwards, does not cross
  it.
- Every hit is at a non-negative distance. Each `distance` pointer may be null.
- A ray whose origin or direction has a NaN or infinite component misses everything. Every
  `intersects()` overload returns false for it and leaves its outputs unchanged.

### Plane, boxes and bounds

- A `Plane` is stored as one equation, `ax + by + cz + d`. `normal()` is unit length only if
  the plane has been normalised.
- `classify()` on a point or a box returns `POSITIVE`/`INSIDE`, `NEGATIVE`/`OUTSIDE` or
  `ON_PLANE`/`CROSSING`.
- For `AABBox` and `Bound2D`, **edges and faces count as inside**. Two boxes that touch at a
  face overlap, and a point on an edge is contained.

### Frustum

`Frustum(viewProjection, depth)` extracts the six planes of a matrix. Each plane faces inward.
The frustum is in whatever space the matrix reads: a projection alone gives eye space, and a
projection times a view gives world space.

`planes()` returns them in the order clip-space -x, +x, -y, +y, near, far. The cameras in
`api/type` build a y-down clip space, so for them the -y plane is the top of the screen and the
+y plane the bottom.

**Tell it the depth range of the matrix.** The default, `Depth::ZeroToOne`, matches the
matrices `type::camera::Camera` builds. moya's matrices use `Depth::MinusOneToOne`. With the
wrong one, the near plane is placed behind the true one. The frustum then keeps boxes it should
cull, which looks correct and is hard to notice.

`intersect(box)` is conservative. A box near a frustum corner can be outside the frustum while
inside every plane separately, and it is reported as `CROSSING`. The test never culls a box it
should keep.

### Picking the ground

`Camera::pick(cursor, viewport, plane)` returns where a click lands on a plane, such as the
ground. It casts `ray()` and intersects the ray with the plane, and returns nothing when the ray
does not cross the plane ahead of the near plane. The caller passes the plane, because which way
is up is the caller's choice. Code that clips a renderer's own primitive against a plane belongs
to that renderer, as moya's `Polygon::clip` does.

## Random numbers

`v3d::type::Random` ([api/type/Random.h](../../api/type/Random.h)) is the api's source of random
numbers.

- **A seed fixes every value it gives**, on every compiler and standard library. The values are
  derived in the class, not through `<random>`, whose distributions differ between standard
  libraries. A test can therefore pin exact values.
- The generator is splitmix64. Its whole state is one 64-bit integer, and every seed is valid,
  including zero.
- `state()` returns a seed that resumes the sequence from where it stands. Store it in a save
  file to restore the sequence.
- `next()` gives 64 bits. `below(n)` gives a whole number in `[0, n)` with no modulo bias, and
  throws for `n` of zero. `unit()` gives a float in `[0, 1)`. `range(low, high)` gives a float
  in `[low, high)` when `high > low`. Where the sum rounds up to `high`, it returns the largest
  float below `high` instead.
- `inside(min, max)` gives a point in a box. `cone(axis, angle)` gives a unit direction spread
  evenly within `angle` radians of `axis`.
- It is not thread-safe, and the values depend on the order of calls.

## Checked conversions

A float read from a file, the command line or a caller is made into an integer through
[api/type/Checked.h](../../api/type/Checked.h), never through a bare `static_cast`. Converting a
float that is outside an integer type's range is undefined behaviour, and NaN and infinity are
outside every range.

- `toCount(value, {.minimum, .maximum})` returns a `std::optional<uint32_t>`. `toCount(value,
  maximum)` is the same with a minimum of zero.
- `toInteger(value, {.minimum, .maximum})` returns a `std::optional<int32_t>`, for an index, a
  frame number or a handle.

Both follow the same rules:

- **NaN and both infinities are refused.**
- **A value below `minimum` or above `maximum` is refused.** The range is tested on the value as
  given, before its fraction is dropped. For example, 2.5 is refused when `maximum` is 2, and
  -0.5 is refused when `minimum` is 0.
- **A value that passes is truncated toward zero.** 7.9 becomes 7 and -2.5 becomes -2.
- A range whose `minimum` is above its `maximum` refuses every value.
- Neither function throws. A refusal is an empty optional, and the caller decides whether to
  warn, skip or fall back.

The bounds are compared as doubles. A float cannot hold 2^32 - 1, so a float comparison would
admit 2^32 itself. `toCount(value, UINT32_MAX)` accepts every float up to 4294967040, the
largest float below 2^32.

A conversion that clamps, wraps or rounds to nearest has a different rule and does not use
these functions. Examples are the scissor rectangle, the noise lattice and the sample count.

## Flags

`v3d::type::Flags<E>` ([api/type/Flags.h](../../api/type/Flags.h)) is a set of an enum's bits.
It replaces an `int` mask: it cannot hold a bit of another enum or a stray number. An enum opts
in by declaring one `operator|`:

```cpp
enum class Feature : uint32_t { Window = 1 << 0, Config = 1 << 1 };
constexpr type::Flags<Feature> operator|(Feature a, Feature b) noexcept {
    return type::Flags<Feature>(a) | b;
}
```

A single enumerator converts to a set of one. `has(bit)` tests a bit and `empty()` tests for
none. `engine::Features` is the example in the tree.

## Transform

`v3d::type::Transform` ([api/type/Transform.h](../../api/type/Transform.h)) is a position, a
quaternion rotation and a scale. ECS entities and editor meshes are both placed by it.

- `matrix()` scales first, then rotates, then translates. Scaling first keeps a non-uniform
  scale along the object's own axes. It normalises the rotation first, so a quaternion that has
  drifted off unit length through repeated turns still only turns. A rotation of zero length
  is no rotation.
- `interpolate(from, to, alpha)` lerps position and scale and slerps the rotation the short way
  round.
