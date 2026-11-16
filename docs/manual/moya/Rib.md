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
  column. The frames before it are already rendered and written. A count above 4294967040, or a
  numeric light handle above two billion, counts as malformed.

## Parameter lists

Many requests end with a parameter list: pairs of a name in quotes and a value in brackets, such
as `"intensity" [0.5]`.

- **Each name needs a type.** moya has the types of the names in the table below. For any other
  name, give the type in the name, as in `"uniform float frequency" [6]`, or declare it once with
  `Declare`. A name with no type is skipped with a warning, and the parameter keeps its default.
- A type in the name is a storage class, which may be left out, then a type. The classes are
  `constant`, `uniform`, `varying`, `vertex` and `facevarying`. The types are `float`,
  `integer` (or `int`), `string`, `color`, `point`, `vector`, `normal`, `hpoint` and `matrix`. A
  type may end with a count in brackets, such as `float[2]`. RenderMan's `reference` may come
  before the type, as in `"reference color base_color" ["tint:out"]`, and its value names another
  node's output.
- **Bracket the values of a varying, vertex or facevarying parameter**, such as `"P"`. moya
  finds the number of vertices of a polygon from the length of `"P"`, so an unbracketed value
  there ends the request's parameters.
- **A parameter of an `Option`, `Attribute`, `Hider` or `Display` that moya does not read** is
  logged once by name, and changes nothing.
- **A parameter a primitive's kind does not hold becomes a primvar** of the primitive, which a
  material can read by its name.

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
| `Ka`, `Kd`, `Ks`, `Kr`, `roughness`, `intensity`, `coneangle`, `conedeltaangle`, `beamdistribution`, `mindistance`, `maxdistance`, `distance`, `amplitude`, `fov`, `bias` | uniform float |
| `specularcolor`, `lightcolor`, `background` | uniform color |
| `from`, `to` | uniform point |
| `texturename`, `name`, `shadinggroup`, `shader`, `texture` | uniform string |
| `bucketsize` | uniform integer[2] |
| `gridsize`, `maxdepth` | uniform integer |

## Options

Options set up the picture and the camera. Put them before `WorldBegin`.

