# RIB

How moya reads a RenderMan scene file. The terms used here are defined in
[README.md](README.md#glossary).

## RIB

The RIB reader is in `api/rib`. It calls `rib::Handler`, a C++ interface with typed parameter
lists. A request that takes several values of one type passes them as one struct named after the
request, so a call names each value: `handler->clipping({.hither = 0.1F, .yon = 100.0F})`. Each
struct has its own header in `api/rib/requests/`, in the namespace `rib::requests`. moya
implements the handler as `moya::ri::SceneClient`. The editor exports its scenes to RIB, one
way; see [editor/](../editor/README.md).

### What is read

The user manual's [Rib.md](../manual/moya/Rib.md) lists each request the reader recognises, what
moya does with it, and the rules a scene has to follow. This page holds the rules for changing
the reader and the handler.

### Rules

- **`rib::Reader` takes each request's name and calls the handler.** It reads a request's
  arguments through `rib::internal::Operands`, which holds the stream's declarations and the
  first error, and the geometric primitives through `rib::internal::Primitives`.
- **Only ASCII RIB is read.** A binary or gzipped stream is rejected with a message naming
  which it is. A stream is read from where it stands rather than from its beginning.
- **A request whose number cannot be used is skipped with a warning.**
- **An unrecognised request is reported once per name, and its arguments are skipped.**
  `Reader::unrecognised()` lists them. `Reader::unsupported()` lists what was read but not
  built, such as a deforming primitive.
- **Every handler method has an empty body, not a pure virtual one.** The RI standard requires
  a renderer to accept a request it does not support. A misspelled override then compiles
  silently, so every override carries `override`. A method that does not match an RI request
  does not belong on the interface.
- **A matrix crosses the RIB boundary by being read in order, not transposed.** RIB writes row
  major under a row vector convention. glm stores column major under a column vector
  convention. `glm::make_mat4` over the sixteen floats is therefore the whole conversion. This
  applies to `Transform`, `ConcatTransform`, `RtMatrix` and the editor's export alike.
- **`Rotate`'s sign is the one the unit cube in the RenderMan Interface specification's RIB
  entity file example needs** to put each face where it names it. A moya case reads that cube
  and checks each face's centre.
- **A parameter is typed by a declaration, or it is dropped.** The reader declares RI's
  standard names, including `Option "trace"`'s `maxdepth` and `Option "searchpath"`'s `shader`
  and `texture`, and nothing else.
- **A declaration's storage class is `constant`, `uniform`, `varying`, `vertex` or
  `facevarying`**, and one with none is `uniform`. A constant or uniform parameter holds one
  element. A varying or vertex one holds an element per vertex. A facevarying one holds an
  element per corner of each face, which for a mesh is the length of its indices, and for any
  other primitive is its vertex count.
- **RenderMan's `reference` may come before a declaration's type**, as in
  `"reference color base_color"`. A reference names another node's output in place of a value, so
  its values are strings whatever its type, and it is written back as `uniform reference color`.
- **A shader, light or space with an empty name names nothing.** `RIBHandler` ignores the
  request, and the state stays as it was.
- **A predefined space cannot be saved over.** The camera sets them, so the hiders read the
  camera the scene set up. `RiCoordinateSystem` with one of them is logged and saves nothing.
- **A quadric's varying and facevarying parameters hold four values**, one at each corner of its
  parameter space, as RI defines them. A uniform one holds one. The reader reads the seven
  quadrics alike.
- **`Points` carries no count.** The count is the length of `"P"`, or of `"Pz"`, as a
  `Polygon`'s is.
- **A `SubdivisionMesh`'s tags each take two or three counts**, of integers, of floats and,
  after RI 3.2, of strings. The arrays that follow must hold exactly what the counts add up to,
  or the request is an error. The strings' array follows only when each tag gives three counts,
  and the writer writes it only then.
- **`rib::namedBasis()` gives a basis name's matrix**, in RI's row order, and `rib::basisName()`
  a matrix's name, which the writer uses to write a standard basis by name.
- **A `Curves` request's varying parameters are not checked for length.** How many values one
  holds depends on the basis's step, which the reader does not track.
- **A `Polygon` carries no vertex count.** The count is the length of `"P"`, which the reader
  divides out. A parameter list is therefore parsed before the count is known, and an
  unbracketed varying, vertex or facevarying parameter ends the parse instead of being guessed at.
- **A light's or an object's handle may be a number or a string.** RIB 3.03 writes a number and
  later RIB writes a string.
- **An object holds geometry alone.** `ObjectBegin` to `ObjectEnd` marks the geometry an
  `ObjectInstance` places, with the attributes and transformation in force where it is placed.
  `RIBHandler` does not instance, so it leaves out the geometry inside an object, as RI leaves it
  out where it is declared, and logs the first object.
- **`RIBHandler` logs each request its hiders do not draw**, once per name: `Points`, the quadrics
  but `Sphere`, `SubdivisionMesh`, `ObjectBegin` and `Light`. The handler's empty bodies accept
  them, so another handler sees each one.
- **RenderMan's `Light` reaches `Handler::light()`**, with its shader, its handle and its
  parameters, as `LightSource` does. RI's shading language has no such lights, so the default
  body ignores it. The writer and the recorder carry it.
- **A `PixelFilter` name the reader does not recognise never reaches the handler.** A
  `PixelSamples` rate reaches the handler as a count of at least one. `DepthOfField` with no
  arguments is sent as an infinite fstop.
- **`Attribute "displacementbound"` is kept in camera space**, scaled by the largest stretch of
  the axes of the space it was given in.
- **`AreaLightSource` becomes an ordinary light** unless a handler overrides
  `Handler::areaLightSource`. moya overrides it.
- **A primitive repeated inside a motion block is a deformation, which moya does not build.**
  The reader hands the later poses to `Handler::deformation()`, which returns null unless a
  renderer overrides it, and lists them in `Reader::unsupported()`.
- **A polygon's `"st"` is carried by both hiders.** A traced triangle weights its corners' values
  by its barycentric coordinates. A triangle given none has `s` and `t` equal to those
  coordinates. The reyes hider interpolates `"st"` across a grid, as it does `"Cs"`, and carries
  both through a split. A grid whose corners have none uses its own parameters.

### The scene client

`moya::ri::SceneClient` fills the new core's scene from RI requests, beside `RIBHandler`, which
drives the reyes and ray hiders. [Scene.md](Scene.md) describes the scene it fills.

- **The options and the camera are made with the client**, named `options` and `camera`, so a
  scene that names neither has RI's defaults. The frame's requests set their parameters.
  `PixelSamples` sets the samples per pixel to its two counts multiplied, and `PixelVariance`
  sets the options' `noiseThreshold`. `CropWindow` sets the options' `crop`, which renders the
  window of the image RI's crop window names, as
  [Integrator.md](Integrator.md#rendering-a-scene) says.
- **`WorldBegin` makes the current transformation the camera's.** It takes world space to camera
  space, so the camera holds its inverse, and the transformation is reset for the world.
- **The current transformation holds a sample at each time of a motion block.** Inside a block,
  each request gives the transformation at the block's next time, built on the transformation
  the block opened with. Outside a block, a request applies to every sample. A block whose times
  do not rise is logged, and keeps its first time alone.
- **A display becomes an output named after its file.** A name starting with `+` adds an output,
  and any other replaces every output before it. A `.png` file is written as PNG, and any other
  as EXR, unless the type is `"framebuffer"`, or `"deepexr"` for a deep file. The mode names the
  channels the output holds beside the beauty, apart by commas, as RenderMan's do. `rgb`, `rgba`,
  `Ci` and `a` name the beauty. Any other name is a `DisplayChannel`'s, or a channel named after
  it whose source is the name.
- **RenderMan's `DisplayChannel` declares a channel**, named as its declaration is, such as
  `"varying color diffuse"`, of three components for a colour, point, vector or normal and one
  for any other type. Its `"string source"`, after the source's type, is `lpe:` and a light path
  expression, or a value at the first hit, RenderMan's `z`, `P` and `Nn` read as the depth, the
  position and the normal. A channel of no source collects the value its name names. Its
  `"string filter"` of `min`, `max`, `sum` or `zmin`, the closest, sets how its samples combine,
  and its `"int denoise"` of 1 writes it denoised beside it.
- **RenderMan's `PxrCryptomatte` sample filter adds a Cryptomatte output** of its
  `"string filename"`, in floats. Its `"string layer"` names what the IDs name: `Name` an object,
  `Material` a material, and `Path` an asset, each a layer named CryptoObject, CryptoMaterial or
  CryptoAsset. Any other sample filter is logged and changes nothing.
- **A light's `"string lightGroup"` names its light group**, on RI's `LightSource` and on
  RenderMan's `Light`.
- **A projection other than perspective or orthographic** is logged and changes nothing.
- **`WorldEnd` commits the frame**, and calls the function the client's `frame()` was given, where
  the scene's owner applies the batch. A file of several frames is a scene that changes between
  commits.
- **An object takes its name from `Attribute "identifier" "name"`**, held in the attribute state,
  or else from its request and a count, such as `Polygon 2`. A name its kind already holds gets
  a number after it. A light without a name is named after its handle, as `light 1`, and an
  object after its handle, as `object leaf`.
- **Each primitive is a geometry prototype and an instance of it**, at the current
  transformation. `Polygon` and `PointsPolygons` are meshes, and the other primitives have
  kinds of their own. The request's parameters that the kind does not hold become primvars, with
  their RIB storage class, so a `facevarying` one is a facevarying primvar. `"P"` comes from
  `"Pw"` divided out when there is no `"P"`.
- **A curve or a point cloud takes `"width"` and `"constantwidth"` as its kind's widths**, and
  neither becomes a primvar. A cubic curve takes the basis in force, and a basis RI does not name
  is logged and taken as Bezier.
- **A subdivision mesh keeps a `catmull-clark`, `loop` or `bilinear` scheme.** Another scheme
  is logged and subdivided by catmull-clark. The tags `crease`, `corner`, `hole` and
  `interpolateboundary` become the mesh's parameters, and another tag is logged. A mesh with no
  `interpolateboundary` has a boundary rule of none.
- **A primitive repeated inside a motion block deforms.** Each later pose adds a sample, at the
  block's next time, to each parameter of the first pose's prototype that may move. A pose whose
  points differ in number keeps the first pose alone, and is logged.
- **A primitive outside the world is not drawn**, and the first is logged.
- **The material is made when a geometry needs one**, from the attribute state's `Surface`,
  `Displacement`, `Color` and `Opacity`. Each shader is a node of language `ri` with its
  parameters as inputs, and the colour and opacity are the surface node's `Cs` and `Os`. The
  instances made under one attribute state share it. With no `Surface`, the surface is
  `defaultsurface`. RI's built-in surfaces compile onto OpenPBR, and any other shades as `matte`,
  as [Materials.md](Materials.md#ris-built-in-surfaces) says.
- **`Attribute "displacementbound"` bounds the subdivision surfaces after it** in its attribute
  block, as their `displacementBound`. Its `sphere` is taken into object space from the
  `coordinatesystem` it names, at the largest stretch of that space's axes, and a space no
  request named is logged and taken as object space.
- **RenderMan's `Pattern` and `Bxdf` make a material of MaterialX nodes.** Each adds a node of
  language `materialx` to the attribute state's network, of the node definition its type names,
  such as `ND_image_color3`, and the network is saved and restored with the attribute state. Each
  parameter is a constant input, but a `reference`, which connects its input to the output of the
  node whose handle its value names, written `handle:output`, or `handle` for a node's only output.
  A reference reads a node that came before it, and one that names no such node is logged and
  leaves its input as it was.
- **The last `Bxdf` is the material's surface**, and the material holds it and every node it
  reads. The later of a `Surface` and a `Bxdf` shades what follows, because a `Surface` clears
  the network. The attribute state's `Displacement` stays RI's, unless a document's material
  gives one.
- **A `Bxdf` of type `materialx` reads a material from a MaterialX document.** Its `file` names
  the document, as a path from the working directory, and its `material` the material node, or
  the document's first when it gives none. The material's `surfaceshader` and every node it
  reads, through node graphs and their interface inputs, become the scene's nodes, each of the
  definition MaterialX's libraries resolve it to. A colour keeps its document's space, and a
  relative filename is taken from the document's directory. The `Bxdf`'s other parameters then
  override the surface node's inputs, and its references connect them to the network's nodes.
  When the material's `displacementshader` names a node, that node and every node it reads
  become the material's displacement, in place of the attribute state's `Displacement`. A
  document that cannot be read, or holds no such material, is logged, and the surface is the
  attribute state's `Surface`.
- **An object is a group.** `ObjectBegin` saves the attribute state and resets the
  transformation, so the object's geometry is placed in its own space, and its instances name
  the group and no material. `ObjectInstance` places the group at the current state. An
  `ObjectInstance` of an object no `ObjectBegin` made is logged.
- **RI's three standard lights become UsdLux's.** `pointlight` is a sphere light of radius zero
  at its `from`. `distantlight` is a distant light. `spotlight` is a sphere light of radius zero
  whose cone angle is its `coneangle` in degrees, and whose softness is its `conedeltaangle` over
  its `coneangle`. Each points its `-z` from its `from` towards its `to`, and a point light,
  which has no `to`, looks down z from its `from`. Each holds its `lightcolor`. A distant
  light holds its `intensity`. A point or spot light is normalised, with four times its
  `intensity`, so that its radiant intensity is RI's, as [Lights.md](Lights.md#points) says.
  Another light shader is logged and makes no light, and `Illuminate` passes over its handle.
- **A light that lit every instance keeps `includeRoot`.** Any other sets `includeRoot` to 0 and
  names the instances made while it was on, so a light that was off for every instance lights
  none.
  `Illuminate` turns a light on and off in the attribute state, and `AttributeEnd` restores it.
- **RI's `ambientlight` is a dome light** of its `lightcolor` and `intensity`, whose shadow link
  names no instance and whose `camera` is 0. It lights every point by its albedo times the light,
  as RI's ambient term does, and the camera does not see it. Since it casts no shadow, only a
  light sample finds it, so a glossy surface under it converges slowly and shows bright single
  samples at first. RenderMan's `PxrDomeLight` casts shadows, and lights the same surface
  smoothly.
- **`AreaLightSource` makes a mesh light of each geometry after it in its attribute block**, with
  its `intensity` and `lightcolor`. The mesh light names the geometry's prototype, so the
  instance that places it emits. A shader other than `arealight` is logged once, and makes no
  light.
- **RI's `background` imager and RenderMan's `PxrBackgroundDisplayFilter` set the options'
  `background`**, to the imager's `"color background"` or the filter's `"color backgroundColor"`,
  black when neither is given. An output is composited over it when it is written, as
  [Output.md](Output.md#a-background) says. Another imager or display filter is logged once by name.
- **RenderMan's lights become UsdLux's.** `Light` makes a rect, disk, sphere, cylinder, distant
  or dome light of `PxrRectLight`, `PxrDiskLight`, `PxrSphereLight`, `PxrCylinderLight`,
  `PxrDistantLight` and `PxrDomeLight`. Each is on as a `LightSource` is, and `Illuminate` and
  `AttributeEnd` turn it off. Another shader is logged and makes no light.
- **A RenderMan light's parameters keep RenderMan's meaning under UsdLux's names.** Every light
  takes `intensity`, `exposure`, `lightColor` as its colour, `enableTemperature` as
  `enableColorTemperature` and `temperature` as `colorTemperature`. Every light but the dome
  takes `areaNormalize` as `normalize`. A rect, disk, sphere or cylinder light takes `coneAngle`,
  `coneSoftness`, `iesProfile` as `iesFile`, `iesProfileScale` as `iesAngleScale` and
  `iesProfileNormalize` as `iesNormalize`. A rect or dome light takes `lightColorMap` as its
  `texture`. A distant light takes `angleExtent` as its `angle`, and its intensity is 50000
  unless the request gives one, as RenderMan's is. A parameter the request does not give keeps
  UsdLux's fallback, which is RenderMan's default.
- **A RenderMan light's shape is UsdLux's at its fallback size**, since RenderMan sizes a light by
  its transformation alone. A rect is a unit square, and a disk, sphere and cylinder have a radius
  of 0.5. A cylinder is of length 1 along x.
- **A RenderMan light is mirrored onto UsdLux's axes.** It faces its +z, where UsdLux's faces -z,
  and lays a rect's texture from its -x and -y. Its transformation is therefore the current one
  scaled by -1 on every axis, as Pixar's hdPrman places a UsdLux light in RenderMan. A dome's pole
  is its +z, where UsdLux's is +y, so a dome is turned instead: its UsdLux +y is RenderMan's +z,
  its +z is RenderMan's -x, and its +x is RenderMan's -y.
- **`PxrMeshLight` makes a mesh light of each geometry after it in its attribute block**, as
  `AreaLightSource` does, with RenderMan's parameters. Its colour is `lightColor` times a
  constant `textureColor`. No cone or profile shapes it.
- **`CoordinateSystem` names the current transformation**, and `CoordSysTransform` makes a named
  one current. `"world"` is the identity, and `"camera"` is the camera's own space. Another name
  is logged.
- **A new world replaces the one before it.** `WorldBegin` removes every object the world before
  it made, and keeps the options, the camera and the outputs.
- **A block the world opened ends with it.** An `AttributeBegin` or `ObjectBegin` still open at
  `WorldEnd` is closed there, with a warning, so the next world's geometry names no object the
  new world removed.
- **`Option "moya"` sets moya's own options**: its `"int denoise"` of 1 writes the beauty
  denoised beside it, and its `"float deepTolerance"` sets how near two depths join in a deep
  file.
- **`Option "searchpath" "texture"` sets the options' `textureSearchPath`**, and `"shader"` their
  `shaderSearchPath`, each `&` in either standing for the path before, as RI says. A shader's
  document on the shader search path defines it, as [Materials.md](Materials.md#ris-built-in-surfaces)
  says. `Option "limits" "texturememory"` sets
  `textureMemory`, in kilobytes, and a limit below one or above what an integer holds is logged
  and changes nothing.
- **`Option "trace" "maxdepth"` sets the options' `maxDepth`**, the most bounces a path takes. A
  depth below zero or above what an integer holds is logged and changes nothing.
- **`Hider "raytrace"` and `Hider "hidden"` are passed over**, since each names a renderer's
  default and moya renders by path tracing alone. Another hider is logged once by name.
- **`ShadingRate` is passed over.** A surface is tessellated to the options' `edgeLength`.
- **`Sides` sets each later instance's `doubleSided`**: 1 for RI's default of 2, and 0 for 1, to
  the end of its attribute block. A count that is neither is logged and changes nothing. It
  decides which faces an emitting material emits from, and hides no face from the camera.
- **Some parameters are passed over by rule.** `Option "limits"`'s `bucketsize` and `gridsize`
  are. `Attribute "trace" "bias"` is too, since a
  ray leaving a surface starts off it by the core's own rule. `Attribute "light" "nsamples"` is
  too, since a path samples one light at each bounce.
- **Every other parameter of an `Option`, an `Attribute`, a `Hider` or a `Display` that the client
  does not read is logged once** by its request, its name and the parameter's name, and changes
  nothing.
- **`MakeTexture` makes the texture file at once**, as [Textures.md](Textures.md#making-a-texture)
  says, with the request's wrap modes, filter and widths and its `"string colorspace"`. A
  texture that cannot be made is logged, and the file reads on.
- **The client's jobs are classes of their own**, in `moya/libmoya/ri/internal`. `Shapes`
  turns each primitive request into a scene kind and its parameters. `Deforming` takes a
  deforming primitive's later poses. `Lights` makes the lights and records what each lit.
  `Objects` makes the world's objects and names them.
- **`moya --scene`** reads a file through the client and prints the scene's description after
  each frame, in place of rendering it.

### Writing and recording

Two handlers in `api/rib` take requests from anything that sends them, such as
the editor's export, rather than from the reader.

- **`rib::Writer` writes each request as a line of ASCII RIB**, which the reader reads back as
  the same request. A number takes the fewest digits that read back as the same float. A
  parameter is written by its name when its declaration is the one in force. Otherwise it
  carries its declaration inline, as in `"uniform float Ks"`. A deforming primitive is written
  as it was given, because RIB carries a deformation as the same primitive given again.
- **RIB has no number for an infinity or a NaN.** The writer writes an infinity as the largest
  float of its sign and a NaN as 0. `DepthOfField`'s infinite fstop is the exception, and is
  written as the request with no arguments, which is a pinhole.
- **`rib::Recorder` keeps each request with copies of its arguments**, and `replay()` passes them
  in order to another handler, as often as it is called. A deforming primitive goes to that
  handler's `deformation()`, and is dropped when that is null, as the reader drops it. A
  recording cannot be copied or moved, so one passed to another thread is held by a pointer.

Background: [ADR-0023](../adr/offline/0023-offline-rib-is-the-scene-format.md),
[ADR-0025](../adr/offline/0025-offline-rib-reader-calls-a-typed-handler-interface.md)
