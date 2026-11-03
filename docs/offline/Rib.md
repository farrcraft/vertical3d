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
  `"N"` and `"st"`.
- **A shader, light or space with an empty name names nothing.** `RIBHandler` ignores the
  request, and the state stays as it was. The C interface passes `RI_NULL` on as the empty name.
- **`RiGetContext` returns a handle that lasts until `RiEnd` ends its context.** `RiContext`
  makes that context active again, and a handle to a context that has ended is ignored. `RiEnd`
  with no context begun does nothing.
- **A predefined space cannot be saved over.** The camera sets them, so the hiders read the
  camera the scene set up. `RiCoordinateSystem` with one of them is logged and saves nothing.
- **A `Sphere`'s varying parameters hold four values**, one at each corner of its parameter
  space, as RI defines them for a quadric. A uniform one holds one.
- **`rib::namedBasis()` gives a basis name's matrix**, in RI's row order, and `rib::basisName()`
  a matrix's name, which the writer uses to write a standard basis by name.
- **A `Curves` request's varying parameters are not checked for length.** How many values one
  holds depends on the basis's step, which the reader does not track.
- **A `Polygon` carries no vertex count.** The count is the length of `"P"`, which the reader
  divides out. A parameter list is therefore parsed before the count is known, and an
  unbracketed varying or vertex parameter ends the parse instead of being guessed at.
- **A light handle may be a number or a string.** RIB 3.03 writes a number and later RIB
  writes a string.
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

Background: [ADR-0023](../adr/0023-offline-rib-is-the-scene-format.md),
[ADR-0025](../adr/0025-offline-rib-reader-calls-a-typed-handler-interface.md)
