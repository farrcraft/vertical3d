# Ray tracing

The shared ray tracer in `api/render/offline/trace`, which the ray-trace hider uses to find what
the camera sees and which shaders use for shadows, reflection and refraction.

## Ray tracing

The ray tracer is in `api/render/offline/trace`, namespace `offline::trace`. Both hiders use it.
The ray hider casts every primary ray into it. Under either hider, a shader's `trace()` and
`transmission()` calls are answered from it.

### The traced scene

- **`trace::Scene` holds primitives in world space, as one list.** A primitive is anything that
  derives from `trace::Primitive` and implements `intersect` and `describe`. `trace::Triangle`
  and `trace::Sphere` are the two kinds. A new kind is a new class, and the scene does not
  change.
- **Every primitive a scene gives moya is added to the traced scene as it arrives**, in world
  space, before any split or dice. A polygon becomes a fan of triangles, since RI defines a
  polygon as planar and convex. The reyes hider reads the scene for `trace()` and
  `transmission()`. The ray hider reads it for everything.
- **A traced primitive carries its colour, opacity, surface shader and lights** from the moment
  it was made. A primitive with no surface shader, or whose shader fails to run, is drawn in its
  own colour at its own opacity. A triangle made from a polygon with a varying `"Cs"` carries a
  colour at each corner, and a hit's `Cs` blends them by the hit's barycentric weights. A
  gradient therefore draws the same under both hiders, and a shader's `trace()` sees it too.
- **A traced primitive carries the lights that were on when it was made**, as one set shared by
  every primitive made until the lights change (`trace::Lights`). A primitive given no lights,
  as in a scene built in code, is lit by the scene's own list, `Scene::lights()`.
- **A sphere is intersected where it is defined**, cut to its slab of heights and its sweep,
  with RI's outward normal and its `u` and `v`. Its silhouette is exact at any size. A sphere
  whose radius is not positive is logged and not drawn.
  `Orientation` is not read, so a sphere cannot be turned inside out.
- **A moving primitive is stored where its motion's reference end put it**, which is the open
  end unless that end has no inverse ([CamerasAndSampling.md](CamerasAndSampling.md#motion-blur)). `Scene::nearest()` takes
  the poses at a time: a ray is carried back into the stored pose, and its hit is carried
  forward again.
- **`Scene::nearest()` tests every primitive.** There is no acceleration structure.

### The tracer

- **`trace::Tracer` traces rays through a scene and shades what they hit.** `see()` is what a
  primary ray sees. `transmitted()` and `traced()` are `transmission()` and `trace()` in world
  space, for a shading point that is not a hit, such as a grid vertex.
- **Each hit is shaded by a `trace::HitShader`** made on the stack for it. The `HitShader`
  answers the shader's callbacks for that hit. A hit a shader traces into gets its own
  `HitShader`, so nothing is saved and restored around a traced ray.
- **The `Tracer` keeps one machine per program per trace depth.** A surface tracing into another
  surface with the same shader is still part way through its run when the second run starts.
- **The `Tracer` computes the poses once for a sample's time** (`Tracer::time()`), so a
  sample's shadow and traced rays see the same moment without inverting a motion per ray.
- **A primary ray composites what it passes through**, front to back by each surface's `Oi`,
  with the background behind what remains. A shader that never writes `Oi` is as opaque as its
  primitive's `Os`. A ray continuing through a surface is not a traced ray and does not count
  against the trace depth.
- **A primitive with no surface shader is its own flat colour.**
- **One shader run per hit is slow**, and accepted. The references are 64 by 48 pixels.

### Shadows and traced rays

- **`transmission()` is how a shader casts a shadow.** The three standard directional lights call
  it. It multiplies the light by every occluder's `Os`, read off the primitive without running
  its shader, so a shadow ray never shades anything. A renderer that cannot answer
  `transmission()` lets all the light through.
- **A ray leaving a surface is offset along the geometric normal, toward its target.** A ray
  started exactly on the surface hits that surface, and every lit pixel turns black in a
  pattern that looks like a normal fault.
- **`trace()` goes as deep as `Option "trace" "maxdepth"`**, two by default. Past that depth it
  returns the background, which stops two surfaces that trace into each other. moya takes a
  depth above 16 as 16, with a warning, because a shader that traces twice at every hit
  doubles the rays at every level. A depth that is negative or not finite is not used.
- **A grid traces through the same scene.** `GridShader` carries a shading point's ray from
  camera space into world space, starting it off the grid's plane. A ray from a grid of a warped
  quad may hit the exact polygon the grid was diced from, because the two agree only when the
  polygon is planar.

Background: [ADR-0077](../adr/0077-offline-one-shared-ray-tracer.md)
