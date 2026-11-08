# Moya Rebuild: A Spectral Path Tracer

What moya needs to become a modern production renderer, and in what order. moya stands on its
own, renders to a high standard on the CPU and on GPUs, integrates first with the editor, and
plugs into other pipelines through RIB and USD. It is written for whoever starts the rebuild.

moya today implements RenderMan as it was before 2016: a reyes hider, a Whitted ray hider and an
interpreted RenderMan Shading Language. Every current production renderer is a path tracer in
which the integrator gathers light. The rebuild replaces moya's core with one, and keeps the name,
the executable and the editor's two ways of rendering.
[The offline renderer](../offline/README.md) describes the code as it stands.

## Glossary

- **Path tracing**: estimating light at a pixel by following random paths from the camera, each
  bouncing until it reaches a light or ends.
- **Integrator**: the part of a renderer that builds paths and decides where each bounce goes.
- **BSDF**: a model of how a surface scatters light from one direction into another. The
  integrator evaluates it and samples directions from it.
- **NEE** (next event estimation): casting a ray at a sampled point on a light at each bounce.
- **MIS** (multiple importance sampling): weighting light samples against BSDF samples, so
  neither a small light nor a sharp reflection is noisy.
- **Device**: a processor moya renders on: the CPU, or one kind of GPU.
- **Kernel subset**: the restricted C++ that compiles for every device.
- **AOV** (arbitrary output variable): an image other than the beauty image, such as albedo,
  normal or one light's contribution.
- **LPE** (light path expression): a regular expression over the events of a path, which names
  the light an AOV collects.

## Decisions

The rebuild follows these records.

| ADR | Decides |
|---|---|
| [0186](../adr/offline/0186-offline-moya-renders-by-path-tracing-alone.md) | moya is a unidirectional path tracer, and reyes and the RenderMan Shading Language are retired |
| [0187](../adr/offline/0187-offline-moya-owns-its-algorithms-libraries-own-formats.md) | moya implements its own algorithms, and uses reference libraries for formats, OpenSubdiv and OIDN |
| [0188](../adr/offline/0188-offline-a-retained-scene-interface-moya-owns.md) | A retained scene interface is moya's API, and RIB, the editor and Hydra are its clients |
| [0189](../adr/offline/0189-offline-light-is-carried-as-sampled-spectra.md) | Light is carried as four sampled wavelengths |
| [0190](../adr/offline/0190-offline-materials-are-a-bsdf-library-and-materialx-graphs.md) | Materials are moya's BSDF library fed by MaterialX graphs, with OSL as a second pattern language |
| [0191](../adr/offline/0191-offline-one-c-kernel-source-for-every-device.md) | One C++ kernel source compiles for the CPU, HIP and CUDA |
| [0192](../adr/offline/0192-offline-path-stages-run-fused-on-the-cpu-queued-on-the-gpu.md) | The integrator's stages run fused on the CPU and queued on a GPU |
| [0193](../adr/offline/0193-offline-moya-owns-its-ray-acceleration-structure.md) | moya builds its own wide BVH, and intersects every primitive but triangles by its own code |
| [0194](../adr/offline/0194-offline-stateless-samples-one-picture-per-device.md) | Samples come from ZSobol, and each device gives one picture on every run |

## What carries over

- **The RIB reader** in `api/render/offline/rib`, and the lexer it shares in
  `api/render/offline/source`. RIB becomes a client of the scene interface.
- **The pixel filters** in `offline::film`. The film around them is rebuilt for spectra and
  AOVs.
- **`api/image`**, for reading and writing the 8-bit formats it already handles.
- **The editor's two paths**: `MoyaDelegate` for interactive renders and `moya.exe` for batches.
  The delegate moves onto the scene interface.
- **The fuzzed parsers and the reference-image tests.** Their reference images are made again.

## What is retired

The reyes hider, the ray hider, `offline::trace`, the shading language compiler and machine in
`api/render/offline/sl`, the built-in shaders, and RI's light shaders. The moya manual is
rewritten for the new renderer. Section 7 removes them, once the new core renders what RIB
scenes describe.

## What it needs

### 1. The foundation

