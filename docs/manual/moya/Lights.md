# Light a scene

A light is created with `LightSource` or `Light` inside the world block. moya reads RI's four
standard lights, RI's area light, and RenderMan's lights. A surface lit by no light is black,
except where its material glows, as `constant` does.

Every light but the ambient light casts shadows. moya traces a ray from each shaded point toward
a light to find whether anything blocks it, so a shadow needs no setting of its own.

## Create a light

```
LightSource "pointlight" 2 "from" [-1 1.5 3] "intensity" [3]
```

- The first argument names the light.
- The second is the light's handle: a number, or a string such as `"key"`. `Illuminate` uses it.
- The rest are the light's parameters, each a name and a value in brackets.

A light is placed by the transformation current when it is created. Its `from` and `to` points
are in that space, so a `Translate` before `LightSource` moves the light.

A light shines on every object created after it while it is on. A light created inside an
`AttributeBegin` block is switched off at the block's `AttributeEnd`.

## RI's lights

Each of RI's lights takes `intensity` (default 1) and `lightcolor` (default `[1 1 1]`).

| Light | Further parameters | What it does |
|---|---|---|
| `ambientlight` | None | Lights every point evenly from every direction. Casts no shadow |
| `distantlight` | `from` `[0 0 0]`, `to` `[0 0 1]` | Shines along the direction from `from` to `to`, like the sun. Its light does not fall off |
| `pointlight` | `from` `[0 0 0]` | Shines in every direction from `from`. Its light falls off with the square of the distance |
| `spotlight` | `from` `[0 0 0]`, `to` `[0 0 1]`, `coneangle` 0.5236, `conedeltaangle` 0.0873 | Shines from `from` toward `to`, inside a cone. Falls off with the square of the distance |

- `coneangle` is the cone's half angle in radians, 30 degrees by default.
- `conedeltaangle` is the width, in radians, of the soft edge inside the cone, 5 degrees by
  default.
- `beamdistribution` is not read. A spotlight is equally bright across its cone, up to the soft
  edge.
- A point light, a spotlight and a distant light each shine from a single point or direction.
  Their shadows have hard edges.
- Any other light name is logged, and makes no light.

A scene with these lights:

```
Format 320 240 1
PixelSamples 8 8
Projection "perspective" "fov" [50]
Clipping 0.1 100
WorldBegin
LightSource "ambientlight" 1 "intensity" [0.1]
LightSource "pointlight" 2 "from" [-1 1.5 3] "intensity" [3] "lightcolor" [1 0.9 0.8]
LightSource "spotlight" 3 "from" [2 2 2] "to" [0.5 -1 5] "intensity" [8]
    "coneangle" [0.4] "conedeltaangle" [0.1]
Surface "matte"
Color [0.8 0.8 0.8]
Polygon "P" [-3 -1 2  3 -1 2  3 -1 9  -3 -1 9]
Color [0.3 0.6 0.3]
AttributeBegin
Translate 0 -0.5 5
Sphere 0.5 -0.5 0.5 360
AttributeEnd
WorldEnd
```

### The ambient light

`ambientlight` surrounds the scene, and lights each point by the light's colour times its
intensity, from every direction. It casts no shadow, and the camera does not see it. A white
`matte` surface under an ambient light of intensity 0.2 alone shows a grey of 0.2.

The ambient light is found only by aiming a ray at it. A shiny surface, such as `metal` or
`plastic`, reflects it in a few bright pixels at first, and needs many samples to smooth them.
For a sky that a shiny surface reflects smoothly, use RenderMan's `PxrDomeLight`, which casts
shadows.

## Switch a light on or off

`Illuminate` switches a light on or off for the objects created after it:

```
Illuminate 2 0
```

The first argument is the handle, and the second is 1 for on or 0 for off. Inside an
`AttributeBegin` block, the change lasts until the block ends. A light lights the objects
created while it was on, and no others. Every object casts its shadows from every light.

## Area lights

