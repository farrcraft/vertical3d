# Offline Shading: The Remaining RenderMan Features

What moya and `api/render/offline` need to render the scenes a RenderMan shader writer expects to
work: soft shadows from area lights, translucent surfaces under either hider, displaced and bumped
surfaces, and wide distant lights. Each is refused or approximated today.

The gaps are each described in [TODO.md](../TODO.md#offline-rendering). This roadmap orders them.
Area lights multiply the number of shadow rays, so the ray tracer has to be fast before they are
built. Every later step builds on transformations, so their correctness comes first.
[The offline renderer](../offline/README.md) describes the pipeline and the two hiders.

The [OfflineShading plan](../plans/OfflineShading.md) takes up every section below.

## What exists

- **Two hiders share one film and one ray tracer.** The reyes hider (`"hidden"`) dices
  primitives into grids and shades a grid at a time. The ray hider (`"raytrace"`) shades one hit
  at a time. Both trace shadow and reflection rays through
  [`offline::trace`](../../api/render/offline/trace/).
- **[`trace::Scene::nearest`](../../api/render/offline/trace/Scene.h)** tests every primitive.
- **[`trace::Tracer`](../../api/render/offline/trace/Tracer.h)** gives each hit its own
  `HitShader` and its own run of the shading machine.
- **The reyes hider adds every polygon to the traced scene** as it arrives, before culling. A
  scene that never traces pays for it anyway.
- **A transform into a projected space drops w.** `transform("screen", P)`, `"raster"`, `"NDC"`
  and `depth()` are right only under an orthographic camera.
- **`RiRotate`'s sign is untested.** moya passes the angle to `glm::rotate`. The worked example
  of a unit cube in the RenderMan Interface specification (RISpec 3.1, appendix D) names the face
  each rotation gives, and `glm::rotate` gives each of them. No test pins it.
- **`AreaLightSource` binds a shader to geometry**, but neither hider keeps geometry as a light.
  An area light renders as a point light.
- **A reyes sample keeps only its nearest surface** and sets its opacity to one.
- **Displacement shaders are refused.** `calculatenormal` is a stub, and `Du`, `Dv`, `Deriv` and
  `area` are not declared.
- **`solar` with an angle lights along its axis.** A light shader runs with the surface's
  position only, so it cannot choose a direction inside the cone.

## What it needs

### 1. Transformations that are known to be right

- Pin `RiRotate`'s sign with the specification's unit cube. Each face is one polygon under a
  different rotation, and the specification names where each lands. A picture this tree made
  cannot settle the sign, because both hiders read the same transformation.
- Divide by w for the projected spaces, in the transform built-ins and in a cast.

### 2. An acceleration structure

An acceleration structure goes inside `offline::trace`. Neither hider changes. Which structure is
a choice between real rivals, such as a bounding volume hierarchy and a uniform grid, and it
needs an ADR.

### 3. Shading traced hits in batches

`Tracer` batches the hits of a scanline that share a shader into one run of the machine, as the
reyes hider shades a grid. This is for speed alone. A scanline's hits lie on different primitives
and are cast in different adaptive passes, so differences across them are not derivatives.

### 4. Area lights

- A hider keeps the geometry an `AreaLightSource` names, and runs the light shader at points
  sampled on it.
- The samples are seeded per shading point and per light, not from the film. The reyes hider
  shades a grid before any film sample reaches it.
- Shadows already work through the shared ray tracer, so soft shadows follow.

### 5. Opacity under the reyes hider

Each sample keeps every surface it meets and composites them front to back once the bucket is
done. This is the ray hider's `see()` loop, applied to a sample's list of surfaces.

### 6. Displacement and bump mapping

- **Bump mapping** needs `Du`, `Dv`, `Deriv`, `area` and `calculatenormal`: differences across
  neighbouring shading points. The reyes hider has them from a grid. The ray hider needs its own
  neighbours for each hit, and how it gets them needs an ADR.
- **Displacement under the reyes hider** runs the displacement shader on a grid after dicing and
  before the surface shader, so the surface shader sees the moved `P` and `N`. It grows the
  primitive's bound by `displacementbound`.
- **Displacement under the ray hider** needs tessellation, which the ray tracer does not do. Where
  tessellation happens, and whether the ray hider supports displacement at all, needs an ADR.

### 7. The rest

These have no order against each other, or against steps 2 to 6.

- `solar` receives the surface's illuminance axis and angle along with its position.
- The reyes hider adds geometry to the traced scene only when some shader in the scene traces.
- moya's `--grid` and `--bucket` win over the scene's `Option "limits"`. The reader renders at
  `WorldEnd`, so the command line has to win while the scene is read, not after it.

## The ordering

```
1 Transformations ──> 2 Acceleration ──┬──> 3 Batched hits
                                       └──> 4 Area lights
1 ──> 5 Reyes opacity
1 ──> 6 Displacement and bump
7 The rest   (no order)
```

## What this needs decided

- **The acceleration structure**: step 2, an ADR.
- **How an area light is sampled, and from what seed**: step 4, an ADR.
- **How each hider supplies derivatives**: step 6, an ADR.
- **Displacement under the ray hider**: step 6, an ADR.

## Verification

- The moya suite renders scenes on the CPU and compares them with committed PNGs, one set per
  hider, at a tolerance of 1. [Testing.md](../contributing/Testing.md#moya-references) describes
  it.
- Step 1 is the exception. Its sign is checked against the specification's unit cube, not
  against a picture.
- Steps 2 and 3 must not change any committed picture. The suites' timings show whether they
  helped.
- Steps 4 to 6 each add a reference scene, rendered by every hider that supports the feature.

## Triggers

Each step is due when a scene needs it. Step 2 is due when a scene takes a second to render.
Steps 4, 5 and 6 are due with the first scene that needs soft shadows, glass or smoke under the
reyes hider, or surface detail a texture cannot give.
