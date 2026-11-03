# RIB requests

This page lists each RIB request moya reads and what moya does with it. A request moya does not
list here is skipped.

## The file

- **moya reads ASCII RIB only.** A binary or gzipped file is refused with a message that says
  which it is. A UTF-8 byte order mark at the start is skipped.
- **A line that starts with `#` is a comment.** `##RenderMan RIB-Structure 1.1` and the other
  `##` lines are comments too.
- **A request moya does not recognise is skipped with a warning**, once for each name, and its
  arguments are skipped with it.
- **A request whose number cannot be used is skipped with a warning**, and reading goes on with
  the next request. The cases are each listed below with their request.
- **A malformed request stops the read.** moya prints `error reading rib file` with the line and
  column, and renders nothing. A count above 4294967040, or a numeric light handle above two
  billion, counts as malformed.

## Parameter lists

Many requests end with a parameter list: pairs of a name in quotes and a value in brackets, such
as `"intensity" [0.5]`.

- **Each name needs a type.** moya has the types of the names in the table below. For any other
  name, give the type in the name, as in `"uniform float frequency" [6]`, or declare it once with
  `Declare`. A name with no type is skipped with a warning, and a shader then uses its default.
- A type in the name is a storage class, which may be left out, then a type. The classes are
  `constant`, `uniform`, `varying` and `vertex`. The types are `float`, `integer` (or `int`),
  `string`, `color`, `point`, `vector`, `normal`, `hpoint` and `matrix`. A type may end with a
  count in brackets, such as `float[2]`.
- **Bracket the values of a varying or vertex parameter**, such as `"P"`. moya finds the number
  of vertices of a polygon from the length of `"P"`, so an unbracketed value there ends the
  request's parameters.
- A parameter a request does not use is ignored. `Display` logs each parameter it is given.

The names with known types:

| Names | Type |
|---|---|
| `P` | vertex point |
| `Pz` | vertex float |
| `Pw` | vertex hpoint |
| `N` | varying normal |
| `Np` | uniform normal |
| `Cs`, `Os` | varying color |
| `s`, `t`, `width` | varying float |
| `st` | varying float[2] |
| `constantwidth` | constant float |
| `Ka`, `Kd`, `Ks`, `Kr`, `roughness`, `intensity`, `coneangle`, `conedeltaangle`, `beamdistribution`, `mindistance`, `maxdistance`, `distance`, `amplitude`, `fov` | uniform float |
| `specularcolor`, `lightcolor`, `background` | uniform color |
| `from`, `to` | uniform point |
| `texturename`, `name`, `shadinggroup`, `shader`, `texture` | uniform string |
| `bucketsize` | uniform integer[2] |
| `gridsize`, `maxdepth` | uniform integer |

## Options

Options set up the picture and the camera. Put them before `WorldBegin`. They take effect at
`WorldBegin`, in any order.

