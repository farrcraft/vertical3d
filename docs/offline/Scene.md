# The scene

This page is for anyone writing a client of moya's new core, or a structure built from its
scene. The scene is in `moya/libmoya/scene/`, in the namespace `v3d::moya::scene`. It is host
code: no device reads it, and the structures a render needs are built from it. RIB and the editor
fill it, and the reyes and ray hiders do not read it.

## Kinds and their parameters

Every object of the scene is of a kind, and a kind declares its parameters once.

- **A `Kind`** has a name and a list of `Declaration`s. A `Schema` holds the kinds a scene's
  objects can be, each reached by its index.
- **A `Declaration`** gives a parameter's name, its `Type`, its element count, whether it holds
  samples over the shutter, and its fallback. An object's parameter holds its fallback until a
  client sets it.
- **The count** is one for a parameter that is not an array, a number for an array of exactly
  that many, or `Declaration::kAny` for an array of any length.
- **A string parameter may give its choices**, the strings it may hold. A string outside them is
  refused.
- **The types** are integer, float, string, colour, point, vector, normal, matrix and handle. A
  handle parameter names the kinds of object it may hold, its targets.
- **`Schema::add()` refuses a broken kind** with `std::invalid_argument`:
  - a kind or a parameter with no name, or two kinds or two parameters of one kind with the same
    name
  - a fallback whose type differs from its declaration's
  - a fallback whose count differs from its declaration's, or a string outside its choices
  - choices on a parameter that is not a string
  - a handle parameter with no targets, or one that is no kind added before it

Background: [ADR-0200](../adr/offline/0200-offline-scene-objects-are-nodes-of-declared-parameters.md)

## moya's kinds

`scene::kinds()` gives the schema of moya's scene. Every default is RI's where RI names one. A
parameter whose RI default depends on another is empty or zero until a client sets it, and is
then derived as RI derives it.

- **`camera`**: `transform` from camera space to world space, over the shutter. `projection`,
  `orthographic` or `perspective`, and `fov` in degrees for a perspective one. `screen`, the
  screen window as left, right, bottom and top, which is derived from the frame's aspect while it
  is empty. `near` and `far` clipping. `fstop`, `focalLength` and `focalDistance` for depth of
  field, with an infinite f-stop for a pinhole. `shutter`, its open and close times.
