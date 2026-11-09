# RIB

How moya reads a RenderMan scene file. The terms used here are defined in
[README.md](README.md#glossary).

## RIB

The RIB reader is in `api/render/offline/rib`. It calls `offline::rib::Handler`, a C++
interface with typed parameter lists, not the RI C API. A request that takes several values
of one type passes them as one struct named after the request, from `rib/Requests.h`, so a call
names each value: `handler->clipping({.hither = 0.1F, .yon = 100.0F})`. moya implements the
handler as `moya::ri::RIBHandler`. The editor exports its scenes to RIB, one way; see
[editor/](../editor/README.md).

### What is read

The user manual's [Rib.md](../manual/moya/Rib.md) lists each request the reader recognises, what
moya does with it, and the rules a scene has to follow. This page holds the rules for changing
the reader and the handler.

### Rules

- **Only ASCII RIB is read.** A binary or gzipped stream is rejected with a message naming
  which it is. A stream is read from where it stands rather than from its beginning.
- **A request whose number cannot be used is skipped with a warning.** `RiFormat` and
  `RiPixelFilter` in the C interface refuse the same values, and the context keeps what it had.
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
- **The C interface reads a parameter's values by its declared type.** An `integer` parameter's
  array holds `RtInt`s and a `string` parameter's holds `RtString`s. Every other type's holds
  `RtFloat`s.
- **A C entry point and a file's request reach the same `RIBHandler` method.** A variadic entry
  point forwards to its `V` form, which converts the arguments and calls the handler, so the two
  forms of a request cannot differ. `RiPolygon` and the RIB `Polygon` both read `"P"`, `"Cs"`,
  `"N"` and `"st"`. The quadrics, `RiPoints` and `RiSubdivisionMesh` reach the handler as their
  RIB requests do. The C interface gives a subdivision mesh's tags two counts each, as RI 3.2
  defines it. `RiObjectBegin` returns no handle and reaches no handler.
- **A shader, light or space with an empty name names nothing.** `RIBHandler` ignores the
  request, and the state stays as it was. The C interface passes `RI_NULL` on as the empty name.
- **`RiGetContext` returns a handle that lasts until `RiEnd` ends its context.** `RiContext`
  makes that context active again, and a handle to a context that has ended is ignored. `RiEnd`
  with no context begun does nothing.
- **A predefined space cannot be saved over.** The camera sets them, so the hiders read the
  camera the scene set up. `RiCoordinateSystem` with one of them is logged and saves nothing.
- **A quadric's varying parameters hold four values**, one at each corner of its parameter
  space, as RI defines them. A uniform one holds one. The reader reads the seven quadrics alike.
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
  unbracketed varying or vertex parameter ends the parse instead of being guessed at.
- **A light's or an object's handle may be a number or a string.** RIB 3.03 writes a number and
  later RIB writes a string.
- **An object holds geometry alone.** `ObjectBegin` to `ObjectEnd` marks the geometry an
  `ObjectInstance` places, with the attributes and transformation in force where it is placed.
  `RIBHandler` does not instance, so it leaves out the geometry inside an object, as RI leaves it
  out where it is declared, and logs the first object.
- **`RIBHandler` logs each request its hiders do not draw**, once per name: `Points`, the quadrics
  but `Sphere`, `SubdivisionMesh` and `ObjectBegin`. The handler's empty bodies accept them, so
  another handler sees each one.
- **A `PixelFilter` name the reader does not recognise never reaches the handler.** A
  `PixelSamples` rate reaches the handler as a count of at least one. `DepthOfField` with no
  arguments is sent as an infinite fstop.
- **`Attribute "displacementbound"` is kept in camera space**, scaled by the largest stretch of
  the axes of the space it was given in.
- **`AreaLightSource` becomes an ordinary light** unless a handler overrides
  `Handler::areaLightSource`. moya overrides it, and `RiAreaLightSource` does the same.
- **A primitive repeated inside a motion block is a deformation, which moya does not build.**
  The reader hands the later poses to `Handler::deformation()`, which returns null unless a
  renderer overrides it, and lists them in `Reader::unsupported()`. `RIBHandler` leaves out a
  later pose that reaches it through the C interface in the same way, and logs the first one.
- **A polygon's `"st"` is carried by both hiders.** A traced triangle weights its corners' values
  by its barycentric coordinates. A triangle given none has `s` and `t` equal to those
  coordinates. The reyes hider interpolates `"st"` across a grid, as it does `"Cs"`, and carries
  both through a split. A grid whose corners have none uses its own parameters.

### The scene client

`moya::ri::SceneClient` fills the new core's scene from RI requests, beside `RIBHandler`, which
drives the reyes and ray hiders. [Scene.md](Scene.md) describes the scene it fills.

- **The options and the camera are made with the client**, named `options` and `camera`, so a
  scene that names neither has RI's defaults. The frame's requests set their parameters.
  `PixelSamples` sets the samples per pixel to its two counts multiplied.
- **`WorldBegin` makes the current transformation the camera's.** It takes world space to camera
  space, so the camera holds its inverse, and the transformation is reset for the world.
- **The current transformation holds a sample at each time of a motion block.** Inside a block,
  each request gives the transformation at the block's next time, built on the transformation
  the block opened with. Outside a block, a request applies to every sample. A block whose times
  do not rise is logged, and keeps its first time alone.
- **A display becomes an output named after its file.** A name starting with `+` adds an output,
  and any other replaces every output before it. A `.png` file is written as PNG, and any other
  as EXR, unless the type is `"framebuffer"`. A mode of `"rgb"` or `"rgba"` writes the beauty
  alone, and any other mode adds a channel of that source, named after it.
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
  their RIB storage class. `"P"` comes from `"Pw"` divided out when there is no `"P"`.
- **A primitive repeated inside a motion block deforms.** Each later pose adds a sample, at the
  block's next time, to each parameter of the first pose's prototype that may move. A pose whose
  points differ in number keeps the first pose alone, and is logged.
- **A primitive outside the world is not drawn**, and the first is logged.
- **The material is made when a geometry needs one**, from the attribute state's `Surface`,
  `Displacement`, `Color` and `Opacity`. Each shader is a node of language `ri` with its
  parameters as inputs, and the colour and opacity are the surface node's `Cs` and `Os`. The
  instances made under one attribute state share it. With no `Surface`, the surface is
  `defaultsurface`.
- **An object is a group.** `ObjectBegin` saves the attribute state and resets the
  transformation, so the object's geometry is placed in its own space, and its instances name
  the group and no material. `ObjectInstance` places the group at the current state. An
  `ObjectInstance` of an object no `ObjectBegin` made is logged.
- **RI's three standard lights become UsdLux's.** `pointlight` is a sphere light of radius zero
  at its `from`. `distantlight` is a distant light. `spotlight` is a sphere light of radius zero
  whose cone angle is its `coneangle` in degrees, and whose softness is its `conedeltaangle` over
  its `coneangle`. Each points its `-z` from its `from` towards its `to`, and a point light,
  which has no `to`, looks down z from its `from`. Each holds its
  `intensity` and `lightcolor`. Another light shader is logged and makes no light, and
  `Illuminate` passes over its handle.
- **A light that lit every instance keeps `includeRoot`.** Any other sets `includeRoot` to 0 and
  names the instances made while it was on, so a light that was off for every instance lights
  none.
  `Illuminate` turns a light on and off in the attribute state, and `AttributeEnd` restores it.
- **`AreaLightSource` makes a mesh light of each geometry after it in its attribute block**, with
  that geometry's prototype as its shape, and its `intensity` and `lightcolor`.
- **`CoordinateSystem` names the current transformation**, and `CoordSysTransform` makes a named
  one current. `"world"` is the identity, and `"camera"` is the camera's own space. Another name
  is logged.
- **A new world replaces the one before it.** `WorldBegin` removes every object the world before
  it made, and keeps the options, the camera and the outputs.
- **`moya --scene`** reads a file through the client and prints the scene's description after
  each frame, in place of rendering it.

### Writing and recording

Two handlers in `api/render/offline/rib` take requests from anything that sends them, such as
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