An area light shines from the surface of its own geometry, so its shadows have soft edges.
Create it with `AreaLightSource` instead of `LightSource`. Each object that follows it in the
same attribute block glows:

```
AttributeBegin
AreaLightSource "arealight" 2 "intensity" [4]
Polygon "P" [-1 1.5 4  1 1.5 4  1 1.5 6  -1 1.5 6]
AttributeEnd
```

- `arealight` takes `intensity` (default 1) and `lightcolor` (default `[1 1 1]`). Any other area
  light name is logged, and makes no light.
- **A polygon, a mesh or a subdivision surface glows.** A sphere, another quadric, a curve or a
  point makes no light, and the log says so.
- **The geometry glows from its front**, the side its winding faces.
- **Only the objects inside the same block glow.** The block's `AttributeEnd` ends the area
  light.
- **The camera does not see the light itself.** It sees the geometry with its own material. Give
  it `Surface "constant"` and a `Color` to draw it as a bright panel. A `constant` surface glows
  too, so its light adds to the area light's.
- **An area light lights every object**, and `Illuminate` does not switch it.

## RenderMan's lights

The `Light` request makes one of RenderMan's lights:

```
AttributeBegin
Translate 0 3 5
Rotate 90 1 0 0
Light "PxrRectLight" "key" "float intensity" [10] "color lightColor" [1 0.95 0.9]
AttributeEnd
```

| Light | Shape |
|---|---|
| `PxrRectLight` | A square, one unit on a side, facing +z |
| `PxrDiskLight` | A disk of radius 0.5, facing +z |
| `PxrSphereLight` | A sphere of radius 0.5 |
| `PxrCylinderLight` | A cylinder of radius 0.5 and length 1 along x |
| `PxrDistantLight` | None. Shines along +z from beyond the scene |
| `PxrDomeLight` | None. Surrounds the scene, with its pole along +z |
| `PxrMeshLight` | The objects that follow it in the attribute block, as `AreaLightSource` makes them |

- **The transformation sizes and places the light.** `Scale 4 2 1` makes a rect light four units
  wide and two high.
- **Every light takes** `intensity`, `exposure`, `lightColor`, and `enableTemperature` with
  `temperature`, a colour temperature in kelvin. The light's brightness is `intensity` times 2 to
  the power of `exposure`.
- **Every light but the dome takes `areaNormalize`.** At 1, the light gives the same power at any
  size.
- **A rect, disk, sphere or cylinder light takes** `coneAngle` and `coneSoftness`, in degrees and
  from 0 to 1, and `iesProfile`, `iesProfileScale` and `iesProfileNormalize` to shape it by an IES
  file.
- **A rect or dome light takes `lightColorMap`**, an EXR image that tints it. A dome's image is
  laid out in latitude and longitude.
- **`PxrDistantLight` takes `angleExtent`**, the angle across the sun in degrees, which softens
  its shadows. Its intensity is 50000 unless the request gives one.
- **`PxrMeshLight` takes `textureColor`**, a colour that tints its `lightColor`.
- **Give each parameter its type**, as in `"float exposure"`. Of these names, only `intensity`
  has a type already.
- **The camera sees a dome light**, behind everything the scene does not cover. It does not see
  any other of these lights' shapes.
- `Illuminate` and `AttributeEnd` switch these lights off as they do RI's. Any other light name
  is logged, and makes no light.

## Light groups

A light's `"string lightGroup"` puts it in a named group, on `LightSource` and on `Light`. A
channel of the output can then hold the light of one group alone.
[Output.md](Output.md#add-channels) describes channels.

## Shadows

- **An object's opacity sets how much light its shadow lets through.** `Opacity [0.3 0.3 0.3]`
  lets 70 percent through.
- **`glass` casts a shadow** as an opaque surface does. Light reaches what lies behind it only by
  a path that passes through it.
- **A distant light's shadows reach any distance.**
- **An area light's shadows are soft**, and their noise falls as `PixelSamples` rises.
