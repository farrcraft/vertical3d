# Lights

This page is for anyone changing how moya places lights and what they emit. The
kernel side is in `moya/libmoya/kernel/light/`, in the namespace `v3d::moya::kernel::light`. What
samples a rectangle, an ellipse or a triangle uniformly in solid angle is in its subdirectory
`spherical/`, in `light::spherical`. The host side is `light::Lights`, in `moya/libmoya/light/`,
and the walk that places each light's shape is in `geometry::gather()`. A rect, disk, sphere or
cylinder light has a shape. A point and a distant light have none. A mesh light makes a geometry
of the scene emit, as an emitting material does, and a dome light surrounds the scene. A volume
light emits from inside a volume, or from the medium inside a closed surface.
[Scene.md](Scene.md) lists each light's parameters, and [Integrator.md](Integrator.md) covers the
paths that meet them.

## A light's shape

A rect, disk, sphere or cylinder light has a shape that no geometry in the scene holds. Its shape
is a prototype of the world's hierarchy, so the one traversal that finds geometry finds lights too.
`geometry::shaped()` says which lights have one.

- **Each light places its shape through an instance in the world**, after every other instance.
  The instance's index in the scene is the light's own, so a hit on the shape names the light.
- **Each shape is UsdLux's, in the light's own space.** A rect light is two triangles, its width
  along x and its height along y, centred on the origin and facing -z. A disk light is a disk of
  its radius facing -z. A sphere light is a sphere of its radius. A cylinder light is a cylinder of
  its radius and length along x, with no caps.
- **A disk, sphere or cylinder is one of RI's quadrics.** A quadric's cylinder lies along z, so a
  cylinder light's instance turns it onto x, as the light's transformation stands at every key.
- **A light's shape is built through the geometry cache**, keyed by the light's index. A change to
  a light builds its shape's hierarchy and the world's again, and no other.
- **A light's instance is visible to indirect rays.** Camera rays see it only while the light's
  `camera` parameter is 1, and shadow rays never meet it.