| Request | What moya does |
|---|---|
| `version 3.03` | Nothing |
| `Declare "name" "class type"` | Gives a parameter name a type for the rest of the file |
| `Option "searchpath" "shader" ["dirs"]` | Sets the directories searched for MaterialX documents that define surfaces and displacements. See [Materials.md](Materials.md#1-tell-moya-where-the-documents-are) |
| `Option "searchpath" "texture" ["dirs"]` | Sets the directories searched for images. See [Textures.md](Textures.md#where-moya-looks-for-the-image) |
| `Option "limits" "texturememory" [kilobytes]` | The most memory the texture tiles of a render take. Default 1 GiB |
| `Option "limits" "bucketsize" [w h] "gridsize" [n]` | Nothing |
| `Option "trace" "maxdepth" [n]` | The most bounces a path takes. Default 12. See [Quality.md](Quality.md#bounces) |
| `Option "moya" "int denoise" [1]` | Denoises the picture after the last pass. See [Quality.md](Quality.md#denoising) |
| `Option "moya" "float deepTolerance" [d]` | How near two depths lie, as a share of the first, to join in a deep file. Default 0.001 |
| `Hider "name"` | Nothing. A name other than `"hidden"` or `"raytrace"` is logged |
| `Format width height aspect` | The picture's size in pixels and the shape of a pixel. Default 320 240 1 |
| `FrameAspectRatio ratio` | The frame's width divided by its height. It follows `Format` by default |
| `ScreenWindow left right bottom top` | The part of the projection the picture shows |
| `CropWindow xmin xmax ymin ymax` | Renders only that part of the picture. See [Output.md](Output.md#render-part-of-the-picture) |
| `Projection "name" "fov" [degrees]` | `"perspective"` or `"orthographic"`. Default orthographic, with a fov of 90 for perspective |
| `Clipping near far` | The nearest and furthest distances drawn. Default 1e-10 to 1e38 |
| `DepthOfField fstop focallength focaldistance` | Sets the lens. With no arguments, a pinhole. See [Camera.md](Camera.md#add-depth-of-field) |
| `Shutter open close` | The times the shutter opens and closes. Default 0 0, no motion blur |
| `PixelSamples x y` | Samples per pixel, x times y, rounded up to a power of two. Each from 1 to 256. Default 2 2 |
| `PixelFilter "name" xwidth ywidth` | `"box"`, `"triangle"`, `"catmull-rom"`, `"gaussian"` or `"sinc"`. Default gaussian 2 2 |
| `PixelVariance v` | Stops sampling a pixel once its grain falls below v. Default 0, off |
| `Display "file" "type" "mode"` | A file to write. See [Output.md](Output.md) |
| `DisplayChannel "type name" params` | A channel a `Display` mode can name, and what its `"string source"` collects. See [Output.md](Output.md#add-channels) |
| `SampleFilter "PxrCryptomatte" handle params` | A Cryptomatte file of its `"string filename"` and `"string layer"`: `Name` for objects, `Material` for materials, `Path` for assets. Any other sample filter is logged |
| `Imager "background" "background" [r g b]` | Lays the picture over the colour when a file is written. Any other imager is logged |
| `DisplayFilter "PxrBackgroundDisplayFilter" handle "color backgroundColor" [r g b]` | The same as the `background` imager. Any other display filter is logged |

Rules:

- `Option` reads only the names in the table.
- A `Format` side above 65536, between 0 and 1, or not a finite number skips the request with a
  warning. A side of zero or less is the default for that side. A pixel aspect that is not above
  zero is 1.
- A `PixelFilter` name moya does not recognise is reported, and the filter stays as it was. A width
  that is not above zero skips the request with a warning.
- A `Projection` name other than the two is logged, and the projection stays as it was.
- A `CropWindow` with no area is logged, and the whole picture is rendered.
- A `maxdepth` below zero is logged, and changes nothing.
- A `texturememory` below 1 is logged, and changes nothing.

## Blocks

| Request | What moya does |
|---|---|
| `FrameBegin n`, `FrameEnd` | Mark a frame. A frame number that is not a finite number skips the request with a warning |
| `WorldBegin` | Places the camera by the transformation as it stands. Clears every object, light and material of the world before it. The scene follows |
| `WorldEnd` | Renders the frame and writes its files |
| `AttributeBegin`, `AttributeEnd` | Saves and restores the transformation and every attribute: colour, opacity, material, displacement, lights switched on, basis and the `Attribute` values. A light made inside the block is switched off at its end |
| `TransformBegin`, `TransformEnd` | Saves and restores the transformation only |
| `MotionBegin [t0 t1 ...]`, `MotionEnd` | Holds one transformation request, or one primitive, for each time. See [Camera.md](Camera.md#add-motion-blur) |

- A primitive or a light given outside the world is not made, and the first is logged.
- An `AttributeBegin` or `ObjectBegin` still open at `WorldEnd` is closed there, with a warning.

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
| `CoordSysTransform "name"` | Replaces the transformation with a saved one, or with world or camera space |

- **The transformation before `WorldBegin` places the camera.** It maps world space into camera
  space, where the camera is at the origin looking along +z, with +x right and +y up.
- **A request applies before the requests already given.** Each one changes the points of what
  follows first, then the earlier requests apply.
- **A matrix is 16 numbers, row by row.** A point is a row vector on its left, so the last row is
  the translation.
- **A positive `Rotate` about y turns +z toward +x. About x, it turns +z toward -y.**
- **`CoordSysTransform "world"`** resets the transformation, and **`"camera"`** makes camera
  space current. Any other name has to be one `CoordinateSystem` saved in the same world. A name
  that is neither is logged, and the transformation stays as it was.

## Attributes

Attributes belong to the objects that follow, until a block ends or the attribute is set again.

| Request | What moya does |
|---|---|
| `Color [r g b]` | The surface colour. Default white |
| `Opacity [r g b]` | The surface opacity, as the mean of the three. Default 1, opaque. See [Materials.md](Materials.md#opacity) |
| `ShadingRate area` | Nothing |
| `Sides n` | 2, the default, lets a surface that glows, such as `constant`, glow from both faces, and 1 from its front alone. Both faces are seen either way |
| `Basis "name" step "name" step` | The basis of cubic curves. See [Curves.md](Curves.md#choose-the-basis-of-a-cubic-curve) |
| `Attribute "identifier" "name" ["name"]` | The name of the objects and lights that follow, which a Cryptomatte file and the log use |
| `Attribute "displacementbound" "float sphere" [d]` | How far a displacement can move a point. See [Displacement.md](Displacement.md#3-set-the-displacement-bound) |
| `Attribute "trace" "bias" [d]` | Nothing |
| `Attribute "light" "integer nsamples" [n]` | Nothing |

- `Attribute` reads only the names in the table. Any other attribute is logged once by name.
- A `Basis` name is `"bezier"`, `"b-spline"`, `"catmull-rom"`, `"hermite"` or `"power"`, or it is
  16 numbers. Its step is from 1 to 4. Any other name or step is an error that stops the read.
- A `Sides` count other than 1 or 2 is logged, and changes nothing.

## Materials and lights

| Request | What moya does |
|---|---|
| `Surface "name" params` | The material of the objects that follow. Default `matte`. See [Materials.md](Materials.md) |
| `Displacement "name" params` | The displacement of the subdivision surfaces that follow. See [Displacement.md](Displacement.md) |
| `Pattern "type" handle params` | A MaterialX node of the material, such as `ND_constant_color3` |
| `Bxdf "type" handle params` | The surface node of the material, such as `ND_open_pbr_surface_surfaceshader`, or `materialx` with a `file` and a `material` to read one from a document. See [Materials.md](Materials.md#rendermans-bxdf-and-pattern) |
| `LightSource "name" handle params` | Creates one of RI's lights and switches it on. See [Lights.md](Lights.md) |
| `AreaLightSource "arealight" handle params` | Makes the polygons, meshes and subdivision surfaces after it in the same block glow |
| `Light "name" handle params` | Creates one of RenderMan's lights, such as `PxrRectLight`, and switches it on |
| `Illuminate handle on` | Switches a light on (1) or off (0) for the objects that follow |
| `MakeTexture "image" "texture" swrap twrap filter swidth twidth` | Makes a texture file from an image at once. See [Textures.md](Textures.md#make-a-texture-yourself) |

- A handle is a number or a string.
- A material, light or space named with an empty string names nothing, and the request is
  ignored.
- A surface moya cannot find is logged once, and shades as `matte`. A displacement moya cannot
  find is logged once, and moves nothing.
- A light of any other name is logged once, and makes no light. `Illuminate` passes over its
  handle.

## Geometry

| Request | What moya does |
|---|---|
| `Polygon params` | One flat polygon. Reads `"P"`, `"Pw"`, `"N"`, and every other parameter as a primvar |
| `PointsPolygons [counts] [indices] params` | A mesh of polygons that share points |
| `SubdivisionMesh "scheme" [counts] [indices] [tags] [nargs] [ints] [floats] params` | A smooth surface whose control mesh is the polygons given. The scheme is `"catmull-clark"`, `"loop"` or `"bilinear"`. The tags `crease`, `corner`, `hole` and `interpolateboundary` are read |
| `Sphere radius zmin zmax thetamax` | A sphere cut to the heights zmin to zmax and swept through thetamax degrees |
| `Cone`, `Cylinder`, `Hyperboloid`, `Paraboloid`, `Disk`, `Torus` | RI's other quadrics, as RI defines each |
| `Points params` | A small sphere at each point of `"P"`, as wide as `"width"` or `"constantwidth"` gives, or 1 |
| `Curves "type" [counts] "wrap" params` | Ribbons along lines or cubic curves. See [Curves.md](Curves.md) |
| `ObjectBegin handle`, `ObjectEnd` | Keeps the geometry between them as an object, in its own space, and draws none of it there |
| `ObjectInstance handle` | Places the object at the current transformation, with the current material |

- **`"P"`** holds three numbers for each vertex. A polygon's number of vertices is the length of
  `"P"` divided by three. `"Pw"` gives four numbers for each vertex, and is used when there is no
  `"P"`.
- **`"Cs"`** gives a colour at each vertex, blended across the polygon. RI's surfaces other than
  `constant` read it. Without it, the polygon takes the current `Color`.
- **`"N"`** gives a shading normal at each vertex, blended across the polygon, which makes a
  faceted surface look smooth. Without it, the normal is the polygon's own.
- **`"st"`** gives two texture coordinates at each vertex. See
  [Textures.md](Textures.md#map-the-image-with-st). A quadric has none.
- **A polygon may be concave**, and is cut into triangles inside its own outline. A face with
  fewer than three vertices, or that covers no area, is dropped.
- Any other subdivision scheme is logged, and the mesh is subdivided by
  `"catmull-clark"`. A tag moya does not read is logged.
- An `ObjectInstance` of a handle no `ObjectBegin` made is logged.
- A primitive given again inside a motion block changes its shape over the shutter. See
  [Camera.md](Camera.md#change-an-objects-shape).

## What moya does not read

Requests such as `Orientation`, `ReadArchive`, `Patch`, `NuPatch` and `Atmosphere` are skipped
with a warning, as is every request this page does not list.
