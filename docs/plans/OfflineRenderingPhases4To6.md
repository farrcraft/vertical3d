# Offline Rendering, Phases 4 To 6 — Samples, Rays That Recurse, And One Ray Tracer For Both

Drafted 2026-10-04 against `f61f898`. Seventeen steps across `api/render/offline`, `moya`,
`talyn` and the documents, three of them held, taking up the last three phases of
[the offline rendering roadmap](../roadmap/OfflineRendering.md): phase 4, sampling and quality;
phase 5, talyn's own list; and phase 6, whether the two renderers unify.
[Phase 3](completed/OfflineRenderingPhase3.md) closed on 2026-09-10 and left all three unblocked.

Both renderers are in this tree, and nothing outside it consumes them. Their suites run in CI on
every pull request, so every step here is checked by a machine rather than by a person looking
at a window. Each renderer's suite runs in about a second.

The roadmap orders the phases 4, 5, 6, and the plan keeps that order with three changes:

* **A step of corrections goes first.** The surveys found the roadmap and two comments wrong
  about the tree, in ways a later step would trip on.
  [Step 1](#step-1--what-the-roadmap-and-the-comments-got-wrong).
* **moya's hider is fixed before it is sampled.** It fills a micropolygon's whole raster bounding
  box rather than testing a pixel centre against the micropolygon. At one sample per pixel that is
  a fringe at the edges; at sixteen it would be sixteen fringes.
  [Step 6](#step-6--moyas-hider-tests-a-sample-against-the-micropolygon).
* **Phase 6 is decided early and built last.** Its record needs phase 5's re-entrant trace to
  know what it is moving, and moving talyn's ray tracer is only worth doing once it does
  something moya wants: shadows and reflections.
  [Step 14](#step-14--the-record-one-ray-tracer-both-renderers-reach).

## What the surveys found

The tree was read at `f61f898`.

**Shared, in `api/render/offline`:**

* **There is no shared sample store.** Both renderers write `offline::FrameBuffer` planes
  directly, one sample per pixel centre, and then run `sl::Imager`. The coverage plane was built
  to hold a fraction once there is more than one sample
  ([OfflineRenderers.md](../OfflineRenderers.md)).
* **The RIB reader knows none of the sampling requests.** `PixelSamples`, `PixelFilter`,
  `PixelVariance`, `Shutter`, `DepthOfField`, `Exposure`, `Quantize` and `MotionBegin` fall to
  `unrecognised_` and are skipped (`rib/Reader.cxx:582-591`), and `rib::Handler` has no method for
  any of them.
* **The stubs are `texture` (two forms), `shadow` and three `noise` forms**, declared at
  `sl/Builtins.cxx:132-139` and answering their defaults with one report each
  (`sl/runtime/Library.cxx:487-490`). `calculatenormal` is declared and has no body. `environment`
  is not declared, so a shader calling it does not compile. Displacement and volume shaders are
  refused at `sl/Compiler.cxx:389-391`.
* **`trace` and `transmission` fall back** to black and to fully lit when a renderer does not
  answer them (`sl/runtime/Library.cxx:577-596`). The comment at `sl/runtime/Renderer.h:84` says
  moya answers `trace` with its background. It does not; it answers black, and
  `SlLightingTest.cxx:372-380` pins black.
* **`type::Random` is splitmix64** with its floats derived by hand, so a seed gives the same
  sequence on any standard library. `v3dlib_render_offline` already links `v3dlib_type`.
* **Every reference is compared at a tolerance of one 8-bit step**, `image::compare(..., 1)`,
  which is for float rounding across compilers and not for noise. Sampling has to be
  deterministic, or no reference survives it.

**In moya:**

* **The hider is `hide()`** (`moya/libmoya/Bucket.cxx:25-80`). It projects a micropolygon's four
  corners, takes their raster bound, and writes every pixel centre inside it: no point in polygon
  test. Depth is the mean of the four corners, colour is one corner's `Ci`, and opacity is
  ignored. It writes the whole image rather than its bucket, and `Bucket::left_` and `top_` are
  never set.
* **The five `Ri*Filter` functions return 0.0** (`RenderMan.cxx:194-212`). `RiPixelSamples`,
  `RiPixelFilter`, `RiPixelVariance`, `RiShutter`, `RiExposure`, `RiQuantize`, `RiHider` and
  `RiMotionBegin` are empty. **`RiDepthOfField` is not declared at all**, only named in a comment
  at `RenderMan.cxx:372`.
* **`fStop_`, `focalLength_`, `focalDistance_`, `shutterOpen_` and `shutterClose_` are moya's**
  (`moya/libmoya/RenderContext.h:315-319`), unread and unwritten. The roadmap places them on a
  shared `RenderContext` that does not exist.
* **A primitive dices into a fixed 16 by 16 grid** whatever its size, bilinearly from its first
  four vertices (`Polygon.cxx:356-433`). The shading rate only sets when a primitive splits.
* **moya keeps no scene.** Primitives go into buckets in camera space and are diced once, so it
  has nothing to trace a ray against. `GridShader` answers neither `trace` nor `transmission`, so
  moya has no shadows.
* **Its suite is 70 cases in 12 files**, and two references, each reached from code and from RIB.
  `ReferenceTest.cxx:182-189` asserts an exact red and depth at one pixel.

**In talyn:**

* **One ray per pixel centre**, from the near plane through `(column + 0.5, row + 0.5)`
  (`talyn/libtalyn/RenderContext.cxx:99-100`), shaded by `HitShader` as a batch of one.
* **`trace()` stops at a depth of one** (`HitShader.cxx:253-258`), and **it is not re-entrant**.
  `HitShader` keeps one `Machine` per program, so a surface that traces into another surface with
  the same shader runs that machine while it is already running and overwrites its registers.
  `trace()` restores `hit_` and not `placement_`, which `shade()` overwrites. No shipped shader
  calls `trace()`, and no talyn case tests it.
* **`transmission()` is opaque only** (`HitShader.cxx:243`). `Os` reaches the shader and `Oi` is
  set, but talyn never reads `Oi`: there is no continuation ray and no compositing.
* **Triangles only, by brute force** (`Scene.cxx:254-286`). The reader dispatches `Sphere` and
  talyn drops it silently. The largest test scene is four triangles.
* **Texture coordinates are barycentric**, standing in for `s` and `t`, with no `du` or `dv`.

**Carried from phase 3** ([its plan](completed/OfflineRenderingPhase3.md), "What this does not
do"): area lights, deferred to phase 4 as a sampling problem; no displacement or volume shaders;
no `Sides` or `Orientation`; `calculatenormal` stubbed; `"object"` space unanswered in moya; a
distant light's shadow ray of fixed length; and the C array helper still in moya rather than in
`rib/`. [TODO.md](../TODO.md#rirotates-sign) holds `RiRotate`'s sign.

---

## The steps

| | What | Where | ADR | State |
|---|---|---|---|---|
| [1](#step-1--what-the-roadmap-and-the-comments-got-wrong) | The roadmap, two comments and the declared C API, corrected | `docs`, `api/render/offline`, `moya` | — | done |
| [2](#step-2--the-sampling-requests-reach-both-renderers) | The sampling, camera and shutter requests reach both renderers | `api/render/offline`, `moya`, `talyn` | — | done |
| [3](#step-3--the-record-a-pixel-is-a-filtered-set-of-seeded-samples) | The record: a pixel is a filtered set of seeded samples | `docs/adr` | **0076** | done, accepted |
| [4](#step-4--five-filters-and-a-film) | Five filters and a film both renderers write samples into | `api/render/offline` | 0076 | done |
| [5](#step-5--talyn-samples-through-the-film) | talyn samples through the film | `talyn` | 0076 | done |
| [6](#step-6--moyas-hider-tests-a-sample-against-the-micropolygon) | moya's hider tests a sample against the micropolygon | `moya` | — | done |
| [7](#step-7--moya-samples-through-the-film) | moya samples through the film | `moya` | 0076 | done |
| [8](#step-8--depth-of-field) | Depth of field in both | `moya`, `talyn` | — | done |
| [9](#step-9--motion-blur-of-a-transform) | Motion blur of a transform, in both | `api/render/offline`, `moya`, `talyn` | — | done |
| [10](#step-10--adaptive-sampling-in-talyn) | Adaptive sampling in talyn | `talyn` | — | done |
| [11](#step-11--a-trace-that-recurses) | A trace that recurses, to a depth a scene sets | `talyn` | — | not started |
| [12](#step-12--reflection-refraction-and-transparency) | Reflection, refraction, transparency and spheres | `api/render/offline`, `talyn` | — | not started |
| [13](#step-13--texture-and-noise) | `texture()` from an image, and `noise()` | `api/render/offline`, `moya`, `talyn` | — | not started |
| [14](#step-14--the-record-one-ray-tracer-both-renderers-reach) | The record: one ray tracer both renderers reach | `docs/adr` | **0077** | not started |
| [15](#step-15--the-ray-tracer-moves-into-the-shared-library) | talyn's scene and hit shading move into the shared library | `api/render/offline`, `talyn` | 0077 | not started |
| [16](#step-16--moya-traces) | moya traces: shadows and `trace()` | `moya` | 0077 | not started |
| [17](#step-17--held-area-lights-displacement-and-acceleration) | Area lights, displacement and bump, and an acceleration structure | — | — | held |

Steps 1 and 2 depend on nothing. Step 4 needs step 3. Step 5 needs steps 2 and 4. Step 7 needs
steps 4 and 6. Steps 8 and 9 need steps 5 and 7. Step 10 needs step 5. Step 12 needs step 11.
Step 13 depends on nothing. Step 15 needs steps 11 and 14, and step 16 needs step 15.

---

### Step 1 — What the roadmap and the comments got wrong

**Documents and comments only, and the one missing declaration.** Nothing changes a picture.

* The roadmap's phase 4 says the depth of field and shutter fields are on `RenderContext` as if it
  were shared. They are moya's, and talyn has none.
* `sl/runtime/Renderer.h:84` says moya answers `trace` with its background. It answers black,
  through the fallback, which `SlLightingTest` pins. The comment says so.
* `RiDepthOfField(RtFloat fstop, RtFloat focallength, RtFloat focaldistance)` is declared in
  `RenderMan.h` and given an empty body beside `RiShutter`, so the interface moya claims to
  declare in full does. Step 2 fills it.
* OfflineRenderers.md said no plan was open against the renderers, and now points here.

**Tests:** none new. All 25 suites pass and the references are untouched.

**Landed.** The survey's claim that OfflineRenderers.md gave a count for moya's suite was
wrong: no document records one, so there was nothing to bring up to date.

### Step 2 — The sampling requests reach both renderers

**A plain description of how a frame is sampled**, `offline::Sampling`, in the shared library:

```cpp
struct Sampling final {
    glm::uvec2 samples { 2, 2 };            // PixelSamples, the RI default
    Filter filter { Filter::Gaussian };      // PixelFilter
    glm::vec2 width { 2.0f, 2.0f };
    float variance { 0.0f };                 // PixelVariance; 0 asks for no adaptive sampling
    glm::vec2 shutter { 0.0f, 0.0f };        // Shutter, open and close
    float fstop { 0.0f };                    // DepthOfField; 0 is a pinhole
    float focalLength { 0.0f };
    float focalDistance { 0.0f };
};
```

`rib::Handler` gains `pixelSamples`, `pixelFilter`, `pixelVariance`, `shutter` and `depthOfField`,
and the reader dispatches the five requests. Both renderers' handlers store them into a
`Sampling` they hold per frame. moya's C entry points do the same, and its five unread fields
are replaced by the struct.

**The RI defaults are the defaults**: two by two samples and a gaussian two pixels wide. This
tree's two renderers read RIB, and a scene that says nothing should render the way RenderMan says
it renders. What that does to the existing references is [step 5](#step-5--talyn-samples-through-the-film)'s
to handle.

**Tests**, headless:

* in the offline suite: each request reaches a recording handler with its arguments, and a filter
  named by an unknown name is reported and leaves the default;
* in each renderer's suite: a scene's `PixelSamples 4 4` and `DepthOfField 8 0.1 3` reach the
  renderer's `Sampling`, and a scene that names nothing holds the RI defaults.

**Landed.** The reader turns a `PixelSamples` rate into a count of at least one and resolves a
`PixelFilter` name to an `offline::Filter` before the handler sees either, so both renderers'
handlers only store. `DepthOfField` and `Shutter` are a request group of their own rather than
part of the camera's, which clang-tidy's complexity check would not take. RIB's argumentless
`DepthOfField` reaches the handler as an infinite fstop, and `Sampling::pinhole()` answers for
both RI's spelling and this struct's zero.

### Step 3 — The record: a pixel is a filtered set of seeded samples

**ADR-0076.** It goes in as `proposed` and is accepted when step 5 renders through it.

* **A sample is a point in a pixel, a time, and a point on the lens.** Both renderers produce
  samples, and a shared film turns them into pixels. The film is the one place a filter is
  applied, so the two renderers cannot filter differently.
* **Samples are stratified and jittered from a seed per pixel.** A pixel's samples are drawn from
  a `type::Random` seeded from its column and row, so a frame is the same on every run and every
  standard library, and a reference at a tolerance of one step survives sampling. Rendering a
  pixel never depends on the order pixels or buckets are rendered in.
* **A pixel's colour is the filtered sum over the samples within the filter's width**, normalised
  by the sum of the weights. Coverage is the same sum over a sample's hit or miss, which is the
  fraction the coverage plane was built for. Opacity is held per sample and composited front to
  back once a renderer writes more than the nearest hit, which step 12 does for talyn.
* **The imager runs after the film resolves**, on pixels, as it does today.

**Alternatives the record weighs:**

* **Each renderer filters its own samples.** moya filters per bucket and talyn per pixel. Two
  filters to keep in agreement, and a reference that pins each separately.
* **Random sampling from a global stream.** Simpler, and a bucket order or a thread changes the
  picture.
* **Box filtering only.** Exact for a test and wrong for a picture: the RI default is a gaussian,
  and box filtering at two by two aliases visibly on an edge.

### Step 4 — Five filters and a film

**In `api/render/offline`:**

* **`offline::filter(Filter, x, y, width)`**, the five RI filters with the RI conventions: box,
  triangle, gaussian, catmull-rom and sinc, each zero outside its width. moya's `RiBoxFilter` and
  the others become calls to it.
* **`offline::Film`**, sized to the image, which takes a sample — a raster position, a colour, an
  opacity, a depth and whether it hit — and resolves into the frame buffer's planes once a frame
  is done. It holds the weighted sums per pixel rather than the samples, so its memory is the
  frame buffer's again, not the samples'. A sample near an edge of the image is filtered into the
  pixels it reaches.
* **`offline::Sampler`**, which gives a pixel's samples from `Sampling` and a seed: stratified
  positions, a time across the shutter, and a point on a unit disc for the lens.

**Tests**, headless:

* each filter's weight at its centre, at half its width and past it, against the RI formulas;
* a film of uniform samples resolves to that colour exactly, and full coverage to one;
* half the samples of a pixel hitting resolves to a coverage of one half under a box filter;
* a sampler gives the same samples for one pixel whatever order pixels are asked for in, and the
  same on a second run;
* one sample a pixel under a box filter one pixel wide resolves to that sample exactly, which is
  the case step 5 relies on.

**Landed**, with two rules the step did not foresee. **An axis with one stratum is sampled at
the pixel centre**, not jittered, because otherwise one sample a pixel moves off the centre the
references were drawn at and none of them survives. **A miss carries a colour into the film**,
black by default, because talyn's `Scene::background()` colours a miss and two cases pin it;
coverage still counts only hits. A uniform film resolves to its colour to float rounding rather
than exactly, since a gaussian's weights do not sum exactly.

### Step 5 — talyn samples through the film

**talyn casts a ray per sample** rather than per pixel centre, and writes it to the film.

**The existing references are kept, not regenerated.** Each scene that draws one, in code and in
its `.rib`, gains `PixelSamples 1 1` and `PixelFilter "box" 1 1`. At one sample per pixel at its
centre under a one pixel box, the film gives back exactly the sample, so those references must
match byte for byte, and a case that does not is a fault in the film. That pins the old path.

**A new reference pins sampling**: `reference-sampled.png`, the shaded scene at the RI defaults,
reached from code and from RIB like the others. Its edges are antialiased, and it is the same on
every run because the samples are seeded.

**Tests:**

* both existing references match at one sample, unchanged;
* the new reference matches from both routes;
* two renders of the sampled scene are equal byte for byte;
* the coverage of a pixel half covered by a triangle's edge is between zero and one.

**Landed.** Both references and `RenderContextTest`'s exact-pixel cases match at one sample
under a one pixel box. They match byte for byte by construction rather than by tolerance: the
sample is the pixel centre's float, and its weight is one. `reference-sampled.png` is the shaded
scene at the defaults, and the code and RIB routes render it identically.

### Step 6 — moya's hider tests a sample against the micropolygon

**At one sample per pixel**, before any sampling: a pixel centre is written only if it falls inside
the micropolygon, as two triangles, and its depth is interpolated across the micropolygon rather
than averaged. Colour stays the corner's for now.

This changes moya's shaded reference at its edges, on purpose. It is regenerated in this step and
in a commit of its own, and the picture is looked at before it is committed: the edges move
inwards by up to one micropolygon and nothing else moves. The white quad's reference does not
change, because its micropolygons tile its bound exactly.

**Tests:**

* a micropolygon rotated 45 degrees covers the centres inside it and not the corners of its bound;
* a sample's depth is interpolated, so two crossing micropolygons are ordered at the sample and
  not by their means;
* the white quad's reference is unchanged; the shaded one is regenerated, and its regeneration is
  the step's.

**Landed, and neither reference changed.** The prediction above was wrong: every moya scene is
an axis-aligned quad under an orthographic camera, so each micropolygon already fills its raster
bound, and its depth is flat, so the mean and the interpolation agree. The fringe is real for a
rotated or perspective micropolygon, and `MicroPolygonGridTest` pins the test that removes it.
With no picture to regenerate, steps 6 and 7 went in as one commit.

### Step 7 — moya samples through the film

**The hider writes samples into the film**, a bucket at a time. A bucket is given its position, so
it hides only the samples inside it, and a primitive is filed into every bucket its bound touches
rather than the one under its upper left corner. The film filters across bucket edges, because it
holds sums for the whole image.

The two references are kept at one sample under a box, as talyn's are in step 5, and must match
step 6's pictures byte for byte. A new `reference-sampled.png` pins moya at the RI defaults.

**Tests:**

* both references unchanged at one sample;
* the new reference matches from both routes, and twice in a row;
* a primitive straddling a bucket edge renders the same as one inside a bucket;
* `ReferenceTest.cxx:182-189`'s exact pixel still holds at one sample.

**Landed, without bucket-local hiding.** moya's sweep comes round again when a split lands
behind it, and a primitive dices once, so no bucket is finished until the sweep is and a
primitive filed into several buckets would be diced by the first and yield nothing to the rest.
The hider writes into `moya::Samples`, a store of every sample of the frame, and the store goes
through the film once after the last bucket. Its memory is the samples' rather than a frame
buffer's, which ADR-0076's film avoids for talyn and a reyes hider cannot; bucket-local stores
are what threads would want, and they wait for those. A primitive is still filed under its upper
left corner, which costs nothing now that hiding is not bucket-local. The straddling case
renders the shaded scene at a bucket size of 8 and of 64 and requires them equal byte for byte.

### Step 8 — Depth of field

**A sample's lens point moves the eye.** `fstop`, `focalLength` and `focalDistance` give a lens
radius, `focalLength / (2 * fstop)`, and every sample's eye is offset across it while its ray
still passes through the point in focus at `focalDistance`.

* **In talyn** that is the ray: its origin moves on the lens and its direction is aimed at the
  point on the plane of focus.
* **In moya** it is the hider: a micropolygon is projected for the sample's lens point, which
  shifts it by its circle of confusion. Its raster bound is grown by the largest circle of
  confusion over its depth, so the bucket filing still finds every sample it can reach.

**Tests:**

* an fstop of zero, the default, renders every existing reference unchanged;
* a quad on the plane of focus is as sharp as at a pinhole, to the tolerance;
* a quad far from it spreads its edge over more pixels, by the circle of confusion the lens gives,
  in both renderers;
* a new reference per renderer, of two quads at two depths, from both routes.

**Landed.** Both renderers ignore the lens under an orthographic projection, which every
earlier scene uses. talyn takes the lens's axes from the inverse of the camera's view, because
a profile's normals do not follow its rotation. `reference-focus.rib` is one scene in both
suites, and the two renderers draw it alike: a red quad on the plane of focus and a blue one
blurred over about five pixels. A blurred micropolygon is tested against every sample its grown
bound reaches, and in a Debug build that took moya's suite from about one second to fourteen
and talyn's, with step 5's sampled cases, to about eleven.

### Step 9 — Motion blur of a transform

**Only a transform moves.** `MotionBegin [t0 t1]` around a transform request gives the
transform at each end, and a primitive under it is placed at a sample's time by interpolating
between them: translation linearly, rotation by a quaternion. Deforming geometry, a vertex list
inside a motion block, is not built, and the reader reports it.

* `rib::Handler` gains `motionBegin(times)` and `motionEnd()`, and the transform stack holds a pair
  while a block is open. That is shared, so both renderers read it the same way.
* **talyn** transforms a sample's ray into the primitive's space at its time rather than moving
  the triangles.
* **moya** dices once, in the primitive's space, and places the grid's vertices at the sample's
  time when it hides them, with the bound grown over the shutter.

**Tests:**

* a shutter of zero, the default, renders every existing reference unchanged;
* a quad translated across the shutter spreads over the distance it moved, with coverage falling
  off linearly along it under a box filter;
* a new reference per renderer, from both routes;
* a vertex list inside a motion block is reported and drawn at the block's first time.

**Landed.** The shared piece is `offline::MovingTransform`, which each renderer holds as its
current transformation, rather than a transform stack in the library: both renderers keep their
own stacks, and what had to agree was how a block fills the two ends and how they blend. A
deforming primitive is the reader's to report, through `Reader::unsupported()`, since it is the
reader that knows a primitive repeated inside a block. moya's first version placed a moving
micropolygon afresh for every sample its swept bound reached and took 75 seconds over the
suite; the motion to a sample's time is now worked out once per sample per grid, and a sample is
rejected by the bound of its slice of the shutter first, which brings the suite back to about
twenty-three. `reference-motion.rib` is one scene in both suites, a quad sliding and a quad
turning, and the renderers draw it alike.

### Step 10 — Adaptive sampling in talyn

**`PixelVariance` above zero asks for more samples where they matter.** talyn takes the pixel's
first stratified set, and where the colour variance across it is above the threshold it takes a
second set, until the variance falls or a cap of four times the first set is reached. Each further
set is seeded from the pixel too, so the result is still deterministic.

**moya takes a fixed count.** A reyes hider samples a whole bucket at once, and adapting per pixel
would split a grid's hiding into two passes. The record says so.

**Tests:**

* a variance of zero, the default, renders the sampled reference unchanged;
* a flat colour takes no extra samples, and a pixel on an edge does;
* the adapted render is the same twice.

**Landed**, and with it phase 4. The variance is the largest channel's variance of the pixel's
mean, which is what RI's `PixelVariance` bounds; fewer than two samples never settle. Casting a
sample became a `Caster` of its own in talyn's render context, so the loop over sets stays
readable and inside the complexity gate.

### Step 11 — A trace that recurses

**`HitShader` keeps a machine per depth**, made as a trace first reaches that depth, and saves and
restores everything `shade()` writes, `placement_` with `hit_`. A surface tracing into another
with the same shader then runs a different machine.

**The depth is the scene's.** `Option "trace" "maxdepth"`, the standard name, sets it, and its
default is two. Past it, `trace()` answers the background, as it does at a depth of one today.

**Tests:**

* a surface tracing a ray into a surface with the same shader gets that surface's colour, and its
  own registers are intact after the trace;
* the depth stops at the option's value and answers the background past it;
* the existing references, whose shaders do not trace, are unchanged.

### Step 12 — Reflection, refraction and transparency

**Three things a ray tracer is for, and a shape it can reflect.**

* **Shaders that trace.** The standard library gains `shinymetal` and `glass`, written in the
  language: `shinymetal` adds `trace(reflect(I, N))` scaled by its `Kr`, and `glass` splits a ray
  between reflection and refraction by a `fresnel` built-in, which the library gains with
  `refract`'s conventions.
* **Transparency.** A hit whose `Oi` is not one continues the ray behind it and composites front to
  back, to the depth limit. `transmission()` multiplies by each occluder's opacity rather than
  stopping at the first.
* **Spheres.** talyn intersects `Sphere` analytically rather than dropping it. A reflection is the
  clearest picture with a sphere in it. moya does not dice spheres, and a scene with one renders
  without it in moya; that is reported.

**Tests:**

* a mirror facing a red quad shows the red quad, exactly, at a depth of two;
* a half-opaque white quad over a black one composites to grey exactly, through `Oi`;
* a shadow through a half-opaque occluder is half as dark;
* a ray through a glass slab at normal incidence comes out where it went in, displaced by nothing;
* a sphere's silhouette covers the pixels its radius says;
* a new reference, a glass sphere and a metal one over a checked floor, from both routes.

### Step 13 — `texture()` and `noise()`

**`texture(name, s, t)` reads an image through `image::Factory`**, once per name per frame, and
samples it bilinearly with the RI default of periodic wrapping. `MakeTexture` is accepted and
copies nothing: the image a scene names is the texture, since there is no `txmake` in the tree and
no reason to write one. A name that does not load is reported once and answers black.

**`noise()` is Perlin's improved noise**, in the library and seeded by nothing, so a shader's
pattern is the same on every machine. Its three forms answer a float, a point and a colour.

**Real `s` and `t`.** talyn takes `st` from a polygon's vertices when the scene gives them, and
falls back to the barycentrics it uses today. moya's grid already has them.

**Tests:**

* a texture of four known texels samples to each texel at its centre and to their blend between;
* wrapping past one returns to the start;
* `noise` is in its range, continuous, and the same at a point on every call;
* a textured quad renders the same in both renderers to the tolerance, which is the first
  picture the two are asked to agree on;
* a missing texture answers black and reports once.

### Step 14 — The record: one ray tracer both renderers reach

**ADR-0077 answers phase 6.** It goes in as `proposed` and is accepted when step 16 renders a moya
shadow through it.

**The decision drafted: two renderers that share one ray tracer.** talyn's scene, its intersection
and its hit shading move into `api/render/offline` as a library both renderers link. talyn becomes
a driver that casts primary rays into it. moya keeps its reyes hider for what the camera sees and
reaches the shared tracer from a shader's `trace()` and `transmission()`. A renderer is then its
hider, and everything after a hit is shared.

**Alternatives the record weighs:**

* **talyn as moya's ray tracing component**, moya linking `libtalyn`. One renderer linking another
  makes talyn's driver a part of moya's build, and its options moya's.
* **One renderer with two hiders.** A RenderMan renderer with a ray traced hider is a real design,
  and it would retire talyn as an executable. It is a larger change than either renderer needs,
  and talyn is the simpler of the two to read.
* **Leave them apart, and moya traces nothing.** moya then never has shadows, which a reyes
  renderer gets from shadow maps, and a shadow map needs a depth pass of its own that is a larger
  piece of work than sharing the tracer.

What the record has to settle besides: **the space the shared scene is in**, world space as talyn's
is, which moya's camera space primitives are transformed into as they are added.

### Step 15 — The ray tracer moves into the shared library

**A move, and no change to a picture.** `talyn::Scene`, its triangles and spheres, and `HitShader`
move to `api/render/offline/trace` under `offline::trace`, and talyn links them.

**Tests:** every talyn case passes unchanged, and every talyn reference matches byte for byte.
The cases that were talyn's and test the moved code move with it into the offline suite.

### Step 16 — moya traces

**moya adds every primitive to a shared scene as it is added to a bucket**, in world space, and
`GridShader` answers `trace` and `transmission` from it. moya then has ray traced shadows, and its
shaders can reflect.

moya's shaded reference changes, on purpose: the shadow talyn's shaded scene has, moya's now has.
It is regenerated in this step and looked at before it is committed.

**Tests:**

* the white quad's reference is unchanged, because nothing in it casts a shadow;
* moya's shaded reference shows the occluder's shadow where talyn's does, and is regenerated;
* a `shinymetal` grid in moya reflects a red quad;
* the fallback cases in `SlLightingTest` still pin black and fully lit for a renderer that answers
  neither.

### Step 17 — Held: area lights, displacement, and acceleration

**Area lights.** Phase 3 deferred them to phase 4 as a sampling problem, and step 4 solves the
sampling: a light's points are a sampler's lens disc in another place. What it does not solve is
the shading model, since a RenderMan area light runs its light shader at points on a primitive,
and `AreaLightSource` binds a shader to geometry the renderers do not keep as lights. **The
trigger is a scene that wants soft shadows**, which after step 16 both renderers could cast.

**Displacement and bump.** Displacement reaches back into dicing in moya, moves a grid after it is
shaded, and needs a bound grown by `displacementbound`. In talyn it needs tessellation a ray
tracer does not otherwise do. Bump needs `calculatenormal`, which needs derivatives across a
batch, and talyn's batch is one hit. **The trigger is a scene that needs surface detail a texture
cannot give.**

**An acceleration structure.** The roadmap says it waits for a scene slow enough to need one. The
largest test scene is four triangles, and step 12's adds a few more. **The trigger is a scene that
takes a second to render**, which the suites' times would show.

Each moves to [TODO.md](../TODO.md) with its trigger when the plan closes.

---

## Sequence

**Steps 1 and 2 first.** One is corrections, and the other is plumbing every later step reads from.

**Then the record and the film, then talyn.** Step 5 is the plan's first midpoint: if one sample
under a box does not reproduce talyn's references byte for byte, the film is wrong, and the plan
stops there.

**Then moya, in two steps.** Step 6 changes a picture on purpose and step 7 must not, so they are
separate commits. Step 7 is the second midpoint, for the same reason as step 5.

**Steps 8 to 10 complete phase 4.** Each defaults to the picture before it.

**Steps 11 to 13 are phase 5.** Step 13 can go at any time.

**Steps 14 to 16 are phase 6, last**, as the roadmap asked. Step 15 is a move that must change
nothing, and step 16 changes moya's shaded picture on purpose.

## Verification

Per [sdlc.md](../sdlc.md), every step that changes code: `ninja -C out/build/x64-Debug`, `ctest`,
cpplint, and the `/W4 /WX`, `/analyze` and clang-tidy gates with the apps built. The tree is clean
at all of them, so every finding is the step's. CI runs both renderers' suites on the pull request.

**What can be pinned is pinned, by references the specification and a seed determine:**

* every existing reference, kept at one sample under a box from step 5 on, byte for byte;
* a sampled reference per renderer, the same on every run because its samples are seeded;
* filters, the film, the sampler, a recursing trace, compositing and texture sampling, headless,
  by hand-worked values.

**A reference changes only in a step that says it does**: moya's shaded reference in steps 6 and 16.
Each is looked at before it is committed, and the commit says what moved and why. A reference
that changes in any other step is a fault.

## What this does not do

* **No RenderMan compliance**, per the roadmap.
* **No integration with the realtime stack**, per the roadmap and the modernization plan.
* **No shadow maps and no `environment()`.** Ray traced shadows replace the first in both renderers
  after step 16. The second needs a cube or a latitude map read, and no scene asks.
* **No deforming motion blur, and no motion blur of a shader's parameters.**
* **No `Sides`, `Orientation` or volume shaders**, which phase 3 also left.
* **No threads.** Each renderer renders on one, and a seed per pixel is what would let it render on
  more without changing a picture.
* **`RiRotate`'s sign** stays in [TODO.md](../TODO.md#rirotates-sign).

## When a step lands

Update the state in the table above.

* **Drafting** points [the roadmap](../roadmap/OfflineRendering.md)'s phases 4, 5 and 6 here, and
  says in the plans index that this plan is open.
* **Step 1** corrects the roadmap and [OfflineRenderers.md](../OfflineRenderers.md).
* **Step 2** adds the requests to OfflineRenderers.md's account of the reader.
* **Step 3** adds the index row for 0076 as `proposed`; **step 5** accepts it.
* **Steps 4, 5 and 7** describe the film and the sampler in OfflineRenderers.md, and replace its
  "one sample per pixel centre".
* **Steps 6 and 16** say in this plan what each regenerated reference shows now.
* **Steps 8 to 13** each add their requests, built-ins and shaders to OfflineRenderers.md.
* **Step 14** adds the row for 0077 as `proposed`; **step 16** accepts it, and the roadmap's phase 6
  says it is answered.
* **When the plan closes**, the roadmap moves to `roadmap/completed/` and points here, the held
  items move to TODO.md with their triggers, and this file moves to [completed/](completed/).
