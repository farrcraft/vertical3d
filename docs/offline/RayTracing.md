# Ray tracing

The shared ray tracer in `api/render/offline/trace`. The ray-trace hider uses it to find what the
camera sees, and shaders use it for shadows, reflection and refraction.

## Ray tracing

The ray tracer is in `api/render/offline/trace`, namespace `offline::trace`. Both hiders use it.
The ray hider casts every primary ray into it. Under either hider, a shader's `trace()` and
`transmission()` calls are answered from it.

### The traced scene

- **`trace::Scene` holds primitives in world space, as one list.** A primitive is anything that
  derives from `trace::Primitive` and implements `intersect` and `describe`. `trace::Triangle`,
  `trace::Sphere` and `trace::Curve` are the three kinds. A new kind is a new class, and the
  scene does not change.
- **A curve segment is a ribbon that faces whatever ray meets it.** `trace::Curve` is one cubic
  Bezier segment with a width at each end. A ray is tested in a frame where it runs along +z.
  The segment is halved until each piece bows from straight by less than a twentieth of the
  width, at most ten times. A piece is met where the ray passes within half the width of it,
  between its ends. The normal points back along the ray, turned square to the curve, so a
  curve looks the same width from every side, and flat close up.
- **A ray is tested against the primitives near it, through a bounding volume hierarchy.**
  `Scene::nearest` builds a `trace::Hierarchy` on its first call after a primitive was added,
  with a binned surface area heuristic, and walks it nearest box first. A primitive gives its
  box through `Primitive::bound()`. One that has no bound, such as an infinite plane, returns
  false and is tested against every ray. The build happens inside a const query, so the scene is
  not safe to trace from two threads at once.
- **A moving primitive's box holds it across its whole motion.** Its stored box's corners are
  followed through 16 steps of the motion, and the box is grown by the longest step, which
  holds the arc a rotation carries a corner along. The primitive itself is still tested at the
  ray's time.
- **Two hits at one distance resolve to the primitive added first**, as a search through the
  list in order would. Each box is grown by a small fraction of its size, so a hit that rounding
  puts on a box's face is still inside it. The hierarchy therefore finds exactly the hit a
  search of every primitive finds, and adding it changed no picture.

Background: [ADR-0092](../adr/0092-offline-a-bounding-volume-hierarchy-for-traced-rays.md).
- **Every primitive a scene gives moya is added to the traced scene as it arrives**, in world
  space, before any split or dice. A polygon becomes a fan of triangles, since RI defines a
  polygon as planar and convex. The reyes hider reads the scene for `trace()` and
  `transmission()`. The ray hider reads it for everything.
- **The reyes hider drops the traced scene's primitives when no shader traces.** The shading
  language compiler marks a program that calls `trace()` or `transmission()`, directly or
  through one of the shader's own functions. When rendering starts under the reyes hider and
  no surface, displacement, light or imager shader the scene bound is marked, the primitives
  are dropped, since no ray will meet them. They are still made as the scene is read, because a
  shader bound later may trace. Every standard light with a direction calls `transmission()`,
  so only a scene with no shadowing light gains. An area light keeps its own copy of its
  geometry.
- **A traced primitive carries its colour, opacity, surface shader and lights** from the moment
  it was made. A primitive with no surface shader, or whose shader fails to run, is drawn in its
  own colour at its own opacity. A triangle made from a polygon with a varying `"Cs"` carries a
  colour at each corner, and a hit's `Cs` blends them by the hit's barycentric weights. A
  gradient therefore draws the same under both hiders, and a shader's `trace()` sees it too.
- **A traced primitive carries the lights that were on when it was made**, as one set shared by
  every primitive made until the lights change (`trace::Lights`). A primitive given no lights,
  as in a scene built in code, is lit by the scene's own list, `Scene::lights()`.
- **A polygon with a displacement shader is added to the traced scene as the surface it is
  moved to**, under either hider. It is diced in camera space into a grid of
  `RenderContext::tracedAcross` micropolygons a side, 64, whatever the grid size. The
  displacement shader runs on that grid, and each micropolygon becomes two triangles with the
  moved normals. The ray hider sees and shades those triangles, and both hiders' shadows and
  reflections meet them. The triangles are flat between grid points, so a displacement finer
  than the grid is lost. The displacement runs once, so one that varies with time does not move.
  A sphere is not diced, and is traced undisplaced.

  Background: [ADR-0095](../adr/0095-offline-displaced-geometry-is-diced-into-the-traced-scene.md)
- **A sphere is intersected where it is defined**, cut to its slab of heights and its sweep,
  with RI's outward normal and its `u` and `v`. Its silhouette is exact at any size. A sphere
  whose radius is not positive is logged and not drawn.
  `Orientation` is not read, so a sphere cannot be turned inside out.