- **A sphere light of no radius, or one whose `treatAsPoint` is 1, places no shape.** It is a
  point, as [Points](#points) says.

Background: [ADR-0213](../adr/offline/0213-offline-a-lights-shape-is-a-primitive-of-the-hierarchy.md)

## What a light emits

`light::Lights` holds each light as a `kernel::light::Light`, with its emission's parts worked
out on the host: the lights with a shape first, then the points and the distant lights in the
order of their indices. A light with a shape emits a radiance, as UsdLux's does.

- **The scale** is the light's `intensity` times two to its `exposure`.
- **The spectrum** is its `colour` as an illuminant, converted into ACEScg from the space the
  colour names, so a white light shines with ACES white at a luminance of one.
- **A colour temperature** applies while `enableColorTemperature` is 1. The spectrum is then a
  Planckian radiator at `colorTemperature` kelvin, of a luminance of one, tinted by the colour as
  an unbounded spectrum. `light::planckian()` finds the radiator's scale and its ACEScg colour,
  summed at 1 nm through the observer. An RGB build uses the colour.
- **`normalize`** divides the radiance by the light's area in the world, as it stands when the
  shutter opens, so its power is the same at any size. A rect's and a disk's area are exact under
  any transformation. A sphere's and a cylinder's are summed over their normals, which is exact
  under a turn and a scale of the same size on every axis. A light of no area is not divided.
- **A light emits from one face.** A rect or disk light emits from the face its -z axis points
  to, and a sphere or cylinder light from its outer face. The inside of a cylinder light, seen
  through its open end, is dark.
- **A cone shapes what a light emits**, as UsdLux's shaping does. The angle off the light's -z
  axis is measured towards where the ray came from. Past `coneAngle` the light emits nothing, and
  from `coneAngle` times one less `coneSoftness` it falls to nothing along a smooth step. A cone of
  90 degrees or more with no softness shapes nothing, which is the fallback. The angle is held to
  [0, 180] degrees and the softness to [0, 1].

The light's axis is turned into the world as a normal is, by its transformation at the ray's
time. A light that moves over the shutter therefore emits from where it stands then.

- **Each light holds its own placement.** A light with a shape takes its instance's
  transformation and keys, and a light with none its `transform`.
  `kernel::light::Lights::placement()` gives a light's transformation at a time.

## Points

A sphere light of no radius, or one whose `treatAsPoint` is 1, is a point at its origin. UsdLux
calls `treatAsPoint` a hint, so a point emits as the sphere it stands for, and only its shadows
change.

- **A point's radiant intensity is the sphere's**: its radiance times pi times the radius
  squared, which is its radiance times its area in the world over four. With `normalize`, the
  radiance is divided by the area first, so the intensity is a quarter of the light's emission
  at any radius. A shrinking sphere light that is normalised therefore converges to its point.
- **A point of no radius without `normalize` emits nothing**, since the sphere it stands for has
  no area.
- **A point lights a surface by its intensity, shaped by its cone, over the distance squared.**
  Its cone is measured from its -z axis, as a shaped light's is.
- **RI's `pointlight` and `spotlight` are normalised points** of four times RI's `intensity`,
  so their radiant intensity is RI's, as [Rib.md](Rib.md#the-scene-client) says.

## Distant lights

A distant light shines along its -z axis from beyond the scene. Its `angle` is the diameter of
the cone of directions it comes from, held to [0, 360] degrees, and its half angle is held to a
half turn. It has no cone of emission.

- **A distant light of no angle** shines along one direction. Its emission is the irradiance it
  gives a plane that faces it, with `normalize` or without.
- **A distant light with an angle** emits a radiance over its cone. With `normalize`, the
  radiance is divided by pi sin^2 of the half angle, or pi (2 - sin^2) past a quarter turn, as
  UsdLux measures a distant light's size. Its emission is then the irradiance of a plane that
  faces it, at any angle.
- **Camera rays see no distant light**, and a distant light casts its shadows through every
  shadow ray, which runs on past everything in the scene.
- **`kernel::light::Lights::distant`** names the distant lights with an angle, which a ray that
  leaves the scene meets, as [Integrator.md](Integrator.md#lights) says.

## Mesh lights

A mesh light makes the geometry its `geometry` names emit, wherever the scene places it.
`light::Lights` makes a `kernel::light::Light` of shape `Mesh` for each chain of instances from
the world that places the geometry.

- **Each chain is a light of its own.** It holds the chain's instances, outermost first, and the
  geometry's place among the surfaces'. A geometry placed by two instances emits twice as much
  light as one.
- **The geometry emits from its triangles**: a mesh's, or those the gather cuts a subdivision
  surface into. A geometry with no triangles, such as a quadric, emits nothing, and the logger
  reports it once for each light.
- **A triangle emits from the side its winding faces**, turned into the world as a normal is, the
  side a shading point's geometric normal faces. An emitting material's triangles emit from both
  faces where the innermost instance of the chain is `doubleSided`, as the next section says.
- **Its radiance is UsdLux's**, from its `intensity`, `exposure`, `colour` and colour
  temperature. With `normalize`, it is divided by the area in the world of the triangles the
  chain places, as they stand when the shutter opens.
- **Its `transform`, `coneAngle` and `coneSoftness` are not read**, since the instances place
  the geometry and no cone shapes it.
- **Camera rays see what it emits only while its `camera` is 1.** The geometry is still seen as a
  surface, by its material.
- **A geometry that more than one mesh light names** emits by the first, in the order of their
  indices.
- **A geometry that moves emits from where it stands at the ray's time.** Its chain's
  transformation and its points are both read then.

**A material that emits** makes a mesh light of its own for each chain of instances that gives
it to a geometry of triangles. A chain gives its geometry the material of its innermost instance
that names one, as a shading point takes it. The material is layered, and emits OpenPBR's
`emissionLuminance`, in nits, in its `emissionColour`, in ACEScg. Camera rays see it, it is not
normalised, and it lights everything, with every instance casting its shadows. A geometry with
no triangles emits nothing from its material either. A chain whose geometry a mesh light also
names makes both lights, the mesh light's first, and their light adds.

- **A material emits from both faces of a double-sided instance.** Where the innermost instance
  of the chain is `doubleSided`, its light is two-sided. A hit on either face sees it, a light
  sample is drawn on the face towards the shading point, and its power counts both faces. The
  light tree bounds it by every direction. A mesh light emits from the side its winding faces,
  whatever the instance holds.

`kernel::light::Lights::emitting()` gives a mesh light whose geometry a hit's chain places, from
`meshes`, which holds the first mesh light each instance in the world places. Given the place of
one it found, it gives the next, so a hit meets both of its chain's lights.
`Lights::triangleAt()` gives a triangle in the world at a time, with the unit normal of the side
its winding faces, and `faceTowards()` turns that of a two-sided light to the side a point lies on.

Background: [ADR-0222](../adr/offline/0222-offline-a-bsdf-is-a-list-of-weighted-closures.md),
[ADR-0241](../adr/offline/0241-offline-ris-sides-decides-which-faces-a-material-emits-from.md)

## Volume lights

A volume whose program emits, and a closed surface whose medium emits, each make a light of
shape `Volume`, drawn from bricks of its medium. [Volumes.md](Volumes.md#volume-lights) states
how it is made, what it emits and its power. A light sample draws it as
[the light tree](#the-light-tree) and [drawing a light](#drawing-a-light) say.

## Images

A rect light's `texture` and a dome light's are EXR files. `light::internal::Textures` reads each
path once, through `output::readExr()`, and holds the images in a device's memory whole, as
`kernel::light::Image`s.

- **An image is read as floats**: its R, G and B channels, or its Y as grey. A channel it lacks is
  zero.
- **An image is converted into ACEScg** from the space its chromaticities name, one of the six
  linear spaces the EXR writer names. A file with none is linear Rec. 709, as OpenEXR takes it.
  Chromaticities of any other space are read as linear Rec. 709, and reported.
- **A file that cannot be read is reported**, once for each path, and the light reads no image.
- **A texel is the box it covers.** A place in an image from (0, 0) at its top left to (1, 1) at
  its bottom right reads the texel that holds it, with no filtering between texels.
- **A texture tints the light's colour**: the colour times the texel, before the light's scale.
- **A rect light's texture lies across the rect**, its first corner at the rect's +x and +y, and
  its last at its -x and -y, as UsdLux lays it. A rect with a texture is sampled by area, and its
  power is weighed by the average of its texels' luminance.

## Dome lights

A dome light surrounds the scene, and every ray that leaves the scene meets it. Its emission is
its `intensity` times two to its `exposure`, times its `colour` as an illuminant, tinted by its
image where it has one. `normalize` is not read, as UsdLux says of a dome.

- **Its image is laid out in latitude and longitude**, as OpenEXR lays out an environment and
  UsdLux takes it. Its top is the dome's +y, and its bottom -y. Its middle across faces +z, and a
  quarter of the way in from its left +x. Its `transform` turns it into the world.
- **Its `format` names the layout.** `latlong` reads the image so. `automatic` does too, unless
  the file's envmap attribute names a cube map. Any other layout, or a cube map, is reported, and
  the light emits nothing.
- **A dome with no image** emits its colour from every direction, and is drawn uniformly over the
  sphere.
- **Camera rays see a dome while its `camera` is 1**, which it is unless a client sets it. A dome
  of `camera` 0 still lights the scene and is seen by every other ray.
- **A texel is drawn in proportion to its luminance times the solid angle it covers**: a row by
  the sums of its texels, then a texel along it, then a place within it uniformly. A place covers
  2 pi^2 times the cosine of its latitude for each unit of the image's area, so its density by
  solid angle is its density over the image over that. A way that gives nothing to draw by is
  drawn in proportion to solid angle alone.
- **Multiple importance sampling draws by Karlik and others' compensation.** A second way of
  drawing takes the image's average luminance over the sphere off each texel, held to zero or more.
  Light samples under `mis` draw by it, since BSDF samples find the dim directions it leaves out.
  Light samples alone draw by the first way.
- **Its power** is its emission's luminance, times the integral of its image's luminance over the
  sphere, or four pi with no image, times the area of a disk of the world's reach.

## IES profiles

A light's `iesFile` names an IES file, in the IESNA's LM-63 format, that shapes what it emits by
direction, as UsdLux's `shaping:ies` does. A rect, disk, sphere or cylinder light and a point are
shaped by one. A mesh light's is not read, as its cone is not.

- **`light::parseIes()` reads the file**, of any year of LM-63. The keyword lines
  before `TILT` are skipped, and the tilt `TILT=INCLUDE` gives is skipped too. Numbers may be
  parted by spaces, line breaks or commas.
- **Only type C photometry is read**, the photometry of luminaires. A file of type A or B, one
  whose `TILT` names another file, or one moya cannot read is reported, once for each path, and
  its light is not shaped.
- **Its candela are the file's**, times its candela multiplier, its ballast factor and its
  ballast-lamp photometric factor.
- **A vertical angle is measured from the light's -z axis**, its nadir, towards its +z, and a
  horizontal angle about that axis from its +x towards its +y. A cylinder light's own axes are
  read through the turn that lays its shape along its prototype's z.
- **The horizontal angles are one angle, or span 0 to 90, 0 to 180, 90 to 270 or 0 to 360
  degrees.** A smaller span is mirrored to fill the turn, as type C photometry says.
- **A direction is interpolated linearly** between the angles about it, horizontal and
  vertical. A vertical angle the profile does not reach is dark.
- **`iesAngleScale` scales the vertical angle first**, as UsdLux's `ies:angleScale` does. Above
  zero, the angle is divided by it from the nadir. Below zero, it is divided by its negation
  from the zenith.
- **Without `iesNormalize`, the profile multiplies the light's emission by its candela.** With
  it, the candela are divided by their mean over the sphere first, so the profile shapes the
  light and leaves its power as it was. UsdLux writes that step as a product with the profile's
  power, which would not keep the power.
- **The profile shapes every way a light is found**: the light a light sample brings in, and
  what a BSDF sample that meets the light gathers.

`light::internal::Profiles` reads each file once, and `kernel::light::Lights::shaping()` gives
what a light's profile multiplies its emission by along a direction.

## Meeting a light

`kernel::light::Lights::at()` gives the place among the lights of the light whose shape a hit
met, from the hit's outermost instance, or none for a hit on anything else. `Lights::emitted()`
gives the radiance the light sends back along the ray. `light::fromAfar()` gives what a distant
light with an angle sends along a direction inside its cone, and nothing outside it. A path that
meets a light gathers its share of that radiance times its throughput, as
[Integrator.md](Integrator.md#lights) says. A path that meets a light's shape ends there, since a
light's shape does not scatter.

## Drawing a light

A light sample draws one light. The options' `lightPicking` chooses how: by `tree`, the
fallback, which draws by what each light may bring to the shading point, or by `power`, which
draws in proportion to each light's power alone. Both use the powers below. Each light's cone is
left out of its power. An IES profile weighs it by the profile's mean over the sphere, as
[IES profiles](#ies-profiles) says.

`Lights::pick()` draws a light in proportion to its power, and `Lights::chance()` gives the chance
it was drawn with.

- **A light with a shape** has its radiance's luminance, times its area in the world as it stands
  when the shutter opens, times pi.
- **A point** has its radiant intensity's luminance times four pi.
- **A mesh light** has its radiance's luminance, times the area in the world of the triangles its
  chain places, times pi. `Lights::pickTriangle()` then draws one of its triangles in
  proportion to its area in the world when the shutter opens, and `Lights::triangleChance()`
  gives the chance. `triangles` holds each mesh light's sums of chances from its own first.
- **A volume light** has the power [Volumes.md](Volumes.md#volume-lights) gives.
  `Lights::pickTriangle()` then draws one of its bricks in proportion to its emission, from its
  own first among `triangles`, and `Lights::triangleChance()` gives the chance.
- **A dome light** has the power [Dome lights](#dome-lights) gives.
- **A distant light** has the luminance of the irradiance it gives a plane that faces it, times
  the area of a disk of the world's reach. The reach is the radius of a sphere about the box that
  bounds the world over the shutter. `integrate::render()` gives `light::Lights` the reach.

`cdf` holds the sum of the chances up to and including each light, and is empty when no light has
any power.

## The light tree

`Lights::pickByTree()` draws a light by a tree over the lights, after Conty Estevez and Kulla, with
one random number. `Lights::chanceByTree()` gives the chance it draws a light with, and a
triangle of a mesh light, at a shading point. `light::internal::build()` builds each tree on the
host, in `kernel/light/Tree.h`'s nodes.

- **The top tree holds every light with a place**: those with a shape, the points, the mesh
  lights and the volume lights. A leaf holds one light.
- **Each geometry a mesh light makes emit has a subtree** over its triangles, in the geometry's
  own space. Every mesh light that places the geometry leads into the same subtree from its leaf.
  The walk takes the shading point and its normal into the geometry's space through the light's
  chain, at the time of the path.
- **Each volume's light has a subtree** over its bricks, in the light's own space. A brick is
  bounded by its box and every direction, and its power is the sum of its cells' weights. The walk
  takes the shading point and its normal into the light's own space through its chain and its
  `transform`, `Lights::pickBrick()`, and `Lights::brickChance()` gives the chance. A closed
  surface's light has no subtree, and draws its one brick.
- **Distant and dome lights are drawn beside the tree.** Each, and the tree as a whole, has the
  same chance: one over their count. A scene of no light with a place has no tree, and draws
  among the distant and dome lights alone.
- **A node bounds its lights.** It holds a box that holds them over the shutter, and a cone of
  the directions their faces point along. It holds the angle past that cone they emit to, and
  the sum of their powers. A
  triangle's power is its area in its geometry's space when the shutter opens.
- **A light's cone of directions** is the axis it emits along, turned into the world as a normal
  is. A rect or a disk emits within a quarter turn of it, or within its cone where that is
  narrower. A sphere, a cylinder or a point emits along every direction, or within its cone,
  whose softness is the angle past it. A triangle emits within a quarter turn of its normal. A
  mesh light's leaf in the top tree holds its triangles' normals in the world.
- **What moves is bounded by every direction**: a light whose rotation or scale changes over the
  shutter, a mesh light whose chain turns, and a geometry whose points move. Their boxes hold
  them at every time. A volume light is bounded by every direction wherever it stands.
- **A walk draws each child by its importance at the shading point.** The importance is the
  node's power over the distance squared to its box's centre, times a cosine. That cosine's
  angle is the least between its cone and the direction to the point, less the angle its box
  subtends. It is
  zero where that angle reaches the angle past the cone, as no light of the node emits to the
  point. It is also times the cosine of the least angle between the shading normal, either way,
  and a direction to the box. The distance is held to the box's half diagonal.
- **The random number is rescaled at each step**, within the share of it the branch taken holds,
  so a walk into a subtree draws its triangle by what is left of it.
- **A BSDF sample that meets a light** finds the chance the tree would have drawn it with. It
  walks from the light's leaf up to the root, and computes each step's chance as the walk down
  computes it. A mesh light's leaf of its triangle comes from the hit.
- **A node splits its lights by the surface area and orientation heuristic.** Twelve planes
  cross the box of the lights' centres on each axis, and it splits at the one whose two sides
  cost least. A side costs its power, times the solid angle its cone reaches, times its box's
  area. A split along an axis much shorter than the node's longest costs more. Where no plane parts
  the centres, the lights are split in half in their order.
- **A light of no power has no leaf**, and is never drawn.
- **The tree is built from the scene alone**, on one thread, so every run builds the same tree.
  Each render builds it again, with the lights.

`nodes` holds every tree, and `roots` the root of each set of linked lights' top tree, as
[Light linking](#light-linking) says. `leaves` holds each light's leaf in each set, and
`triangleLeaves` each triangle's, from the mesh light's `leaves`. `infinite` holds the distant
and dome lights of each set, from `infiniteFirst`.

Background: [ADR-0214](../adr/offline/0214-offline-the-light-tree-has-a-subtree-per-emitting-prototype.md)

## Light linking

A light lights every instance while its `includeRoot` is 1. When it is 0, it lights only what a
chain of instances places through an instance its `instances` names, at any depth of the chain.
Camera rays see every light, whatever it lights.

- **A shading point lies in a set of linked lights**, which its chain of instances decides. Set 0
  holds every light whose `includeRoot` is 1. A chain that reaches an instance a linked light
  names moves on into a set that also holds that light.
- **`light::internal::linking()` finds every set** by walking the chains from the world once,
  and records each step from one set to another at the instance that makes it. A group reached
  in the same set at the same depth is walked once.
- **`kernel::light::link::Links::setOf()`** follows those steps along a hit's chain, and
  `Links::lights()` says whether a light lights a set.
- **Each set has a top tree of its own**, over the lights with a place that light it, and a list
  of the distant and dome lights that do. The mesh lights' subtrees are shared by every set.
- **A light sample draws from the shading point's set.** Drawn in proportion to power, a light
  that does not light the set gathers nothing.
- **A BSDF sample that meets a light** gathers nothing from a light that does not light the set
  of the shading point the sample left. The path keeps that set for it.

A light's `shadowIncludeRoot` and `shadowInstances` say what casts its shadows, as UsdLux's
shadow linking does. While `shadowIncludeRoot` is 1, every instance casts them. When it is 0,
only what a chain places through an instance `shadowInstances` names does.

- **A shadow ray from such a light passes through what casts none of its shadows.** It goes on
  from just past each point it meets, until it meets what casts its shadow or nothing. A surface
  that casts its shadow and is cut out lets one minus its opacity through, and the ray goes on
  past it too. `kernel::integrate::kMostPassed` is the most surfaces it meets, past which it is
  blocked.
- **Such a light is found by light samples alone**, as a point is, and its light sample takes
  the whole weight. A BSDF's direction towards it would stop at what the shadow ray passes
  through.
- **`Links::casts()`** says whether a hit's chain casts a light's shadows, from the light's
  sorted list among `casters`.

## Sampling a light

`light::sample()`, in `kernel/light/Sampling.h`, draws a point on a light's shape from a shading
point, with its density by solid angle. `light::density()` gives the density with which it draws a
given point, and is zero for a point it never draws. The two always agree, since each decides the
method from the same light, transformation and point. `light::arriving()` gives what the light
sends to the shading point from the point drawn.

- **A point is drawn where it stands**, and a distant light of no angle along its direction, each
  with a density of one. Neither has a density for a direction a BSDF draws, since no such
  direction meets it, so `density()` is zero for both.
- **A distant light with an angle is drawn uniformly in its cone.** Its cosine to the light's
  direction is uniform from the cone's edge to one, and its sine is found from one less the
  cosine, so a cone as narrow as the sun's keeps its precision. One less the cosine of the half
  angle is twice the sine squared of its half, for the same reason. A distant light's point lies
  a unit along the direction drawn.

- **A rect light whose edges stay at right angles in the world** is sampled uniformly in the solid
  angle it subtends, by Ure�a, Fajardo and King's spherical rectangle.
- **A disk light that stays round** is sampled uniformly in solid angle by Guill�n and others'
  spherical ellipse, through its radial map. The directions to the disk form an elliptic cone.
  The cone's axis bisects the directions to the two points of the rim in the plane of the disk's
  normal and the direction to its centre. Its semi-axes are the tangents of its half angles across
  that plane and in it.
- **The spherical ellipse's solid angle** comes from Legendre's elliptic integral of the third
  kind, through Carlson's symmetric integrals R_F and R_J. A sample's azimuth inverts the solid
  angle swept from the major axis by Newton's method, held within a bracket that halves when a
  step leaves it. Its cosine to the axis is uniform between the cone's edge and one.
- **A sphere light that stays round** is sampled uniformly in the cone it subtends from a point
  outside it, as pbrt-v4 samples it. A narrow cone is sampled by its small-angle expansion.
- **Anything else is sampled uniformly by its area**, and its density converted to solid angle.
  That is a cylinder light, a point inside a sphere light, and a rect, disk or sphere that its
  transformation shears or stretches. A rect or disk that subtends less than `kSmallestSolidAngle`
  is sampled by area too, since its solid angle loses precision there.
- **A rect or disk light draws nothing for a point behind its emitting face**, and its density
  there is zero.
- **A mesh light's triangle is sampled uniformly in the solid angle it subtends**, by Arvo's
  method as pbrt-v4 gives it, in `kernel/light/spherical/SphericalTriangle.h`. Its solid angle
  is Van Oosterom and Strackee's. Below `kLeastTriangleAngle` its solid angle loses precision,
  and above `kMostTriangleAngle` the method does, so it is sampled uniformly by area there. A
  triangle draws nothing for a point behind it.
- **A volume light's point is drawn in the brick drawn**, `media::glowingSample()`, in
  `kernel/integrate/media/Glowing.h`. A volume's brick draws a cell by its chance, from the first
  of the light sample's point numbers. It draws a point uniformly in the cell from the second and
  from two of a hash, `collisionDraw()` at `kGlowDraws`. A closed surface's brick draws its point
  uniformly in its box. The density, `media::glowingDensity()`, is the point's chance by the
  volume of the light's own space, over the volume the world gives a unit of that space, times its
  distance squared from the shading point. That is a density by solid angle and by length along
  the direction, the measure in which a path that tracks through the medium finds the same point.
  `media::glowingEmission()` gives the medium's emission there.