Done. Everything later runs on this. [MoyaFoundation](../plans/completed/MoyaFoundation.md) was
its plan, and [the offline README](../offline/README.md#the-new-core) describes what it built.

- **The kernel subset and the CPU device.** The rules of the subset: no virtual calls, no
  exceptions and no allocation in shared code. A device interface for memory, ray traversal and
  textures, with the CPU as its first implementation. Threads over tiles of the image.
- **Spectral types.** A sampled spectrum of four wavelengths with hero sampling. RGB to spectrum
  conversion by Jakob and Hanika's table. Conversion to an RGB working space through the CIE
  matching functions.
- **The sampler.** ZSobol: an Owen-scrambled Sobol sequence in Morton order, seeded by the frame.
- **The film.** Channels for AOVs, accumulated in a fixed order. The existing pixel filters.
  Pictures on request while sampling continues.
- **Output.** OpenEXR with half and float channels and several parts, through the OpenEXR
  library. Colour management through OpenColorIO, with the ACES 2.0 configuration.

### 2. The scene interface

- **The retained scene**: geometry prototypes and instances, materials, lights, cameras and
  render outputs, each created, modified and deleted by handle. Transformations and geometry
  carry samples over the shutter.
- **Change tracking**, so a modification rebuilds only the structures it touched.
- **The RIB client**, which keeps RI's graphics state and creates scene objects as RIB requests
  arrive. It reads every request moya reads today, and the geometry and instancing that section
  3 adds.
- **The editor client.** `MoyaDelegate` creates the editor's scene through the interface.

### 3. Geometry and ray traversal

- **The BVH.** Four or eight children per node, traversed with AVX2 or AVX-512. Built by binned
  surface area with spatial splits. Motion over several segments, and nested instances.
- **Triangles**, intersected watertight, and ray starts offset by Wächter and Binder's method.
- **Curves**, round and flat, in every RI basis. **Points**, and the RI quadrics.
- **Subdivision surfaces** through OpenSubdiv, with creases and corners.
- **Tessellation** to an edge length in pixels, and displacement of the tessellated surface
  within a displacement bound.
- **Deformation motion**: geometry given at several times inside a motion block.

### 4. The path tracer

- **The integrator stages**: generate, intersect, shade, sample lights, scatter and accumulate,
  over an explicit path state.
- **NEE and MIS**, Russian roulette, and a depth limit per kind of bounce.
- **Lights**: rect, disk, sphere and cylinder lights, a distant light with an angle, mesh lights,
  and a dome light from an HDR image. Each can be sampled by solid angle.
- **An importance-sampled environment**, from a luminance distribution over the image.
- **A light tree** that picks lights by their expected contribution, after Conty Estevez and
  Kulla.
- **Light linking**, and IES profiles.
- **Adaptive sampling** to a noise threshold, and checkpoint and resume.

### 5. Materials and textures

- **The BSDF library**: GGX with visible-normal sampling and energy compensation, a dielectric
  with transmission, coat, sheen and thin film, as OpenPBR Surface 1.1 arranges them.
- **The MaterialX node program**: MaterialX standard nodes compiled into a small program that
  every device runs, feeding the BSDF library.
- **The texture cache**: tiles of mipmapped textures loaded on demand within a memory limit.
  Lookups filtered by the footprint of a ray's differentials. UDIM tiles.
- **A texture maker**: `moya.exe` converts an image into a tiled, mipmapped EXR, as RI's
  `MakeTexture` names.

### 6. Output for compositing

- **LPE AOVs**: each AOV names a light path expression, and the integrator reports the event of
  each bounce.
- **Light groups**, so a light's contribution can be changed after rendering.
- **Cryptomatte** channels for objects and materials, with the manifest in the EXR header.
- **Deep EXR**: a list of depth samples per pixel, with their IDs.
- **Denoising** through OIDN, from the beauty, albedo and normal images. Albedo and normal come
  from the first hit that is not a perfect mirror.

### 7. The retirement

- RIB scenes that render today render through the new core, with the built-in shaders mapped to
  OpenPBR materials.
- The reyes hider, the ray hider, `offline::trace` and `api/render/offline/sl` are removed.
- The moya manual and the offline reference pages are rewritten. ADR-0026, ADR-0078, ADR-0093,
  ADR-0094, ADR-0095, ADR-0113, ADR-0114 and ADR-0125 are superseded. ADR-0023, ADR-0025,
  ADR-0076 and ADR-0092 are amended.

### 8. Volumes, subsurface and hair

- **Volumes** by null-scattering tracking, with a majorant grid. Homogeneous media, then OpenVDB
  grids read through the OpenVDB library.
- **Random-walk subsurface scattering**, as a lobe of the BSDF library.
- **A hair BSDF** after Chiang and others, on round curves.
- **Volumetric lights**: emission from a volume, sampled by the light tree.

### 9. Interactive rendering

- **Edits while rendering**: the editor's changes reach the scene interface, and sampling
  restarts with only the touched structures rebuilt.
- **Progressive pictures** at a rate the editor's render view sets.

### 10. The HIP back end

- HIP builds the kernel subset for AMD GPUs, and HIP RT traverses boxes and triangles.
- moya's own code intersects curves, points and quadrics, through HIP RT's custom primitives.
- Each stage runs as its own kernel over queues of paths, sorted by material.
- The texture cache and memory on the device.
- Tests compare its pictures with the CPU's within a statistical tolerance.

### 11. The OptiX back end

CUDA builds the kernel subset for NVIDIA GPUs, and OptiX traverses boxes and triangles. It shares
everything else with the HIP back end.

### 12. A Hydra render delegate

- The USD port builds Hydra without OpenGL.
- A delegate translates a Hydra scene into the scene interface, so a USD pipeline renders through
  moya.

### 13. OSL patterns

OSL shaders as a second way to write patterns, on the CPU and through OptiX. Their closures map
onto the BSDF library.

### 14. Path guiding and caustics

- **Path guiding** through OpenPGL, on surfaces and in volumes.
- **Manifold next event estimation**, for caustics seen through glass.
- **Dispersion**, from an index of refraction that varies with wavelength.

## The order

Section 1 comes first. Sections 2 and 3 build on it and can run beside each other. Section 4
needs both, and section 5 needs section 4. Section 6 needs the film and the integrator. Section 7
follows sections 2 to 6. Sections 8 to 14 build on that core in any order, but section 11 shares
its device layer with section 10, and section 13's OptiX half needs section 11.

## Sources

- RenderMan 21 removed reyes and deprecated RSL: <https://rmanwiki.pixar.com/x/mosNB>
- RenderMan's Riley scene interface:
  <https://renderman.pixar.com/resources/rman26/classRiley.html>
- RenderMan XPU, one source for CPU and GPU: <https://graphics.pixar.com/library/RenderManXPU>
- Cycles' kernel scheduling: <https://developer.blender.org/docs/features/cycles/kernel_scheduling/>
- pbrt-v4's wavefront integrator:
  <https://pbr-book.org/4ed/Wavefront_Rendering_on_GPUs/Mapping_Path_Tracing_to_the_GPU>
- pbrt-v4's spectra:
  <https://pbr-book.org/4ed/Radiometry,_Spectra,_and_Color/Representing_Spectral_Distributions>
- pbrt-v4's Sobol samplers: <https://pbr-book.org/4ed/Sampling_and_Reconstruction/Sobol_Samplers>
- Hero wavelength sampling: <https://cgg.mff.cuni.cz/~wilkie/Website/EGSR_14.html>
- Karma XPU's MaterialX support: <https://sidefx.com/docs/houdini/solaris/karma_xpu.html>
- OpenPBR reference implementation: <https://github.com/adobe/OpenPBR-Implementation>
- Many-light sampling: <https://fpsunflower.github.io/ckulla/data/many-lights-hpg2018.pdf>
- HIP RT: <https://gpuopen.com/download/HIPRT-paper.pdf>
- Avoiding self-intersection:
  <https://research.nvidia.com/publication/2019-03_fast-and-robust-method-avoiding-self-intersection>
- Path guiding in production:
  <https://disneyanimation.com/publications/path-guiding-in-production-and-recent-advancements/>
- OSL light path expressions:
  <https://github.com/imageworks/OpenShadingLanguage/wiki/OSL-Light-Path-Expressions>
- OIDN: <https://openimagedenoise.github.io/documentation.html>