- **A moving primitive is stored where its motion's reference end put it**, which is the open
  end unless that end has no inverse
  ([CamerasAndSampling.md](CamerasAndSampling.md#motion-blur)). `Scene::nearest()` takes the
  poses at a time: a ray is carried back into the stored pose, and its hit is carried forward
  again.
- **`Scene::nearest()` tests every primitive.** There is no acceleration structure.

### The tracer

- **`trace::Tracer` traces rays through a scene and shades what they hit.** `see()` is what a
  batch of primary rays sees, each ray with the time its sample was taken. `transmitted()` and
  `traced()` are `transmission()` and `trace()` in world space, for a shading point that is not
  a hit, such as a grid vertex. `traced()` also takes a batch of rays.
- **Rays are traced and shaded in batches.** `see()` finds the next surface along every ray of
  its batch, and shades those hits together. Hits that share a surface shader, the shader's
  placement, an object space and a list of lights are shaded by one run of the shader, up to
  `Tracer::BATCH` (64) hits a run. Anything else a hit reads is a point of the batch.
- **What a ray sees does not depend on its batch.** Each hit is shaded from its own points, and
  each ray is composited in its own order. A batch gives the same answer, to the bit, as each of
  its rays alone.
- **Each batch is shaded by a `trace::HitShader`** made on the stack for it. The `HitShader`
  answers the shader's callbacks for the batch. A batch a shader traces into gets its own
  `HitShader`, so nothing is saved and restored around a traced ray.
- **A shader's traced rays are a batch.** `trace()` traces one ray for each hit of the run, and
  the rays are traced and shaded together, one depth down. A grid's `trace()` under the reyes
  hider is one batch too.
- **A hit is one point, or a stencil of three when its shader differentiates.** A surface
  shader that reads a derivative, or a light shining on it that does, shades each hit with the
  points a step along its primitive's u and along its v. The derivative built-ins difference
  them. Any other shader shades each hit as a single point, because a derivative it never reads
  would not change its result. The compiler records which programs differentiate
  ([ShadingLanguage.md](ShadingLanguage.md#built-in-functions)).
- **A stencil's step** is the change in the parameter that moves the surface one pixel's width,
  from `Scene::footprint()`, so a derivative is taken over what one pixel sees. Each hit of a
  batch has its own steps. A scene with no camera has a pixel one unit wide. `Primitive::at()`
  places a neighbour by its u and v, and `Scene::around()` carries it into the hit's pose. A step
  past a triangle's edge stays on its plane, and one past a sphere's seam continues its sweep. A
  primitive that cannot place a point has zero derivatives.
- **Only the hit traces.** A shadow or traced ray is cast from the hit, and its two neighbours
  take its answer. The neighbours are a pixel away, so the difference would not show, and a
  hit's rays are not tripled.
- **A triangle's u and v are its barycentric weights**, so its Du(P) and Dv(P) are its own
  edges, not those of the grid the reyes hider dices from the same polygon. `calculatenormal`
  points the same way under both, because a change of parameters scales the cross product of
  the two derivatives without turning it.
- **The `Tracer` keeps one machine per program per trace depth.** A surface tracing into another
  surface with the same shader is still part way through its run when the second run starts.
- **The poses are computed once for each ray of a batch**, at its sample's time, and a hit's
  shadow and traced rays use its ray's. A ray at the same time as the one before it shares that
  ray's poses, and a scene with nothing moving computes none. A caller that shades one point at
  a time sets the time with `Tracer::time()`.
- **A primary ray composites what it passes through**, front to back by each surface's `Oi`,
  with the background behind what remains. A shader that never writes `Oi` is as opaque as its
  primitive's `Os`. A ray continuing through a surface is not a traced ray and does not count
  against the trace depth.
- **A primitive with no surface shader is its own flat colour.**
- **The ray hider casts a row pass by pass.** Each pass casts a set of samples for every pixel
  of the row still unsettled, as one batch. A pixel's samples are seeded by the pixel and its
  pass, so the batch does not change them. The film is given the samples a pixel at a time,
  in the order a pixel at a time would give them.
- moya logs how long the hider took, so a render can be timed again after a change.

### Shadows and traced rays

- **`transmission()` is how a shader casts a shadow.** The three standard directional lights call
  it. It multiplies the light by every occluder's `Os`, read off the primitive without running
  its shader, so a shadow ray never shades anything. A renderer that cannot answer
  `transmission()` lets all the light through.
- **A ray leaving a surface is offset along the geometric normal, toward its target.** A ray
  started exactly on the surface hits that surface, and every lit pixel turns black in a
  pattern that looks like a normal fault.
- **A shadow ray is aimed from its offset start at the light, and stops the same offset short
  of it.** The end can lie on a surface too. An area light's samples lie on its own geometry,
  and a shadow ray that reached its end would hit that geometry and shadow the whole light.
- **`trace()` goes as deep as `Option "trace" "maxdepth"`**, two by default. Past that depth it
  returns the background, which stops two surfaces that trace into each other. moya takes a
  depth above 16 as 16, with a warning, because a shader that traces twice at every hit
  doubles the rays at every level. A depth that is negative or not finite is not used.
- **A grid traces through the same scene.** `reyes::GridShader` carries a shading point's ray from
  camera space into world space, starting it off the grid's plane. A ray from a grid of a warped
  quad may hit the exact polygon the grid was diced from, because the two agree only when the
  polygon is planar.

Background: [ADR-0077](../adr/0077-offline-one-shared-ray-tracer.md)
