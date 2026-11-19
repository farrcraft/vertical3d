# Materials

This page is for anyone changing how moya shades a surface by its material. It covers
how the gather reads a material and compiles it into a node program, the BSDF library the program
feeds, and how OpenPBR's layers are arranged in it. The host side is in `moya/libmoya/material/`,
in the namespace `v3d::moya::material`. The kernel side is in `moya/libmoya/kernel/program/` and
`moya/libmoya/kernel/bsdf/`. [Surfaces.md](Surfaces.md) covers the shading point a material is
evaluated at, [Patterns.md](Patterns.md) the noises a program runs, and
[Textures.md](Textures.md) the images it reads.

## The gathered material

The gather reads the material each instance names, once per material, and reads it again on every
update, so a changed material takes effect as the hierarchy does. A material whose surface node is
of language `materialx` is compiled, as [Compiling a material](#compiling-a-material) says, and
one of language `ri` too, as [RI's built-in surfaces](#ris-built-in-surfaces) says. Any other
material is diffuse: a `kernel::surface::Material` holds the colour its surface node's `Cs`
input gives, converted into ACEScg from the space the value names, and whether it gives one. It
also holds the closures its BSDF is built from, at most `kMostMaterialClosures`. Each is a model
and a scale, and a conductor's also holds its roughness, its anisotropy and its F82 tint. A
layered material holds OpenPBR's parameters in a `surface::OpenPbr` in place of its closures, as
[Layering](#layering) says. The gather gives every material one diffuse closure of scale one.
Code can set others on the gathered prototypes, and `integrate::render()` renders prototypes it
is given, as [Integrator.md](Integrator.md#rendering-a-scene) says. A shading point names the
material of the innermost instance of its chain that names one, so an instance in a group with no
material takes the material of an instance that places the group.

### Compiling a material

`material::compile()` reads a material's network into a MaterialX document, through the standard
libraries `material::Library::installed()` reads once from beside the executable.

- **The network is the surface node and every node that feeds it** through the scene's
  connections. Each node becomes the MaterialX node of the definition its `shader` names, such as
  `ND_open_pbr_surface_surfaceshader`, with the inputs its primvars give.
- **What cannot be read is left out and reported.** That is a node of another language, or of a
  definition the libraries do not hold. It is an input the definition does not take, or a value
  not of its type. It is a connection between names or types that do not match. An input fed by
  a node left out keeps its own value.
- **Every node a node graph implements is flattened** into the nodes of its graph, as MaterialX's
  own flattening does. Shader, closure and material nodes are not flattened, because their inputs
  feed moya's BSDF library. `material::flattened()` gives the flattened document as XML.
- **A surface of Standard Surface, UsdPreviewSurface or glTF PBR is translated onto
  `open_pbr_surface`** before the network is flattened, as
  [Other surface shaders](#other-surface-shaders) says.
- **A surface of `open_pbr_surface` makes the material layered**, and its program gives OpenPBR's
  parameters, as [The node program](#the-node-program) says. `geometry_thin_walled` sets
  `thinWalled`, and is read as a constant.
- **A layered material is cut out** when its opacity may be less than one. That is when its
  opacity varies over the surface and the program gives it, which sets `programmedOpacity`, or
  when the constant it holds is less than one. `Material::cutOut()` says which. A traversal asks
  about a hit on such a material, as [Integrator.md](Integrator.md#surfaces-that-are-cut-out) says.
- **A surface of any other definition is reported**, and the material is layered with OpenPBR's
  defaults.
- **Every input of `open_pbr_surface` but `geometry_thin_walled` reaches the program**, and
  `kOpenPbrInputs` lists them. A dispersion whose scale may be other than zero is reported, since
  nothing renders one, and every wavelength refracts alike.
- **The material holds its parameters as `material::fold()` folds its program.** A register is
  constant when it starts from a value and every instruction writing it reads only constant
  registers. An instruction that reads the shading point or an image varies. The constant
  registers are run on the host, so a parameter a chain of constant nodes feeds holds its value,
  and only a parameter that varies over the surface needs the program. The gather folds each
  program again once its colours are converted.

### Other surface shaders

Standard Surface, UsdPreviewSurface and glTF PBR render as OpenPBR Surface. `internal::translated()`
replaces a surface node of one of them by a translation node and an `open_pbr_surface` node
that each of the translation's outputs feeds. The translation of a shader is the node definition
`ND_<shader>_to_open_pbr_surface`, whose outputs are OpenPBR's inputs, each with `_out` after its
name, and a node graph implements it.

- **MaterialX ships Standard Surface's translation**, and moya's own translations of
  UsdPreviewSurface and glTF PBR are read with the libraries, as `material::Library` says.
- **The translation node takes every input of the surface's definition**, with the value or the
  connection the surface gives it, or its definition's default. An input the translation does not
  take, such as an occlusion, is left out.
- **A normal, a tangent and a coat normal in world space pass to OpenPBR's directly** when a node
  feeds them, which are Standard Surface's and glTF PBR's. Standard Surface's `thin_walled`
  passes as it is.
- **UsdPreviewSurface maps by name in its metallic workflow.** In its specular workflow, the
  largest component m of `specularColor` sets the index whose reflectance at normal incidence is
  m, and the colour over m tints it, so the reflectance at normal incidence is `specularColor`.
  An `opacityThreshold` above zero cuts the surface out where the opacity falls below it, in
  either opacity mode. Its tangent-space `normal` is read as a normal map. It has no
  transmission, and its clearcoat's index is a fixed 1.5.
- **glTF PBR maps by name.** A sheen of any colour is a fuzz of weight one in that colour. The
  iridescence's thickness is in nanometres and the thin film's in micrometres. An `alpha_mode`
  of MASK cuts the surface out by `alpha_cutoff`, BLEND takes `alpha` as the opacity, and
  OPAQUE is opaque. Its volume, occlusion and dispersion have no counterpart, and its
  clearcoat's index is a fixed 1.5.
- **A translation missing from the libraries is reported**, and the surface is left as it was.
- **Without the libraries**, a MaterialX material is diffuse, and the reason is reported.

Each report is given once, with the gather's others.

### RI's built-in surfaces

A surface node of language `ri` compiles as a MaterialX node of moya's definition of its shader,
`ND_ri_<shader>_surfaceshader`, which is translated onto OpenPBR as the shaders above are. moya
defines RI's seven built-in surfaces, `constant`, `matte`, `metal`, `plastic`, `paintedplastic`,
`shinymetal` and `glass`, with RI's parameters and defaults, `Cs` and `Os`. Its translations are
read with the libraries, beside UsdPreviewSurface's and glTF PBR's. RI's lobes add without
conserving energy and OpenPBR's layers conserve it, so each translation is near the shader, not
equal to it.

- **`defaultsurface`, RI's default surface, is `matte`.**
- **A shader's document on the shader search path defines it first.** The document is
  `<shader>.mtlx` in the first directory of the options' `shaderSearchPath` that holds one. It
  holds `ND_ri_<shader>_surfaceshader` and its translation, `ND_ri_<shader>_to_open_pbr_surface`,
  with a node graph that implements the translation, as moya's own built-ins are written. Its
  definitions are read before the libraries', so one of a built-in's name takes its place. An
  empty search path names no document, and a document MaterialX cannot read is reported.
- **A shader neither defines shades as `matte`**, and is reported once by name. Its own
  parameters are not reported.
- **`Kd` weighs the base and `Ks` the specular layer.** `Ka` is taken and not used, because the
  integrator gathers every light.
- **The colour is the geometry's `Cs` primvar**, or the node's `Cs` where the geometry has none, as
  RI's is. The opacity is the mean of the node's `Os`, since OpenPBR's opacity is one number. It is
  the node's alone, so a surface whose opacity is one is never cut out.
- **A roughness is RI's Phong exponent of one over it.** Walter and others' equivalence makes an
  exponent n a Beckmann alpha of `sqrt(2 / (n + 2))`, and OpenPBR's roughness is the square root
  of that alpha. RI's default of 0.1 is a roughness of about 0.64.
- **`matte`** is a base of weight `Kd` in the colour, with no specular layer.
- **`plastic`** adds a specular layer of weight `Ks` in `specularcolor`, at OpenPBR's index of
  1.5. **`paintedplastic`** multiplies the colour by its `texturename`'s image at s and one less
  t, since RI's t runs down an image and MaterialX's v up it. A texture of no name multiplies by
  one.
- **`metal`** is a conductor of the colour, of weight `Ks`.
- **`shinymetal`** adds a mirror of weight `Kr`, which OpenPBR has no second lobe for. Its weight
  is `Ks` and `Kr` together, at most one, and its roughness is the glossy lobe's times `Ks` over
  `Ks` and `Kr` together.
- **`glass`** reflects and refracts sharply, as its traced rays do. Its specular layer has the
  weight `Kr`, its index is `eta`, and it transmits `Kt` in the colour. It is opaque, as the shader
  makes it, and its `roughness` and `Ks`, which shaped a highlight, are not used.
- **`constant`** emits its node's `Cs` at a luminance of one and reflects nothing. Its colour is
  the node's alone, because a material's emission has to be the same at every point.
- **A displacement of language `ri` is the node of `ND_ri_<shader>_displacementshader`**, read
  from `<shader>.mtlx` on the shader search path before moya's own, as a surface's definition
  is. moya defines no RI displacement of its own. A name no document defines is reported once,
  and the material does not displace.

### Compiling a displacement

`material::compileDisplacement()` compiles a material's `displacement` terminal whose node is
a MaterialX node or an RI displacement, as `compile()` compiles its surface. Its network is
flattened alike, and its reports follow the same rules.

- **An RI displacement's graph is flattened into the network**, though the node is a shader,
  so the displacement node its graph ends in is the terminal.

- **A displacement of `ND_displacement_float` or `ND_displacement_vector3`** gives a program
  whose values begin with `kDisplacementRegisters`, laid out as `kernel/program/Displacement.h`
  says. A float is a displacement along the normal, and a vector3 one along the tangent, the
  bitangent and the normal, as MaterialX gives it. `program::displacement()` gives either times
  its scale.
- **A displacement of any other definition is reported**, and does not displace.
- **The gather compiles each such displacement** with the materials' programs, numbering the
  primvars it reads with theirs, and runs it as [Geometry.md](Geometry.md#tessellation-and-displacement)
  says.

### Compiling a volume

`material::compileVolume()` compiles a material's `volume` terminal, the medium inside what the
material is given to, as `compile()` compiles its surface. Its network is flattened alike, and its
reports follow the same rules.

- **A volume of `ND_volume` is rewritten into a medium's values**, by
  `internal::rewrittenMedium()`. Its VDF and its EDF are trees of closures, which no program
  runs, so each tree becomes ordinary nodes that feed a node of a medium's values, and the
  program compiles those.
- **The absorption is every VDF leaf's absorption**, an `absorption_vdf's` or an
  `anisotropic_vdf's`, summed by each `add` and weighed by each `mix` above it. A `mix`
  weighs its `fg` by its `mix` and its `bg` by one less it.
- **Each `anisotropic_vdf` scatters as a lobe of its own**, by its scattering, weighed the same
  way, and by its own anisotropy. The first in the tree's order, `in1` before `in2` and `fg`
  before `bg`, is the first lobe, and the second the second. A third and every later one adds its
  scattering to the second lobe, and is reported. `kernel::volume::kMostLobes` is two.
- **The emission is every `uniform_edf's` colour**, summed and weighed alike. Its colours are
  converted into ACEScg as every colour is. A coefficient is a `vector3`, and is not converted.
- **A closure of any other kind is reported**, and adds nothing.
- **The program's values begin with `kMediumRegisters`**, laid out as
  `kernel/program/Medium.h` says, and `program::mediumOf()` reads them. The compiled medium holds
  what the program gives where its values do not vary.
- **A volume of any other definition is reported**, and holds no medium.

A medium, `kernel::volume::Medium`, holds an absorption, each lobe's scattering and anisotropy,
and an emission, each a coefficient per unit of length in each ACEScg channel. Its extinction is
its absorption and every lobe's scattering together.

### The medium inside a glass

`kernel::volume::interior()` gives the medium inside an OpenPBR surface that is not thin-walled,
from its transmission, as OpenPBR gives it.

- **The extinction is the transmission colour's negative logarithm over the depth**, so white
  light that travels `transmission_depth` inside takes the colour. A colour is held to at least
  `kLeastTransmittance`.
- **The scattering is `transmission_scatter` over the depth**, as one lobe of
  `transmission_scatter_anisotropy`.
- **The absorption is the extinction the scattering leaves.** Where that falls below zero in a
  channel, every channel's absorption is raised by the same amount, so the least is zero.
- **A depth of zero, or a thin wall, holds no medium.**

Each material the scene gathers holds the medium it encloses, as `surface::Material::medium`: its
volume's, compiled from its volume terminal, when it has one, and otherwise the one `interior()`
gives a layered material. A volume that does not compile is reported, and the material encloses
no medium. [Integrator.md](Integrator.md#tracking-through-a-medium) says how a path is tracked
through it.

### The node program

A layered material's program is a list of `kernel::program::Instruction` over a store of
`kRegisters` floats, which a shading point holds while the program runs. Its `program::Program`
names its first instruction and their count, and its first value and how many registers it uses.
The gather appends each material's instructions and values after the one before.

- **The registers start from the program's values.** The first `kParameterRegisters` hold
  OpenPBR's parameters, at the offsets `kernel/program/Parameters.h` names. Each constant and each
  node's output takes registers of its own after them. A value of n components takes n registers.
- **Each input of the surface that is not connected is a constant** in its parameter's
  registers: the value its node holds, or the definition's default. A geometry normal or tangent
  given no value is zero, the shading point's own, as [A material's normals](Surfaces.md#a-materials-normals)
  says.
- **Each input that a node feeds is compiled from that node**, and from every node it reads,
  each once. The node's instruction writes its output's registers, and a copy moves them into the
  parameter's.
- **An instruction is one MaterialX node**: an `Op`, its output's registers, and up to
  `kMostOperands` operands, each a first register and its components. Integers and booleans are
  held as floats, a boolean as one or zero. Most operations are taken one component at a time,
  and an operand of one component is spread over every component.
- **The program compiles MaterialX's geometric nodes, and its math, adjustment, compositing,
  conditional, logical and channel nodes** on floats, integers, booleans, vectors and colours,
  each as MaterialX defines it. That is add, subtract, multiply, divide, modulo, fract, invert,
  absval, floor, ceil, round, power, the trigonometric functions and atan2, sqrt, ln, exp, sign,
  clamp, min, max, normalize, magnitude, dotproduct and crossproduct. It is remap, smoothstep,
  luminance, rgbtohsv and hsvtorgb. It is mix, plus, minus, difference, burn, dodge, screen,
  inside and outside, and on colours with alpha it is premult, unpremult, over, in, mask, matte,
  out and disjointover, each as MaterialX's OSL implementations compute it. unpremult divides by
  the alpha whatever it is. It is ifgreater, ifgreatereq, ifequal, and, or and not, and it is
  constant, dot, convert, extract and combine.
- **blackbody gives a Planckian radiator's colour at its temperature**, at a luminance of one, as
  MaterialX's OSL implementation gives it. Planck's law is summed through the observer every
  `kBlackbodyStep` nanometres, from a temperature held to 800 to 25000 kelvin as MaterialX holds
  it. The colour is in ACEScg with no chromatic adaptation, as a light's colour temperature is,
  and a component below zero is zero.
- **The geometric nodes read the shading point.** geompropvalue reads the primvar it names, of up
  to four floats, and gives its `default` where the geometry has none. texcoord reads `st`, or
  `st1` and on for a later index, and geomcolor reads `Cs`, each giving zero where there is none.
  position, normal, tangent and bitangent give the point's, in the world where their `space` is
  `world` and in the geometry's own space otherwise. A primvar's number is its instruction's
  first operand, and a space's is `kObject` or `kWorld`.
- **Matrices are held row after row**, nine registers for a matrix33 and sixteen for a
  matrix44, as MaterialX writes them. A vector is transformed as a row on the matrix's left, as
  MaterialX's own matrices transform it, so a translation is the last row. multiply and divide of
  two matrices are the matrix product, and the product with the second's inverse. The program
  also compiles transpose, determinant, invertmatrix, creatematrix, transformmatrix and extract,
  which gives a matrix's row, and the componentwise nodes on matrices.
- **transformpoint, transformvector and transformnormal** carry a vector between the geometry's
  own space and the world, by the shading point's transformation. A normal moves by the inverse
  transpose and is of unit length. `object`, `model` and an empty space are the geometry's own.
- **The procedural nodes** are the noises, ramps, splits and rotations, each giving what
  MaterialX's implementations give. [Patterns.md](Patterns.md) states their algorithms. An input
  given no value reads the geometry its definition names, such as `st` for `UV0`.
- **The image node reads a texture** through the device's texture access, as
  [Textures.md](Textures.md#looking-up-a-texture) says, and tiledimage flattens to it. Its first
  operand is its texture coordinate and its second its default. Its third is the image's number,
  its fourth u's address mode and four times v's, and its fifth how it filters, each a number. A
  node with no file reads nothing, and gives its default.
- **normalmap gives a normal from a value in tangent space**, as MaterialX's OSL implementation
  does. The value is taken from zero to one onto minus one to one. Its first two components are
  scaled along the tangent and the bitangent, which are first laid flat against the normal and
  each other. A value of no length, or one that gives no direction, gives the normal.
- **heighttonormal gives a value in tangent space from a height's differences**, as MaterialX's
  implementations give it from a height's screen-space derivatives. Its height and texture
  coordinates are compiled three times, as Cycles' SVM compiles a bump's. Two follow a `Shift`
  along each axis of the footprint, and the third a `Shift` back to the point, which every
  instruction after it reads. Each difference is a step's height less the point's, times its
  scale and a sixteenth. The normal is the cross product of the two steps in `s`, `t` and height,
  turned to face up, or flat where they give no direction. bump flattens to heighttonormal and
  normalmap.
- **A heighttonormal that feeds another's height or texture coordinates gives its default**, and
  is reported, since a height's differences are taken once. The nodes the height reads are
  compiled again for it, apart from the same nodes read anywhere else.
- **Any other node gives its output's default**, and is reported. An output with no default
  gives zero, and so does a node that reads its own output.
- **An emission input that varies over the surface gives that default too**, because a
  material's light is made on the host before rendering. It is reported. Emission that a chain of
  constant nodes feeds, as a translation's does, holds its value.
- **A program that needs more than `kRegisters` registers is reported**, and every input a node
  feeds then gives the default of that node's output.
- **Each colour constant is converted into ACEScg** from the space its primvar names, or the space
  its document names for it, when the surfaces are made. `geometry::ConvertedMaterials` converts
  them, and the material's lights read the same conversion.
- **A program of no instructions is not run.** Its material's `surface` holds its parameters.
  Otherwise `bsdf::atSurface()` runs it through `program::evaluate()` before the BSDF is built,
  with the texture access the stage was given.

## The BSDF

A `kernel::bsdf::Bsdf` is how a shading point scatters light. Its directions are in world space and
point away from the surface: outgoing towards where light goes, and incoming towards where it
comes from.

- **It is a list of weighted closures**, at most `kMostClosures`. Each closure is one lobe: a
  `Model`, a weight at the path's wavelengths, and an estimate of its albedo, the mean of its
  weight. A closure of no albedo is left out. A full BSDF replaces its closure of the smallest
  albedo with a larger one, and keeps its closures when the new one is smaller.
- **It evaluates, gives a density and samples**, each at a path's four wavelengths. A sample
  holds the incoming direction, the value, the projected value, the density it was drawn with,
  and its `Lobe`: diffuse, glossy, specular or transmission. A density of zero is no sample.
- **Its value is the sum of its closures' values**, each times its weight.
- **Its projected value** is the sum of each closure's value times the cosine of the incoming
  direction to the normal of that closure's frame. It is what the integrator multiplies light by,
  so each closure takes the cosine to its own normal.
- **A sample picks one closure by its share of the albedo** and draws a direction from it. The
  first random number picks the closure, and is then stretched over the closure's share so the
  closure draws from it again. A BSDF of one closure draws from the same numbers as that closure
  alone. A third random number picks between a dielectric's reflection and its transmission.
- **A sample is weighed by the closure that drew it alone.** Its value and projected value are
  that closure's, and its density is the closure's own density times the chance it was picked.
  Each closure's light is then its own, whichever closures share a direction, so light split by
  lobe adds up to the whole. `sample.closure` names the closure.
- **The density of a direction** is each closure's density times the chance it is picked, summed.
  That is how a BSDF spreads its samples' directions.
- **One closure's part** for two directions, `Bsdf::part()`, is its projected value, its density
  times its chance of being picked, and the kind of bounce it makes. A light sample is gathered
  closure by closure from the parts.
- **A closure that scatters into a single direction is singular**, or into one of two for a
  dielectric. It has no value and no density for any direction it is given, so a light sample
  gathers nothing through it. Its sample is marked `specular`: its value and its projected
  value are the share of light the closure carries along the direction, times its weight, with no
  cosine. Its density is the chance the closure was picked, times the chance of that direction
  among the closure's.
- **Kernel code has no virtual call**, so each closure switches over its `Model`. Each model, and
  the GGX distribution and Fresnel factors they share, is in `kernel/bsdf/scattering/`, in the
  namespace `bsdf::scattering`.
- **Each closure works in a frame around a normal.** A coated closure, the coat, works in the
  BSDF's `coatFrame`, and every other closure in its `frame`. A sample is drawn and weighed in the
  picked closure's frame.
- **The geometric surface and a closure's frame have to agree** on whether two directions lie
  on one side or cross the surface. Where they disagree, the closure gives the pair no value, no
  density and no sample, so a reflection never crosses the geometric surface and a transmission
  always does.
- **A sample that crosses the surface is a transmission.** One that stays on its side is
  specular from a singular closure, glossy from a microfacet lobe and diffuse from a diffuse one.

The library's models:

- **`Diffuse` is Lambert's**: its weight over pi, on whichever side light leaves towards. It
  samples by the cosine, through Shirley and Chiu's concentric mapping of the square onto the
  disk. Its bounces are diffuse.
- **`Mirror` is perfect specular reflection** about the shading normal, on either side. It is
  singular, and its bounces are specular.
- **`Conductor` is a metal's reflection**, on either side, and its bounces are glossy. It is
  singular, and its bounces specular, while both its alphas are below `kSmoothest`, 0.001.
  The parts are set out below.
- **`Dielectric` is an interface between two dielectrics**, or a thin slab of one. It reflects on
  the side light leaves towards and transmits to the other. It is singular while both its alphas
  are below `kSmoothest`, and while its index lies within `kLeastBend`, 0.001, of one, where
  light passes straight through. It is set out after the conductor.
- **`OrenNayar` is OpenPBR's energy-preserving Oren-Nayar reflection**, on either side, and its
  bounces are diffuse. It samples by the cosine. It is set out after the dielectric.
- **`Specular` is a dielectric's reflection alone**, as a layer over a base reflects, on either
  side. Its bounces are glossy, or specular while both its alphas are below `kSmoothest`. It is
  GGX's reflection by Fresnel's equations at the closure's index, tinted by its colour. What a
  single scattering loses is given back as a conductor's is, its Fresnel factor's mean being the
  mean a smooth interface reflects. It samples as a conductor does. Its estimated albedo is its
  weight's mean times its colour's, times the mean a smooth interface reflects.
- **`Fuzz` is OpenPBR's fuzz**, Zeltner, Burley and Chiang's sheen of a layer of fibres, on
  either side, and its bounces are diffuse. It is the one lobe that is not reciprocal. It is set
  out after the Oren-Nayar lobe.

A conductor's or a specular reflection's Fresnel factor can carry a thin film, mixed in by the
film's weight. The film is set out after fuzz. The BSDF holds the path's wavelengths, which the
film reads. Neither compensation takes the film.

A conductor is made of four parts:

- **GGX's distribution of microfacet normals**, by the closure's alphas along the tangent and
  the bitangent. Masking and shadowing are Smith's, height-correlated.
- **OpenPBR's roughness and anisotropy** give the alphas, through `Ggx::alphas()`. The alpha is
  the roughness squared, and an anisotropy stretches it along the tangent and shrinks it along
  the bitangent, keeping its area. No alpha falls below `kLeast`, 0.0001.
- **Its Fresnel factor is OpenPBR's F82 tint**, an `F82`: Schlick's from the reflectance at
  normal incidence, less Hoffman's term that brings it to the tint times Schlick's at a cosine of
  one seventh. Each wavelength is its own.
- **Kulla and Conty's compensation** gives back what a single scattering loses. It is a lobe of
  one minus each direction's albedo, over pi times one minus the mean albedo, coloured by the
  Fresnel factor's mean over the bounces light makes between microfacets. It reads the albedo
  table at the geometric mean of the two alphas, so an anisotropic conductor's compensation is an
  isotropic one's. Both parts are reciprocal.

A conductor samples a visible normal by Heitz's method with the chance of outgoing's albedo, and
reflects about it. Otherwise it samples by the cosine, as the compensation is shaped. Its
estimated albedo is its weight's mean times its Fresnel factor's.

A dielectric's closure holds the ratio of the index of refraction inside, the side the shading
normal points away from, to the index of the medium outside. The BSDF holds the outside's index,
which is the medium a path is in beyond the surface, as
[Integrator.md](Integrator.md#nested-media) says. A ratio below one turns the closure's frame
over and takes the ratio's reciprocal, so the closure always holds a ratio of at least one. eta is
the ratio of the index on incoming's side to the index on outgoing's.

- **A smooth interface** reflects along the mirror direction by Fresnel's equations for
  unpolarised light, and refracts the rest by Snell's law. Beyond the critical angle it reflects
  everything. A sample takes the reflection with the chance of the Fresnel factor.
- **A rough interface** is GGX's, by the closure's alphas, as a conductor's is. Each microfacet
  reflects by Fresnel's equations at the angle between outgoing and its normal, and refracts the
  rest, as Walter and others give it. A microfacet that faces either direction the wrong way
  scatters nothing.
- **Its colour tints its reflection**, and its transmitted colour its transmission.
- **A transmission's value is the radiance it carries**: the energy it carries, divided by eta
  squared. Its value over the square of the index on outgoing's side is reciprocal.
- **Kulla and Conty's compensation** gives back what a single scattering loses, on each side. It is
  a lobe of one minus outgoing's albedo, times one minus incoming's albedo on incoming's side, over
  pi times one minus that side's mean albedo. From outside, it transmits the share a smooth
  interface transmits, averaged over the hemisphere. From inside, it transmits the share that
  keeps the two sides reciprocal: outside's share times outside's mean loss equals eta squared
  times inside's share times inside's mean loss. Where inside's share would exceed one, it is one
  and outside's follows. The albedos are read at the geometric mean of the two alphas.
- **A thin slab** has the same medium on both sides, so light passes through it unbent. It reflects
  by the slab's Fresnel factor, which counts the light that bounces back and forth inside it, and
  transmits the rest. A rough slab is GGX's reflection by that factor, with its transmission the
  reflection's mirror image through the surface. Its compensation is a conductor's of a white
  Fresnel factor, the slab's mean reflectance of it reflected and the rest transmitted.

A dielectric samples a visible normal with the chance of outgoing's albedo, then reflects about it
with the chance of its Fresnel factor, or refracts through it. A reflection that falls below the
surface, or a refraction that comes out above it, is no sample. Otherwise it samples by the
cosine, on the side the compensation's share picks. Its estimated albedo is its weight's mean
times the larger mean of its two colours.

An Oren-Nayar lobe is Portsmouth, Kutz and Hill's, its closure's colour its reflectance and its
roughness from zero, Lambert's, to one.

- **A single scattering** is Fujii's improved Oren-Nayar: the reflectance over pi, times A times
  one plus the roughness times s over t. A is one over one plus the roughness times one half less
  two over three pi. s is the dot product of the two directions less the product of their
  cosines. t is the larger cosine where s is above zero, and one otherwise.
- **Its albedo** at a cosine has a closed form, and its mean is A times one plus the roughness
  times two thirds less 28 over 15 pi.
- **What it loses** is given back by a lobe of one minus each direction's albedo, over pi times
  one minus the mean albedo. It is coloured by the reflectance of the bounces between facets: the
  colour squared times the mean albedo, over one less the colour times one less the mean. A white
  lobe keeps all its energy, and both parts are reciprocal.

Fuzz's closure reads its colour and its roughness, held to [0.01, 1], where its fits hold.

- **Its value times the cosine** is its albedo at outgoing's cosine times a distribution of
  incoming that integrates to one. The distribution is a clamped cosine, transformed linearly by
  a matrix fitted to outgoing's cosine and the roughness, in a frame whose x lies along
  outgoing's projection onto the surface.
- **Its albedo** is a fitted closed form, held to [0, 1], so a sample's mean weight is it. The fits
  are the authors', as MaterialX's library gives them.
- **A sample** draws from the distribution exactly, so its density is the distribution.
- **It is not reciprocal**, since the distribution is fitted to outgoing alone. Its checks are
  chi-squared and its albedo.
- **Its estimated albedo** is its weight's mean times its colour's, times its albedo at a cosine
  of one half.

Background: [ADR-0228](../adr/offline/0228-offline-fuzz-is-openpbrs-sheen-not-reciprocal.md)

A `scattering::ThinFilm` is a film over a surface, in nanometres thick, of an index relative to
the medium over it. Its reflectance at each of the path's wavelengths is Airy's sum over the light
bouncing inside it. Each polarisation is summed from the amplitudes each interface reflects, and
the two are averaged.

- **The phase** between successive bounces is four pi times the film's index, its thickness and the
  cosine inside it, over the wavelength.
- **Over a dielectric**, each interface reflects the amplitudes Fresnel's equations give.
- **Over a conductor**, the conductor reflects the square root of its Fresnel factor at the cosine
  inside the film, for both polarisations. The amplitude turns the phase back where the index its
  reflectance at normal incidence implies lies below the film's.
- **Light that cannot enter the film** is all reflected, and a film of no thickness is no film.
- **An RGB build** has no wavelengths, so it evaluates the film at 630, 532 and 465 nanometres for
  ACEScg's red, green and blue.

**The albedo tables are integrated when moya is built.** `moya_albedo`, in `moya/tools/albedo/`,
integrates them at 32 cosines and 32 roughnesses, the roughness being the square root of the
alpha, by the kernel's own GGX and Fresnel.

- **A reflection's table** holds a GGX reflection with a Fresnel factor of one. Each point is the
  mean weight of reflections drawn through visible normals, on a 128 by 128 grid of the random
  numbers.
- **A dielectric's tables** hold its reflection and transmission together, from outside and from
  inside, at 16 points of the ior axis, (eta - 1) / (eta + 1) from zero to 0.6, an index of four.
  Each point weighs each visible normal's reflection by its Fresnel factor and its refraction by
  the rest, on a 64 by 64 grid, and counts the transmission by the energy it carries.
- **A dielectric's reflection alone** is held from outside on the same grid. A smooth
  interface's is Fresnel's factor at the cosine.
- **At each point of the ior axis**, the mean share a smooth interface transmits from outside and
  the mean share a thin slab reflects are integrated over the cosine.
- **The mean albedo** at each roughness is twice the integral of the cosine times a table. The
  table is read between the cosines as the kernel reads it, so the compensation returns all of
  what the table says is lost.

The tool writes a C++ source, which libmoya compiles, and `material::ggxAlbedos()` returns a
`bsdf::Albedos` view of it. The source is generated, so clang-tidy and `/analyze` skip it. Tables
of no points read an albedo of one, which compensates nothing.

`bsdf::atSurface(surfaces, point, table, albedos, wavelengths, outgoing)` gives the BSDF at a
shading point: the closures its material names, or one diffuse closure for a surface with no
material. Its colour is the geometry's `Cs` primvar, which overrides its material's as RI's does, or
its material's colour, or a mid grey. The colour becomes a reflectance through the RGB-to-spectrum
table, as [Spectra.md](Spectra.md#rgb-to-spectrum) says. A diffuse closure's or a mirror's weight is
that reflectance times its scale. An Oren-Nayar lobe's weight is its scale, its colour that
reflectance and its roughness its material closure's. A conductor's weight is its scale, and that
reflectance is its reflectance at normal incidence. Its roughness, anisotropy and F82 tint are its
material closure's, the tint becoming a reflectance through the same table. A dielectric's weight is
its scale, and its roughness, anisotropy, index and whether it is thin-walled are its material
closure's. A layered material's closures are OpenPBR's for light leaving along outgoing, and the
`Cs` primvar overrides its base colour.

Background: [ADR-0222](../adr/offline/0222-offline-a-bsdf-is-a-list-of-weighted-closures.md),
[ADR-0235](../adr/offline/0235-offline-a-bounce-samples-one-closure-alone.md)

## Layering

`bsdf::Layering::add()` adds the closures of a `surface::OpenPbr` to a BSDF, top down, for light
leaving at a cosine to the BSDF's frame's normal and at another to its coat frame's. Each
parameter has OpenPBR's name and default, and its colours are in ACEScg. Each layer scales what
lies below it by one less its albedo at its cosine, as a white one reflects it, so a white
surface keeps its energy. The whole is not reciprocal, though each lobe but fuzz is.

- **The fuzz**, of its weight, tinted by its colour, lies over everything. What lies below takes
  one less its weight times its albedo, untinted.
- **The coat**, of its weight, is a `Specular` reflection of its own roughness, anisotropy and
  index, in the coat's frame. What lies below takes one less the coat's weight, plus its weight
  times one less its albedo, times the coat's colour and its darkening. Its albedo and its
  darkening are taken at the coat's cosine, and every other layer's at the frame's.
- **The coat roughens the layers below it.** Their roughness becomes the fourth root of its fourth
  power plus twice the coat's, at most one, mixed in by the coat's weight.
- **The coat darkens the base** as OpenPBR gives it: one less K, over one less the base's albedo
  times K, mixed in by `coatDarkening`. K is the share of light the coat reflects back down. For a
  smooth base it is Fresnel's factor inside the coat, at the angle light refracts to. For a rough
  base it is one less the mean a smooth interface transmits over the coat's index squared. The two
  are mixed by the base's roughness, which is one for the diffuse base. The base's albedo is its
  diffuse colour, or its metal's colour mixed in by the metalness.
- **The metal**, of the base's metalness, is a `Conductor` of the base's colour times its weight,
  with the specular colour as its F82 tint, scaled by the specular weight.
- **The rest is a dielectric.** Its index is the specular index with its reflectance at normal
  incidence scaled by the specular weight, at most 0.9999. Of the transmission's weight, it is a
  `Dielectric` that transmits, thin-walled where the surface is. Its reflection is tinted by the
  specular colour and its transmission by the transmission colour. A surface that is not
  thin-walled and has a `transmission_depth` is not tinted by the transmission colour, which the
  medium inside takes over that depth, as [the medium inside a glass](#the-medium-inside-a-glass)
  says.
- **The opaque rest** is a `Specular` reflection over the diffuse base, tinted by the specular
  colour. Its index is relative to the coat's, mixed in by the coat's weight, and its reciprocal
  where that falls below one. The diffuse base is an `OrenNayar` lobe of `baseDiffuseRoughness`.
  Its colour is the base colour times its weight, mixed with the subsurface colour by the
  subsurface weight, as subsurface scatters as the diffuse base. It takes one less the specular
  layer's albedo, which is the table's reflection alone plus what the compensation returns.
- **A thin film**, of its weight, lies over the metal and the opaque specular layer. Its thickness
  is OpenPBR's in micrometres, and its index is relative to the coat's as the specular layer's is.
  The layer below the specular layer takes one less its albedo times the film's reflectance over
  the bare Fresnel factor, at outgoing's cosine and each wavelength.

Specular anisotropy follows its frame's tangent, and the coat's the coat frame's, with no
rotation. A layered material's emission is no closure: it makes the material's geometry a light
of the light tree, as [Lights.md](Lights.md#mesh-lights) says.
