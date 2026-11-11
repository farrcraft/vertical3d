# Moya Materials — The BSDF Library, the Node Program and Textures

Drafted 2026-11-11 against `2bbe0def`. This plan takes up section 5 of
[the moya rebuild roadmap](../roadmap/MoyaRebuild.md). When it closes, the new core shades a
surface by its material. A material is a MaterialX graph, compiled into a node program that every
device runs, and it feeds a BSDF library of moya's own that arranges its lobes as OpenPBR Surface
does. Patterns read textures through a cache of tiles, filtered by the footprint a ray carries. A
material's displacement terminal displaces the tessellated surface, and its opacity cuts it out.
The reyes hider and the ray hider keep rendering every RIB scene and every editor render, and no
picture they make changes.

The rebuild's decisions are ADR-0186 to ADR-0194, and the decisions of its first four plans are
ADR-0195 to ADR-0216, all accepted. This plan follows five of them:

* [ADR-0187](../adr/offline/0187-offline-moya-owns-its-algorithms-libraries-own-formats.md):
  moya implements its own scattering models, node program and texture cache. MaterialX reads
  material documents, and OpenEXR reads texture files.
* [ADR-0189](../adr/offline/0189-offline-light-is-carried-as-sampled-spectra.md): a BSDF is
  evaluated at a path's four wavelengths, and a colour becomes a spectrum through the
  RGB-to-spectrum table.
* [ADR-0190](../adr/offline/0190-offline-materials-are-a-bsdf-library-and-materialx-graphs.md):
  materials are moya's BSDF library fed by MaterialX graphs, compiled into a node program every
  device runs.
* [ADR-0194](../adr/offline/0194-offline-stateless-samples-one-picture-per-device.md): each device
  gives one picture on every run, so a texture lookup gives the same value whatever its cache
  holds.
* [ADR-0212](../adr/offline/0212-offline-the-path-state-is-declared-once-laid-out-per-device.md):
  what a ray's footprint adds to a path is declared once in the path state.

The plan keeps the roadmap's order, with five changes:

* **Six decisions the roadmap leaves open are steps of their own**, each before the step that
  builds on it. Three are how the BSDF library holds OpenPBR's layers, how a material graph runs
  on every device, and how a ray's footprint is carried. Three are how textures are stored and
  cached, how a material reaches the scene from RIB, and how a surface's opacity is met.
  [Decisions](#decisions).
* **The BSDF library comes before the node program.** Each lobe is tested on its own, from
  constants set in code, before any material graph feeds it. The node program then only has to
  reach parameters the library already honours. [Step 8](#step-8--a-bsdf-of-several-lobes).
* **The material's displacement terminal replaces the `displacement` primvar.** The geometry plan
  displaced a surface by a primvar until materials existed. This plan evaluates the material's
  displacement on the host when a surface is tessellated.
  [Step 22](#step-22--displacement-from-the-material).
* **Opacity is a step of its own.** OpenPBR's `geometry_opacity` cuts a surface out, which a ray
  has to pass through. The roadmap does not name it. [Step 23](#step-23--opacity).
* **RIB names a material.** No client gives the scene a MaterialX material today, and the editor
  and the importers read none. A RIB file names one, so `moya --paths` renders it and the
  reference renders are RIB files. [Step 25](#step-25--materials-from-rib).

## What the survey found

The tree was read at `2bbe0def`.

**The BSDF and the integrator:**

* **A BSDF has one model, the diffuse lobe**
  ([Surfaces.md](../offline/Surfaces.md#the-bsdf)). `kernel::bsdf::Bsdf` is a tagged type that
  switches over its `Model`, since kernel code has no virtual call. It evaluates, gives a density
  and samples in the frame around the shading normal, and refuses a reflection that crosses the
  geometric surface.
* **A sample names its lobe**: diffuse, glossy, specular or transmission. The integrator keeps a
  depth limit for each
  ([Integrator.md](../offline/Integrator.md#depth-limits-and-russian-roulette)).
* **Every BSDF sample is weighed by its density.** The scatter stage divides the throughput by the
  density and keeps it for MIS. Nothing marks a sample of a perfectly specular lobe, whose density
  is not a number MIS can weigh.
* **The diffuse lobe's colour is the geometry's `Cs` primvar or its material's `Cs`**, in ACEScg,
  made a reflectance through the RGB-to-spectrum table.
* **`BsdfTest.cxx` checks the diffuse lobe's samples against its density by chi-squared**, and its
  reflectance against a closed form.

**The material and the shading point:**

* **A material is a network of nodes**, whose shaders the scene does not read
  ([Scene.md](../offline/Scene.md)). A `shader` node names its shader and its language,
  `materialx`, `osl` or `ri`. A `material` names a `surface` and a `displacement` terminal.
* **A material on the device is a colour** ([Surfaces.md](../offline/Surfaces.md#materials)).
  `kernel::surface::Material` holds its surface node's `Cs`, and whether it gives one.
* **Two primvars reach the device**, `N` and `Cs`, each of three floats
  ([Surfaces.md](../offline/Surfaces.md#primvars)). No texture coordinate, tangent or primvar of
  another width does.
* **A shading point holds its position, normals, frame, `uv` and corner weights**, but no
  differential. A ray carries no footprint.
* **A surface is displaced by its `displacement` primvar** when it is tessellated, within its
  `displacementBound` ([Geometry.md](../offline/Geometry.md#tessellation-and-displacement)).

**Images and textures:**

* **A light's image is an EXR read whole**, through `output::readExr()`, converted into ACEScg by
  its chromaticities ([Lights.md](../offline/Lights.md#images)). A lookup reads the texel that
  holds a place, with no filtering. A dome light's sampling needs the whole image.
* **The scene holds no search path.** A light's image and an IES profile are opened relative to
  the working directory.
* **The RIB reader reads `MakeTexture`** into `Handler::makeTexture()`. The old hiders' textures
  are in `api/render/offline/texture`, which section 7 retires.

**The clients:**

* **The RIB client makes a node of language `ri` for `Surface`**, with the attribute state's
  colour as `Cs` ([Rib.md](../offline/Rib.md#the-scene-client)). RI's built-in shaders become
  OpenPBR materials in section 7.
* **The editor gives every surface one material**, RI's `matte`, through `SceneTranslator`.
* **The importers read no material.** The glTF and USD readers leave materials and shaders out.

**Dependencies:** MaterialX is not a dependency. The vendored vcpkg has a `materialx` port at
1.39.1. USD is built without its imaging.

**The tests:** moya's suite has 29 RIB scenes, each with a committed description and hit view,
and 7 reference renders of the path tracer held within five standard errors
([Testing.md](../contributing/Testing.md#moya-references)).

---

## Decisions

| ADR | Decides | Step |
|---|---|---|
| 0222 | How the BSDF library holds OpenPBR's layers | [1](#step-1--the-record-the-bsdf-librarys-layers) |
| 0223 | How a material graph runs on every device | [2](#step-2--the-record-the-node-program) |
| 0224 | How a ray's footprint is carried | [3](#step-3--the-record-a-rays-footprint) |
| 0225 | How textures are stored, cached and filtered | [4](#step-4--the-record-the-texture-cache) |
| 0226 | How a material reaches the scene from RIB | [5](#step-5--the-record-materials-in-rib) |
| 0227 | How a surface's opacity is met | [6](#step-6--the-record-opacity) |

Each record step researches how production renderers make the choice, and the owner chooses
among the options before the record is drafted.

---

## The steps

| | What | Where | ADR | State |
|---|---|---|---|---|
| [1](#step-1--the-record-the-bsdf-librarys-layers) | The record: the BSDF library's layers | `docs/adr` | **0222** | accepted |
| [2](#step-2--the-record-the-node-program) | The record: the node program | `docs/adr` | **0223** | accepted |
| [3](#step-3--the-record-a-rays-footprint) | The record: a ray's footprint | `docs/adr` | **0224** | accepted |
| [4](#step-4--the-record-the-texture-cache) | The record: the texture cache | `docs/adr` | **0225** | accepted |
| [5](#step-5--the-record-materials-in-rib) | The record: materials in RIB | `docs/adr` | **0226** | open |
| [6](#step-6--the-record-opacity) | The record: opacity | `docs/adr` | **0227** | open |
| [7](#step-7--materialx) | MaterialX as a dependency | `vcpkg.json`, `ports` | — | open |
| [8](#step-8--a-bsdf-of-several-lobes) | A BSDF of several lobes, and specular samples in the integrator | `moya/libmoya/kernel` | 0222 | open |
| [9](#step-9--microfacets-and-the-conductor) | Microfacets, the conductor and energy compensation | `moya/libmoya/kernel/bsdf` | 0222 | open |
| [10](#step-10--the-dielectric) | The dielectric: reflection and transmission | `moya/libmoya/kernel/bsdf` | 0222 | open |
| [11](#step-11--openpbrs-base-specular-and-coat) | OpenPBR's base, specular and coat | `moya/libmoya/kernel/bsdf` | 0222 | open |
| [12](#step-12--fuzz-and-thin-film) | Fuzz and thin film | `moya/libmoya/kernel/bsdf` | 0222 | open |
| [13](#step-13--emission) | Emission from a material | `moya/libmoya` | 0222 | open |
| [14](#step-14--compiling-a-material) | Compiling a MaterialX material | `moya/libmoya/material` | 0223 | open |
| [15](#step-15--the-node-program-on-the-device) | The node program on the device | `moya/libmoya/kernel` | 0223 | open |
| [16](#step-16--primvars-by-name-and-tangents) | Primvars by name, and tangents | `moya/libmoya/geometry`, `kernel` | — | open |
| [17](#step-17--procedural-patterns) | Procedural patterns | `moya/libmoya/kernel` | 0223 | open |
| [18](#step-18--a-rays-footprint) | A ray's footprint | `moya/libmoya/kernel` | 0224 | open |
| [19](#step-19--the-texture-maker) | The texture maker | `moya/libmoya`, `moya` | 0225 | open |
| [20](#step-20--the-texture-cache) | The texture cache, filtered lookups and UDIM | `moya/libmoya/texture`, `kernel` | 0225 | open |
| [21](#step-21--bump-and-normal-maps) | Bump and normal maps | `moya/libmoya/kernel` | — | open |
| [22](#step-22--displacement-from-the-material) | Displacement from the material | `moya/libmoya/geometry` | — | open |
| [23](#step-23--opacity) | Opacity | `moya/libmoya/kernel` | 0227 | open |
| [24](#step-24--other-surface-shaders) | Standard Surface, UsdPreviewSurface and glTF PBR | `moya/libmoya/material` | — | open |
| [25](#step-25--materials-from-rib) | Materials from RIB | `api/render/offline/rib`, `moya/libmoya/ri` | 0226 | open |
| [26](#step-26--the-reference-renders) | The reference renders | `moya/tests` | — | open |
| [27](#step-27--speed) | Speed: a textured scene against the baseline | `moya/bench` | — | open |
| [28](#step-28--the-documents) | The reference pages for materials and textures | `docs` | — | open |

## What blocks what

```
1 ADR ──> 8 Lobes ──> 9 Microfacets ──> 10 Dielectric ──> 11 Base, specular, coat ──> 12 Fuzz, film
                                                                    │
                                                                    └──> 13 Emission
2 ADR ──> 14 Compile (after 7) ──> 15 Program (after 11) ──┬──> 16 Primvars ──> 17 Procedurals
                                                           ├──> 22 Displacement
                                                           ├──> 24 Other shaders
5 ADR ──────────────────────────────────────────────────────┴──> 25 Materials from RIB
3 ADR ──> 18 Footprint ─────────────────┐
4 ADR ──> 19 Maker ──> 20 Cache (after 15, 18) ──> 21 Bump and normal maps (after 16)
6 ADR ──> 23 Opacity (after 15)

(8 to 25) ──> 26 References, 27 Speed ──> 28 Documents
```

Steps 1 to 7 depend on nothing but each other. Step 8 needs the record of step 1, and each lobe
after it builds on the one before. Step 13 needs the base of step 11. Step 14 needs MaterialX and
the record of step 2, and step 15 needs the program and a BSDF of OpenPBR's parameters to feed.
Steps 16, 17, 22, 23, 24 and 25 each need the node program. Step 18 needs only the integrator and
the record of step 3. Step 20 needs the maker's files, the image node of the program and the
footprint. Step 21 needs tangents and textures. Each step updates its own section of the reference
documents, and step 28 checks that they read as a whole.

## How every step is verified

* **The build and the gates.** Everything compiles at `/W4 /WX`, and cpplint, `/analyze`,
  clang-tidy and every review gate stay clean. Kernel code passes `kernel.ts`.
* **The suites.** The offline suite, moya's suites and the editor's suite pass. Each step with
  code adds cases, and `failsfirst.ts` is run on the change to show that each new case fails
  without it.
* **Every lobe meets the same three checks.** Its samples follow its density by chi-squared,
  through `BsdfTest.cxx`'s check made a helper every lobe calls. Its value is reciprocal, the same
  with the two directions swapped. A white furnace gives the energy a closed form says the lobe
  keeps.
* **The statistical oracle.** A picture is compared with its expected value, or with another
  picture, through the path tracer's comparison within standard errors.
* **One picture on every run.** A render on one thread and on every thread gives the same bytes,
  textured ones included, whatever the texture cache held when the render began.
* **Linux.** portable.yml's clang job builds `v3dlib_moya` and runs moya's suite under the
  sanitizers, in the spectral and the RGB build.
* **No old picture changes.** The old hiders read RIB as before, so every committed PNG in
  `moya/tests/data/` stays as it is, and the editor's render window shows the same picture. An RI
  `Surface` stays diffuse in the new core until section 7, so the path tracer's seven reference
  renders stay as they are.

---

### Step 1 — The record: the BSDF library's layers

**The question.** How a shading point's BSDF holds the lobes OpenPBR Surface layers: a diffuse or
metal base, a dielectric specular layer with transmission below it, a coat, fuzz and a thin film.
Kernel code allocates nothing, so a shading point holds a bounded number of lobes.

**The options:**

* **One fixed model of OpenPBR's parameters**, evaluated lobe by lobe with each layer scaling the
  ones below it by its albedo, as Arnold's Standard Surface, Cycles' Principled BSDF and
  RenderMan's PxrSurface hold theirs. A sample picks a lobe by an estimate of its albedo. It is the
  fastest, and the size of a shading point is fixed. A MaterialX graph of other BSDF nodes has to
  be mapped onto its parameters.
* **A list of weighted closures** that a material's graph builds at each shading point, as OSL's
  closures and Cycles' SVM closures hold a BSDF, and as MaterialX's `layer`, `mix` and `add` nodes
  describe one. Any MaterialX BSDF graph renders as authored. OpenPBR's albedo scaling has to be
  carried by the layer node, and the list has a longest length.
* **Both**: the fixed model for `open_pbr_surface`, and closures for every other BSDF graph.

The record also settles how energy lost to a microfacet lobe's single scattering is put back,
whether by Kulla and Conty's tables of albedo or by Turquin's scale. It settles how a lobe is
picked for a sample, and whether a material's emission is found by light samples or by the BSDF's
alone.

**Drafted** as
[ADR-0222](../adr/offline/0222-offline-a-bsdf-is-a-list-of-weighted-closures.md): a bounded list
of weighted closures, which `open_pbr_surface` and MaterialX's `layer` node weight by the albedo
of the layers above, as Cycles' layered Principled BSDF does. A sample picks a closure by its
estimated albedo. Kulla and Conty's compensation lobe restores a microfacet lobe's energy, and an
emitting material makes its prototype a light of the light tree.

### Step 2 — The record: the node program

**The question.** How a MaterialX graph of patterns runs at a shading point on every device. The
CPU runs kernel code compiled ahead of time, and a GPU will too.

**The options:**

* **An instruction list interpreted on the device**, over a small store of registers, as Cycles'
  SVM runs a shader graph. Each node is an instruction with its inputs and outputs. One program
  format serves every device, and nothing is compiled while rendering. Each node costs a branch.
* **Code generated for each material and compiled for each device**, from MaterialX's shader
  generators, as GPU renderers compile OSL or MDL to PTX at render time. Each node is inlined, but
  every machine needs a compiler for every device, and a cache of what it compiled.
* **A material evaluated on the host into constants and textures**, as many glTF renderers take
  one. The device reads only constants and texture lookups. It is the cheapest, and loses every
  pattern that varies with the shading point's position.

The record also settles where MaterialX's node graphs are flattened into its standard nodes,
which nodes the program implements, and how a program reads primvars, textures and the shading
point.

**Drafted** as
[ADR-0223](../adr/offline/0223-offline-a-material-is-an-interpreted-node-program.md): a program of
instructions over a store of registers, compiled on the host and run by one interpreter on every
device, as Cycles' SVM runs. Compiling flattens every node with a graph implementation into
standard nodes, and stops at surface shader and BSDF nodes, which build the closure list. An
instruction reads the shading point, a primvar by its slot or a texture by its handle.

### Step 3 — The record: a ray's footprint

**The question.** How wide a ray is where it meets a surface, so that a texture is filtered over
the area a pixel's sample covers. A path state's size is ADR-0212's concern on every device.

**The options:**

* **Ray differentials**, after Igehy, as pbrt and RenderMan carry them: the offsets of two
  neighbouring rays' origins and directions, carried through every bounce by rules for
  reflection and refraction. They give an anisotropic footprint, and add four vectors to a path.
* **Ray cones**, after Akenine-Möller and others, as Falcor and Unreal Engine carry them: a width
  and a spread angle, grown at each bounce by the surface's curvature and roughness. Two floats,
  and an isotropic footprint.
* **Camera differentials alone**, as pbrt-v4 approximates later bounces from the camera's
  differentials at the hit, and as Cycles compresses a differential to one float for the
  position and one for the direction.

The record also settles how a rough bounce widens the footprint, and whether a lookup filters
trilinearly or over an anisotropic footprint.

**Drafted** as [ADR-0224](../adr/offline/0224-offline-a-rays-footprint-is-a-cone.md): a ray cone,
a width and a spread angle, carried through reflection and refraction. At each hit it grows by
the surface's curvature, its spread widens by the lobe's roughness, and it is projected onto the
surface as an ellipse. A lookup takes a bounded number of trilinear probes along the ellipse's
major axis, as OpenImageIO's texture system does.

### Step 4 — The record: the texture cache

**The question.** How texture files are held while rendering, within a limit of memory, and what
a lookup does when the tile it needs is not in memory. ADR-0194 requires the same picture on every
run, so a lookup cannot read a coarser level because a tile was absent.

**The options:**

* **Tiled, mipmapped EXR files, cached tile by tile on the host**, as RenderMan's `.tex` and
  Arnold's `.tx` files are, with moya's maker writing them. Any other image is made into one when
  the scene is read. Memory stays within its limit. A GPU loads the tiles a pass misses between
  passes, as OptiX's demand loading does, and runs the pass's missed lookups again.
* **Every image loaded whole and mipmapped in memory**, as Cycles loads its images. The same on
  every device, with no miss. Memory grows with every texture.
* **OpenImageIO's cache and texture system.** Every format it reads, and filtering as Arnold's and
  RenderMan's have used it. Its algorithms are not moya's, so ADR-0187 would be amended, and it
  runs on the CPU alone.

The record also settles which file formats a texture is read from, when an image is converted
into ACEScg, and how a texture's path is found.

**Drafted** as [ADR-0225](../adr/offline/0225-offline-textures-are-tiled-exr-cached-by-tile.md):
tiled, mipmapped OpenEXR files, cached tile by tile, as Cycles' texture cache holds `.tx` files. A
missed lookup runs again once its tile is loaded, never at a coarser level. A colour texture is
converted into ACEScg when it is made, and a data texture keeps its values. Any other image is made
into one in a cache directory, keyed by its content and colour space. A path is found relative to
the file that names it, then along RI's texture search path.

### Step 5 — The record: materials in RIB

**The question.** How a RIB file names a MaterialX material, so `moya --paths` renders it from a
file. No client gives the scene one today.

**The options:**

* **RenderMan's `Bxdf` and `Pattern` requests**, with `PxrSurface` mapped onto OpenPBR and
  RenderMan's patterns onto MaterialX nodes, as step 18 of the path tracer plan mapped RenderMan's
  lights. RenderMan's RIB reads as written. The mapping is large, and MaterialX content still has
  no route.
* **A MaterialX document named from RIB**: a request naming a `.mtlx` file and a material in it,
  which the scene client reads into nodes of language `materialx`. Any MaterialX material renders
  as authored. No other renderer reads that request.
* **Both.**

The record also settles the request's name and parameters, and how a material's inputs are
overridden from RIB.

### Step 6 — The record: opacity

**The question.** How a ray meets a surface that OpenPBR's `geometry_opacity` makes partly
transparent, for camera, indirect and shadow rays.

**The options:**

* **The integrator passes through**: a hit whose opacity is drawn as transparent continues the
  ray from just past it, as a shadow link passes through instances. The traversal is unchanged,
  and each pass is another traversal from the world's root.
* **A test inside the traversal**, as Embree's filter functions, OptiX's any-hit programs and HIP
  RT's filters run one. A ray traverses once, and the traversal runs the node program, so each
  device's traversal reaches the material.

The record also settles whether a shadow ray is attenuated by a transparency or passes by a draw,
and the most surfaces a ray passes through.

### Step 7 — MaterialX

* **MaterialX joins the `moya` feature** in `vcpkg.json`, with the libraries that read a document
  and its standard library of node definitions. Its shader generators are not built.
* **The OpenPBR version is checked.** If the baseline's MaterialX 1.39.1 defines an
  `open_pbr_surface` older than OpenPBR Surface 1.1, an overlay port in `ports/` brings the
  release that defines 1.1.
* **The standard libraries are installed beside moya**, so a document's node definitions resolve
  without a MaterialX install.
* **[Dependencies.md](../contributing/Dependencies.md)** names the port and why.

**Tests:** a document of the standard library loads, and `open_pbr_surface` resolves to its node
graph.

### Step 8 — A BSDF of several lobes

* **The BSDF holds several lobes** as the record of step 1 decides, with the diffuse lobe as the
  first. A sample picks a lobe, and evaluation and density sum over the lobes.
* **A specular sample is marked.** The scatter stage does not divide by a density it cannot
  weigh, and a light the next ray meets takes its whole weight, as no light sample could reach it.
* **The chi-squared check becomes a helper** every lobe's case calls.

**Tests:** every committed reference render of the path tracer is unchanged. A BSDF of two
diffuse lobes of half weight renders as one. A perfect mirror in front of a light shows the light
with no MIS weight lost.

### Step 9 — Microfacets and the conductor

* **The GGX distribution**, with Smith's masking and shadowing, sampled by its visible normals.
* **A conductor** of OpenPBR's base colour and specular colour through the F82 tint, at a path's
  four wavelengths.
* **Anisotropy**, by OpenPBR's roughness and anisotropy, along the shading point's tangent.
* **Energy compensation** as the record of step 1 decides, with any table it needs built when
  moya is built.

**Tests:** chi-squared, reciprocity, and a white furnace in which a white conductor of every
roughness keeps all its energy. Its reflectance at normal incidence is its base colour.

### Step 10 — The dielectric

* **Reflection and transmission by Fresnel's equations**, smooth and rough, with total internal
  reflection.
* **A smooth interface is specular**, marked as step 8 marks it, and a rough one glossy or
  transmission.
* **A thin-walled surface** transmits without bending, as OpenPBR's `geometry_thin_walled` says.
* **The medium inside is not built.** OpenPBR's transmission depth and scatter wait for
  section 8's volumes.

**Tests:** Fresnel's closed forms at several angles, chi-squared over both hemispheres,
reciprocity across the interface by the ratio of the indices, and a white furnace of a clear
sphere.

### Step 11 — OpenPBR's base, specular and coat

* **The diffuse base** as OpenPBR's energy-preserving Oren–Nayar lobe, with its roughness, and the
  metal base mixed by `base_metalness`.
* **The specular layer over the base**, its albedo scaling what lies below it.
* **The coat** over both, with its own roughness, index and colour, and the darkening OpenPBR
  gives the layers under it.
* **Subsurface is the diffuse base** until section 8, with its colour.
* **A BSDF of OpenPBR's parameters** set from constants, which the node program feeds from step 15.

**Tests:** OpenPBR's limiting cases: a metalness of one is the conductor, a specular weight of
zero is the base alone, and a coat of weight zero changes nothing. A white furnace keeps the energy
each layer's closed form allows.

### Step 12 — Fuzz and thin film

* **Fuzz**, OpenPBR's sheen, as the lobe OpenPBR names, over the coat.
* **A thin film** over the specular layer, its interference evaluated at each of a path's four
  wavelengths, so its colours come from the spectrum and need no fit to RGB.

**Tests:** chi-squared and reciprocity for fuzz, and a film of zero thickness that changes
nothing. A film's reflectance at a wavelength matches the closed form for one layer.

### Step 13 — Emission

* **A material emits** OpenPBR's `emission_luminance` in its `emission_color`, found as the record
  of step 1 decides.
* **A mesh light still makes a geometry emit**, as before, and both add.

**Tests:** an emitting sphere in a black scene shows its luminance. A diffuse plane beside it
receives the irradiance a closed form gives, within its errors.

### Step 14 — Compiling a material

* **MaterialX reads a material** from a scene's nodes of language `materialx`, and flattens its
  node graphs into its standard nodes.
* **The material's surface terminal** names `open_pbr_surface`, and its inputs feed the BSDF of
  step 11.
* **A program** as the record of step 2 decides, made on the host for each material when the
  surfaces are made, and rebuilt when a material changes, as the hierarchy is.
* **A node the program does not implement** is reported, and its output is its default.

**Tests:** a material of constant inputs compiles to the BSDF those constants give. A node graph
flattens to the nodes MaterialX's own flattening gives. An unknown node is reported once.

### Step 15 — The node program on the device

* **The program runs at a shading point** before the BSDF is built, and gives its parameters.
* **MaterialX's standard nodes of math, colour, logic and channels**, each as MaterialX defines
  it.
* **A colour input is converted into ACEScg** from the space its document names, when the program
  is compiled.

**Tests:** each node against its definition at sample inputs. A material that mixes two colours
by a constant renders as the mix. The path tracer's references are unchanged.

### Step 16 — Primvars by name and tangents

* **Every primvar a program reads reaches the device**, by name, of one to four floats, spread
  over the primitives as `N` and `Cs` are.
* **Texture coordinates** are the `st` primvar, as RI names them, or MaterialX's `texcoord`.
* **A tangent frame from the texture coordinates**, for anisotropy and normal maps, or from a
  `tangent` primvar where a geometry gives one.
* **MaterialX's geometric nodes**: `texcoord`, `geompropvalue`, `position`, `normal`, `tangent`
  and `bitangent`, in the spaces MaterialX names.

**Tests:** a primvar of each class and width reads back at a shading point. A tangent lies along
the direction `s` increases.

### Step 17 — Procedural patterns

* **MaterialX's noises**: `noise2d`, `noise3d`, `fractal3d`, `cellnoise2d`, `cellnoise3d` and
  `worleynoise2d` and `3d`, as MaterialX's library defines them.
* **Its procedural shapes and ramps**, such as `ramplr`, `ramp4` and `checkerboard`.

**Tests:** each noise against MaterialX's definition at sample points, and a checkerboard's
squares where its frequency puts them.

### Step 18 — A ray's footprint

* **Camera rays carry a footprint** as the record of step 3 decides, from the camera's projection
  and the pixel filter.
* **Each bounce carries it on**, widened by the lobe's roughness.
* **A shading point gives its footprint** in its texture coordinates, which a lookup filters over.

**Tests:** a plane square to the camera gives a footprint of one pixel at each pixel. A plane at an
angle gives the footprint a closed form gives. A rough bounce's footprint is wider than a smooth
one's.

### Step 19 — The texture maker

* **`moya --maketexture <image> <texture>`** makes a texture file as the record of step 4 decides,
  as RI's `MakeTexture` names one.
* **RIB's `MakeTexture`** makes one through the scene client.
* **The wrap modes and the filter** are the request's, and the image's colour space is converted
  as the record settles.

**Tests:** a texture made from an image holds its levels, each the box filter of the level above.
Wrap modes and a texture of one texel hold at their edges.

### Step 20 — The texture cache

* **The cache** as the record of step 4 decides, within a memory limit the options give.
* **MaterialX's `image` and `tiledimage` nodes** look up through it, filtered over the footprint.
* **UDIM tiles**, by the `<UDIM>` token in a texture's path.
* **A texture's path** is found as the record settles, and the scene's options gain RI's texture
  search path.

**Tests:** a texture far from the camera reads a coarser level than one near it. A render with a
cache of one tile gives the same picture as one with room for every tile. A UDIM tile is read for
the coordinates it covers.

### Step 21 — Bump and normal maps

* **MaterialX's `normalmap` node** turns a tangent-space normal map into a shading normal.
* **MaterialX's `bump` and `heighttonormal`** turn a height into one, by the height's differences
  over the footprint.
* **A normal that would face away** from the outgoing direction is bent back, so a bumped surface
  shows no black.

**Tests:** a flat normal map changes nothing. A tilted one turns the shading normal by its angle.
A bump of a constant height changes nothing.

### Step 22 — Displacement from the material

* **The material's displacement terminal** gives a surface's height when it is tessellated,
  through the node program run on the host's CPU device, in place of the `displacement` primvar.
* **A displacement of a vector** moves a point along each axis, as MaterialX's `displacement`
  node's vector input says.
* **The height is held within the displacement bound**, as before.

**Tests:** every displaced reference scene renders as it did, its height now from a material. A
textured displacement moves the points the texture says.

### Step 23 — Opacity

* **OpenPBR's `geometry_opacity`** cuts a surface out, as the record of step 6 decides, for camera,
  indirect and shadow rays.
* **A cut-out surface casts the shadow of its opacity.**

**Tests:** a plane of opacity zero leaves the picture as it was without it. A plane of half opacity
shows the expected mix, within its errors. A leaf cut out by a texture casts the leaf's shadow.

### Step 24 — Other surface shaders

* **Standard Surface, UsdPreviewSurface and glTF PBR** render, each mapped onto OpenPBR's
  parameters, through a translation MaterialX ships where one exists.
* **Another surface shader** is reported, and the surface takes OpenPBR's defaults.

**Tests:** each shader's documented defaults render as OpenPBR's equivalent material. A metal and a
glass of each render as the OpenPBR ones do, within their errors.

### Step 25 — Materials from RIB

* **RIB names a material** as the record of step 5 decides, and the scene client makes its nodes.
* **`moya --scene` prints the material**, and its description is committed with the scene.
* **RI's `Surface` is unchanged**: a node of language `ri`, rendered diffuse until section 7.

**Tests:** a RIB file naming a material gives the scene the nodes MaterialX's document holds. An
input overridden from RIB reaches the node.

### Step 26 — The reference renders

* **A set of scenes made for materials**, as RIB files in `moya/tests/data`. They are a furnace of
  each lobe, a metal, a glass, a coated and a fuzzed material, and a thin film. A textured, a
  normal-mapped, a displaced and a cut-out surface follow.
* **Each is rendered and committed** as the path tracer's references are, with each pixel's
  standard error, and compared within five of them.

**Tests:** the committed renders, and a case that renders each with a material input changed and
checks that the comparison fails.

### Step 27 — Speed

* **`moya_bench` gains a textured scene**: the lit room with a material of textures and a layered
  OpenPBR surface. It reports samples a second, and the share of time in the node program and in
  texture lookups beside traversal's.
* **The baseline** in `moya/bench/Baseline.md` gains the figures, from the same machine.

**Tests:** the benchmark's short pass times the textured scene.

### Step 28 — The documents

* **New reference pages**: `docs/offline/Materials.md` for the BSDF library, its lobes and the
  node program, and `docs/offline/Textures.md` for the maker, the cache, filtering and UDIM. The
  docs site's summary lists them.
* **[Surfaces.md](../offline/Surfaces.md)** names the primvars by name, tangents and the material
  on the device.
* **[Integrator.md](../offline/Integrator.md)** names specular samples and a ray's footprint.
* **[Geometry.md](../offline/Geometry.md)** names displacement from the material.
* **[Scene.md](../offline/Scene.md)** names the options' texture search path and memory limit.
* **[Rib.md](../offline/Rib.md)** names how a material is named and `MakeTexture`.
* **[The offline README](../offline/README.md)**'s section on the new core says it shades by
  materials.
* **The moya manual's [CommandLine.md](../manual/moya/CommandLine.md)** names `--maketexture`.
* **[Testing.md](../contributing/Testing.md)** names the lobes' checks and the material references.
* **Each record's `Documented in` line** names the reference page that states its rule, and
  ADR-0190's names Materials.md.
* **The roadmap's section 5** links here, and says so when this plan closes.

---

## Working beside the editor scripting plan

[EditorScripting](EditorScripting.md) is open. It takes ADR-0217 to ADR-0221, so this plan's
records start at 0222. Its step 16 adds an option to `moya/moya.cxx` and
`docs/manual/moya/CommandLine.md`, where step 19 adds `--maketexture`. Each lands as a commit of
its own. This plan changes nothing the editor sends moya: the editor's surfaces keep RI's `matte`,
and giving the editor materials is work for a plan of the editor's own.