| Request | What moya does |
|---|---|
| `version 3.03` | Nothing |
| `Declare "name" "class type"` | Gives a parameter name a type for the rest of the file |
| `Option "searchpath" "shader" ["dirs"]` | Sets the directories searched for shader files. See [Shaders.md](Shaders.md#2-tell-moya-where-the-shader-is) |
| `Option "searchpath" "texture" ["dirs"]` | Sets the directories searched for texture images |
| `Option "limits" "bucketsize" [w h] "gridsize" [n]` | Sets the reyes bucket size and grid size. `--bucket` and `--grid` win over it |
| `Option "trace" "maxdepth" [n]` | Sets how deep traced rays go, from 0 to 16. Default 2 |
| `Hider "name"` | `"hidden"` for the reyes hider, the default, or `"raytrace"`. See [Hiders.md](Hiders.md) |
| `Format width height aspect` | The picture's size in pixels and the shape of a pixel. Default 320 240 1 |
| `FrameAspectRatio ratio` | The frame's width divided by its height. It follows `Format` by default |
| `ScreenWindow left right bottom top` | The part of the projection the picture shows |
| `CropWindow xmin xmax ymin ymax` | Renders only that part of the picture. See [Output.md](Output.md#render-part-of-the-picture) |
| `Projection "name" "fov" [degrees]` | `"perspective"` or `"orthographic"`. Default orthographic, with a fov of 90 for perspective |
| `Clipping near far` | The nearest and furthest distances drawn. Default 1e-10 to 1e38 |
| `Display "file" "type" "mode"` | The file to write. Only type `"file"` writes. The mode is ignored |
| `Imager "name" params` | An imager shader, run over the finished picture. `"background"` is built in |
| `PixelSamples x y` | Samples per pixel along each axis, from 1 to 256. Default 2 2 |
| `PixelFilter "name" xwidth ywidth` | `"box"`, `"triangle"`, `"catmull-rom"`, `"gaussian"` or `"sinc"`. Default gaussian 2 2 |
| `PixelVariance v` | Under the ray-trace hider, adds samples where a pixel is noisier than v. Default 0, off |
| `DepthOfField fstop focallength focaldistance` | Sets the lens. With no arguments, a pinhole. See [Camera.md](Camera.md#add-depth-of-field) |
| `Shutter open close` | The times the shutter opens and closes. Default 0 0, no motion blur |

Rules:

- `Option` reads only the names in the table. Any other option is ignored.
- A `Format` side above 65536, between 0 and 1, or not a finite number skips the request with a
  warning. A side of zero or less is the default for that side. A pixel aspect that is not above
  zero is 1.
- A `PixelFilter` name moya does not recognise is reported, and the filter stays as it was. A width
  that is not above zero skips the request with a warning.
- A `Projection` name other than the two is logged, and the projection stays as it was.
- A `CropWindow` with no area is logged and refused.
- A `maxdepth` above 16 is taken as 16, with a warning. One that is negative or not a number is
  not used, with a warning. A fraction is rounded down.

## Blocks

| Request | What moya does |
|---|---|
| `FrameBegin n`, `FrameEnd` | Nothing. A frame number that is not a finite number skips the request with a warning |
| `WorldBegin` | Fixes the camera from the options and the transformation as they stand. The scene follows |
| `WorldEnd` | Renders the scene and writes the picture |
| `AttributeBegin`, `AttributeEnd` | Saves and restores the transformation and every attribute: colour, opacity, shaders, lights switched on, shading rate, basis and the `Attribute` values. A light made inside the block is switched off at its end |
| `TransformBegin`, `TransformEnd` | Saves and restores the transformation only |
| `MotionBegin [t0 t1]`, `MotionEnd` | Holds one transformation request for each time, for motion blur. See [Camera.md](Camera.md#add-motion-blur) |

- A polygon given before `WorldBegin` is not drawn, and the first is logged.
- A primitive given again inside a motion block, to change its shape, is drawn at its first
  shape. moya does not build deforming primitives.

## Transformations

| Request | What moya does |
|---|---|
| `Identity` | Resets the transformation |
| `Transform [16 numbers]` | Replaces the transformation |
| `ConcatTransform [16 numbers]` | Applies a matrix before the current transformation |
| `Translate dx dy dz` | Moves |
| `Rotate degrees ax ay az` | Turns about an axis |
| `Scale sx sy sz` | Scales along each axis |
| `CoordinateSystem "name"` | Saves the current transformation under a name |
| `CoordSysTransform "name"` | Replaces the transformation with a saved or predefined one |

- **The transformation before `WorldBegin` places the camera.** It maps world space into camera
  space, where the camera is at the origin looking along +z, with +x right and +y up.
- **A request applies before the requests already given.** Each one changes the points of what
  follows first, then the earlier requests apply.
- **A matrix is 16 numbers, row by row.** A point is a row vector on its left, so the last row is
  the translation.
- **A positive `Rotate` about y turns +z toward +x. About x, it turns +z toward -y.**
- **The predefined spaces** are `object`, `world`, `camera`, `screen`, `raster` and `NDC`. The
  camera sets them. `CoordinateSystem` with one of these names is logged and saves nothing.
- `CoordSysTransform` with a name that is neither predefined nor saved is logged, and the
  transformation stays as it was.

## Attributes

Attributes belong to the primitives that follow, until a block ends or the attribute is set
again.

| Request | What moya does |
|---|---|
| `Color [r g b]` | The surface colour, `Cs` in a shader. Default white |
| `Opacity [r g b]` | The surface opacity, `Os` in a shader. Default 1, opaque. A shadow lets through one minus it |
| `ShadingRate area` | Under the reyes hider, the area of one micropolygon in pixels. Default 1. See [Quality.md](Quality.md#shading-rate-reyes) |
| `Basis "name" step "name" step` | The basis of cubic curves. See [Curves.md](Curves.md#choose-the-basis-of-a-cubic-curve) |
| `Attribute "light" "integer nsamples" [n]` | How many points an area light made after it samples, from 1 to 4096. Default 16 |
| `Attribute "displacementbound" "float sphere" [d]` | How far a displacement shader can move a point. See [Displacement.md](Displacement.md#3-set-the-displacement-bound) |

- `Attribute` reads only the names in the table. Any other attribute is reported once by name
  and ignored.
- A `Basis` name is `"bezier"`, `"b-spline"`, `"catmull-rom"`, `"hermite"` or `"power"`, or it is
  16 numbers. Its step is from 1 to 4. Any other name or step is an error that stops the read.

## Shaders and lights

| Request | What moya does |
|---|---|
| `Surface "name" params` | The surface shader for the primitives that follow. Default `constant` |
| `Displacement "name" params` | The displacement shader for the primitives that follow. See [Displacement.md](Displacement.md) |
| `LightSource "name" handle params` | Creates a light and switches it on. See [Lights.md](Lights.md) |
| `AreaLightSource "name" handle params` | Creates an area light. The polygons and spheres after it in the same block are its geometry |
| `Illuminate handle on` | Switches a light on (1) or off (0) for the primitives that follow |
| `MakeTexture ...` | Nothing. A shader reads the image file itself. See [Textures.md](Textures.md) |

- A handle is a number or a string.
- A shader, light or space named with an empty string names nothing, and the request is ignored.
- A shader that does not compile, or that moya cannot find, is logged. A surface is drawn with
  `matte` instead, and a light is left out. A shader of the wrong type, such as a light given to
  `Surface`, is treated the same way.

## Geometry

| Request | What moya does |
|---|---|
| `Polygon params` | One flat, convex polygon. Reads `"P"`, `"Cs"`, `"N"` and `"st"` |
| `PointsPolygons [counts] [indices] params` | A set of polygons that share points. Reads `"P"`, `"Cs"`, `"N"` and `"st"`, one value for each point |
| `Sphere radius zmin zmax thetamax` | A sphere cut to the heights zmin to zmax and swept through thetamax degrees. Drawn by the ray-trace hider only. Reads no parameters |
| `Curves "type" [counts] "wrap" params` | Ribbons along lines or cubic curves. See [Curves.md](Curves.md) |

- **`"P"`** holds three numbers for each vertex. A polygon's number of vertices is the length of
  `"P"` divided by three.
- **`"Cs"`** gives a colour at each vertex, blended across the polygon. Without it, the polygon
  takes the current `Color`.
- **`"N"`** gives a shading normal at each vertex, blended across the polygon, which makes a
  faceted surface look smooth. Without it, the normal is the polygon's own.
- **`"st"`** gives two texture coordinates at each vertex. See
  [Textures.md](Textures.md#map-the-image-with-st).
- **A polygon's winding sets its geometric normal.** A polygon wound clockwise as the camera sees
  it has a geometric normal that faces the camera. A scene that gives `"N"` facing the camera
  must wind its polygons that way too, or the built-in shaders shade it from behind.
- A polygon with more than four vertices must be convex. The reyes hider may draw a concave one
  wrongly.
- In `PointsPolygons`, a face with fewer than three vertices is skipped. A face whose indices run
  past the end of the index list ends the request.
- A sphere whose radius is not above zero is logged and not drawn. `Orientation` is not read, so
  a sphere cannot be turned inside out.

## What moya does not read

Requests such as `Orientation`, `Sides`, `ObjectBegin`, `ReadArchive`, `Patch`, `NuPatch` and
every quadric other than `Sphere` are skipped with a warning, as is every request this page does
not list. moya has no volume shaders, no shadow maps and no `Display` mode other than RGB.
