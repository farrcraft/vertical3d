# Light a scene

A light is a light shader, created with `LightSource` inside the world block. moya has five
built-in light shaders, and you can write your own ([Shaders.md](Shaders.md)). A surface shader
such as `matte` or `plastic` adds up the light that reaches each point. A surface with no lights
on it is black, except under the `constant` shader, which ignores lights.

## Create a light

```
LightSource "pointlight" 2 "from" [-1 1.5 3] "intensity" [3]
```

- The first argument names the light shader.
- The second is the light's handle: a number, or a string such as `"key"`. `Illuminate` uses it.
- The rest are the shader's parameters, each a name and a value in brackets.

A light is placed by the transformation current when it is created. Its `from` and `to` points
are in that space, so a `Translate` before `LightSource` moves the light.

A light shines on every surface created after it, until it is switched off. A light created
inside an `AttributeBegin` block is switched off at the block's `AttributeEnd`.

## The built-in lights

Every built-in light takes `intensity` (default 1) and `lightcolor` (default `[1 1 1]`).

| Light | Further parameters | What it does |
|---|---|---|
| `ambientlight` | None | Lights every point evenly from no direction. Casts no shadow |
| `distantlight` | `from` `[0 0 0]`, `to` `[0 0 1]` | Shines along the direction from `from` to `to`, like the sun. Its light does not fall off |
| `pointlight` | `from` `[0 0 0]` | Shines in every direction from `from`. Its light falls off with the square of the distance |
| `spotlight` | `from` `[0 0 0]`, `to` `[0 0 1]`, `coneangle` 0.5236, `conedeltaangle` 0.0698, `beamdistribution` 2 | Shines from `from` toward `to`, inside a cone. Falls off with the square of the distance |
| `arealight` | None | The shader for an area light. See [Area lights](#area-lights) |

- `coneangle` is the cone's half angle in radians, 30 degrees by default.
- `conedeltaangle` is the width, in radians, of the soft edge inside the cone.
- `beamdistribution` sets how fast a spotlight dims away from its axis. A larger value gives
  a narrower bright centre.

A scene with these lights:

```
Hider "raytrace"
Format 160 120 1
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

## Switch a light on or off

`Illuminate` switches a light on or off for the surfaces created after it:

```
Illuminate 2 0
```

The first argument is the handle, and the second is 1 for on or 0 for off. Inside an
`AttributeBegin` block, the change lasts until the block ends.

## Area lights

An area light shines from the surface of its own geometry, so its shadows have soft edges.
Create it with `AreaLightSource` instead of `LightSource`. The polygons and spheres that follow it
in the same attribute block become its geometry:

```
AttributeBegin
Attribute "light" "integer nsamples" [16]
AreaLightSource "arealight" 2 "intensity" [4]
Surface "constant"
Polygon "P" [-1 1.5 4  1 1.5 4  1 1.5 6  -1 1.5 6]
AttributeEnd
Illuminate 2 1
```

- The geometry is also drawn as an ordinary surface. Give it `Surface "constant"` and a light
  colour to draw it as a lit panel.
- Only the polygons and spheres inside the same block join the light. Geometry after the block
  does not. A curve never joins a light.
- A sphere joins the light whole, even under the reyes hider, which does not draw it.
- The block's `AttributeEnd` switches the light off, as it does every light made inside a block.
  `Illuminate` with the light's handle switches it on again for the rest of the scene.
- `Attribute "light" "integer nsamples"` sets how many points moya picks on the light for each
  point it shades. More samples give smoother shadows and take longer. The count changes the
  noise, not the brightness. It is from 1 to 4096, and 16 if you do not set it. It must
  come before `AreaLightSource`.
- The `arealight` shader shines from each sample like a point light. Its light falls off with
  the square of the distance.

## Shadows

Every built-in light with a direction (`distantlight`, `pointlight`, `spotlight` and
`arealight`) casts shadows, under either hider. The ambient light casts none.

- **An object's opacity sets how much light its shadow lets through.** `Opacity [0.3 0.3 0.3]`
  lets 70 percent through. moya reads the opacity the scene gave the object and does not run its
  shader to find its shadow.
- **A distant light's shadow ray has an end.** Its length is 100000 times the distance from
  `from` to `to`. An object further than that from a point casts no shadow on it.
- **A moving object's shadow is sharp under the reyes hider.** The reyes hider casts its shadow
  rays at the time the shutter opens.

To light a scene without shadows, write a light shader that does not call `transmission()`.
[ShadingLanguage.md](ShadingLanguage.md#lighting) describes the light shader constructs.