- **`options`**: `resolution` in pixels, `pixelAspect`, and `frameAspect`, which is derived from
  the resolution while it is zero. `crop` as RI's crop window. `samples` per pixel, the pixel
  `filter`, one of RI's five, and `filterWidth`. `frame`, which seeds the sampler. `camera`, the
  camera the render looks through. `edgeLength`, the longest a tessellated edge may be in
  pixels, 1 until a client sets it, and `dicingCamera`, the camera that length is measured
  from, which is the render camera while it names none. `maxDepth`, `diffuseDepth`,
  `glossyDepth`, `specularDepth`, `transmissionDepth` and `rouletteDepth` limit a path's
  bounces, and `environment` is the colour a path that leaves the scene meets, as
  [Integrator.md](Integrator.md#depth-limits-and-russian-roulette) says. `lightSampling` is how a
  path finds the lights: `mis`, the fallback, `light` or `bsdf`, as
  [Integrator.md](Integrator.md#lights) says.
- **`channel`**: `source`, what the film channel holds, and `components`, its floats per pixel.
  The channel's name is the object's.
- **`output`**: `file` and `format`, which is `exr`, `png` or `framebuffer`. `channels`, the
  channels it writes beside the beauty, which is alone when there are none. `precision`, `half`
  or `float`, and `compression`, one of OpenEXR's.
- **`mesh`**: `P` over the shutter, `vertices`, the count of each face's vertices, and `indices`
  into `P`, face after face.
- **`subdivision`**: a mesh's three, then its `scheme`, `catmull-clark`, `loop` or `bilinear`.
  `creases` holds the vertices of each crease in turn, `creaseLengths` how many each has, and
  `creaseSharpness` one sharpness each. `corners` and `cornerSharpness`, `holes` by face, and
  `boundary`, as RI's `interpolateboundary` gives it. `displacementBound`, the furthest its
  surface is displaced, in its own space. A float primvar named `displacement` gives the height
  it is displaced by.
- **`curves`**: `P` and `vertices` per curve, `type` linear or cubic, `basis` one of RI's five,
  and `wrap`. `width` per varying value or per vertex over the shutter, or `constantWidth` while
  it is empty, and `profile`, flat or round.
- **`points`**: `P`, and `width` per point or `constantWidth`. A `profile` of `round`, RI's
  default, makes each point a sphere, and `flat` a disk that faces the ray.
- **The quadrics**, `sphere`, `cone`, `cylinder`, `hyperboloid`, `paraboloid`, `disk` and `torus`,
  take RI's dimensions, and each may move over the shutter. RI gives no defaults, so each is a
  unit shape until a client sets it.
- **`group`**: a prototype made of the instances that name it.
- **`instance`**: the `prototype` it places, any geometry or a group, and the `group` it belongs
  to, or none for the world. `transform` over the shutter, its `material`, and its visibility
  to `camera`, `shadow` and `indirect` rays, each 1 or 0. An instance of a group places every
  instance in it, so instancing nests. An instance in a group with no material takes the
  material of the instance that places the group. Flattening it into placements is the work of
  whatever is built from the scene, which stops at a group that holds itself.
- **`shader`**: a node of a material's network. `shader` names what it runs, and `language` the
  language it is written in, `materialx`, `osl` or `ri`. Its inputs are declared on it as
  primvars of constant interpolation, so each node holds the inputs its shader takes.
- **`connection`**: `from` one node's `output`, `to` another node's `input`.
- **`material`**: its terminals, `surface` and `displacement`, each a node and an output. The
  scene does not read the shaders a network names. Compiling them is the work of whatever is
  built from the scene.
- **The lights** are UsdLux's, with its parameters and defaults: `rectLight`, `diskLight`,
  `sphereLight`, `cylinderLight`, `meshLight`, `distantLight` and `domeLight`. Each holds a
  `transform` over the shutter, `colour`, `intensity`, `exposure` and `normalize`, and what it
  lights. Each also holds a colour temperature in kelvin, `colorTemperature`, which applies while
  `enableColorTemperature` is 1. As in UsdLux's light linking, a light lights every instance
  while its `includeRoot` is 1, the fallback. When it is 0, the light lights only the `instances`
  it names, which may be none. A light with an area holds `coneAngle` and `coneSoftness`. It also
  holds `camera`, which is 1 for a light camera rays see, and 0, the fallback, for one they do
  not. A rect light holds its `width`, `height` and `texture`, a disk or sphere light its
  `radius`, and a cylinder light its `radius` and `length`. A distant light holds its `angle` in
  degrees, and a dome light its `texture`. A mesh light takes its shape from the geometry
  prototype its `geometry` names.
- **Every geometry holds primvars.** Its points' count is the same at every time, so a deforming
  geometry keeps its topology.

## Values

A `Value` is an array of elements of one type.

- **A parameter that is not an array holds a value of one element.** `Value::integer()`,
  `number()`, `colour()` and the others make one, and `integers()`, `floats()`, `colours()` and
  the others make an array.
- **Colours, points, vectors, normals and matrices are floats.** `floatArray()` gives them element
  after element, 3 floats for each colour, point, vector or normal and 16 for each matrix, column
  by column. `triple()` and `matrixAt()` give one element.
- **A colour value names its colour space** as OpenColorIO names it. An empty name is the working
  space.
- **Reading a value as another type** throws `std::logic_error`.

## Handles and names

`create(kind, name)` returns a `Handle`: the object's kind, its index among the scene's objects,
and the generation of that index.

- **An index is used again** once its object is removed, with the next generation. The handle of
  a removed object therefore matches nothing, and every call that takes it refuses it.
- **A handle no scene gave** has generation zero, and `valid()` is false. `Value::handle({})` is
  the null handle.
- **A handle parameter is set to a live object** of one of its targets. `set()` refuses a handle
  that names no object, as a removed object's does, and one of a kind the parameter does not
  take.
- **The null handle sets a parameter that is not an array back to naming nothing**, its
  fallback. An array of handles names nothing when it is empty, and takes no null handle.
- **A handle parameter that names an object removed after it was set names nothing.** The scene
  does not clear it, and a structure built from the scene treats it as absent.
- **Every object has a name**, unique among the objects of its kind that have not been removed.
  An empty name, or one that another live object of the kind has, throws
  `std::invalid_argument`. A removed object's name is free again at once.
- **A name does not depend on the order objects were made in**, as an index does.
  `find(kind, name)` gives the handle of the object with a name, and `name(handle)` gives an
  object's name.

## Samples over the shutter

A parameter declared for motion holds `Samples`: one value at each of a list of times.

- **The times are the object's own**, as its client gave them. Two objects need not share times
  or a count of samples.
- **The times rise strictly**, there is one value at each, and every value has the same type and
  size. A deforming geometry keeps its topology at every time.
- **A value set with no times holds at every time.** A parameter not declared for motion takes
  only that.

Background: [ADR-0201](../adr/offline/0201-offline-each-scene-object-keeps-its-own-sample-times.md)

## Primvars

A kind that holds primvars takes a declaration of each one from its client, per object, with
`declare()`.

- **A primvar's `Interpolation`** is one of RI's and USD's five classes: constant, uniform,
  varying, vertex and face-varying.
- **A primvar is set and read as a parameter** once it is declared, and holds its fallback until
  it is set.
- **Declaring a primvar the object already has replaces it**, and its value goes back to the new
  declaration's fallback.
- **A primvar cannot take the name of one of its kind's parameters.**

## Edits, commits and versions

- **A client edits from any thread.** `create()`, `set()`, `declare()` and `remove()` are checked
  when they are called, and refused with `std::invalid_argument`. An edit that passes is held
  back.
- **`commit()` closes the edits since the last commit into one batch.**
- **The owner applies.** `apply()` makes each committed batch one new version of the scene, in
  the order they were committed. A render calls it between passes, never during one, so a pass
  sees whole batches. `apply()` and every read are called by one thread, the owner.
- **The reads** are `contains()`, `objects()`, `find()`, `name()`, `samples()`, `value()` and
  `primvars()`. A read of
  an object the scene does not hold throws `std::out_of_range`. An object a client created is not
  held until its batch is applied.

## Changes

Each object holds the version that created it, and each parameter the version that last set or
declared it.

- **A consumer keeps the version it last built from**, and `changes(since)` gives every object
  created, modified or removed after it. A modified object lists the parameters that changed.
- **Several consumers** each ask with their own version, so none hides a change from another.
- **The changes are ordered by index.** At one index, a removal comes before the object made
  there after it.
- **`forget(through)`** drops the record of removals up to a version, once every consumer has
  built past it.

Background: [ADR-0203](../adr/offline/0203-offline-scene-changes-are-versioned-and-committed.md)

## The description

`scene::describe(scene)` writes the objects the applied batches hold as text. Tests compare a
scene with a committed description, and two scenes that hold the same objects describe alike
whatever order they were made in.

- **The order is by name.** Kinds come in name order, the objects of each kind in name order,
  and each object's parameters and primvars in name order.
- **An object** is a line of its kind and its quoted name. Each parameter follows on a line of
  its own, indented four spaces.
- **A parameter's line** gives its name and type, then its element count in brackets when it is
  an array, then a primvar's interpolation class unless it is constant. Its value follows `=`.
  A parameter with samples gives each one as `@ time = value`.
- **A float** is written to six significant digits, and a negative zero as zero. Two compilers'
  mathematics round a sine or an inverse differently in the last bits a float holds, and six
  digits leave those bits out, so a description is the same on every platform. A colour, point,
  vector or normal is three floats in parentheses, and a colour with a space names it after
  `in`. A matrix is sixteen floats in square brackets, column by column. A string is quoted,
  with a quote or a backslash inside it escaped.
- **A handle** is its object's kind and quoted name, or `none` when the scene holds no object it
  names.
