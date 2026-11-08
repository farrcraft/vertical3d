# Traversal

This page is for anyone writing kernel code that traces rays, or a device's traversal. Rays,
hits and the tests that intersect primitives are kernel code, in `moya/libmoya/kernel/geometry/`
and the namespace `v3d::moya::kernel::geometry`. Every device compiles them. Which primitives a
ray reaches is each device's own traversal. The reyes and ray hiders use none of this.

## Rays

A `Ray` is the points `origin + t * direction` for `t` in `[start, end)`, at one `time` over
the shutter.

- **The direction need not be unit length.** A distance along a ray is measured in its
  direction's lengths, so a ray moved into an instance's space keeps its distances.
- **The interval is open at its end.** A primitive exactly at `end` is not met, so a shadow ray
  to a point on a light does not meet the light.
- **By default a ray starts at zero and has no end.**

## Hits

A `Hit` is as small as every device can return it. HIP RT, OptiX and Embree each hand back the
same fields.

- **`distance`** along the ray, and the **`time`** of the ray.
- **`primitive`**, the index within the geometry the innermost instance places. `kNoPrimitive`
  means the ray met nothing, which `found()` reports.
- **`uv`**, the position on the primitive. For a triangle it is the weight of its second and
  third vertices.
- **`instances`**, the chain of instances that placed the primitive, outermost first, and
  **`depth`**, how many there are. The chain holds at most `kInstanceDepth`, which is 4.

A hit holds no point, normal or differential. The stage that shades it computes them from the
primitive, once, rather than for every hit a nearer one replaces.

## The traversal a stage takes

A stage that traces rays takes its traversal as a template parameter, and each device
instantiates the stages with its own. The concept `Traversal` names what one provides:

- **`nearest(ray)`**: the hit nearest the ray's start within its interval, or a hit that found
  nothing.
- **`any(ray)`**: whether anything lies within the interval. It may stop at the first primitive
  it meets, as a shadow ray needs.

Both are `const`, since a launch traces from every pixel at once. Kernel code names no device,
so a test runs the same stages over any traversal.

Background: [ADR-0204](../adr/offline/0204-offline-stages-take-their-traversal-as-a-type.md)

## Triangles

`intersectTriangle` decides whether a ray meets a triangle within an interval, and where. It
meets either side.

- **The test is watertight**, by Woop, Benthin and Wald's method. A ray through an edge or a
  vertex that triangles share meets at least one of them, so a closed mesh has no gaps.
- **A `Shear` holds the ray made ready for it.** The axis the direction is longest along becomes
  z, and a shear turns the direction into +z. `Shear::of(ray)` is computed once for every
  triangle the ray meets.
- **An edge that rounds to zero is computed again in double precision**, so both triangles that
  share it decide a ray exactly on it the same way.
- **A distance that is not a number is no hit.**

## Leaving a surface

`offsetStart(point, normal)` moves the start of a ray that leaves a surface off it, along the
geometric normal, so the ray does not meet the surface again where it starts. The normal points
to the side the ray leaves by, and need not be unit length.

- **Each coordinate moves by a count of representable floats** in proportion to the normal, so
  the move grows with the point's distance from the origin, as the hit's rounding does.
- **Near the origin, where floats are densest, it moves by a fixed fraction of the normal.**

This is Wächter and Binder's method. A ray that starts at an offset point needs no `start`
above zero.
