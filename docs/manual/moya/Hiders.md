# Choose a hider

The hider is the part of moya that works out which surface the camera sees at each point of
the picture. moya has two. [HowMoyaRenders.md](HowMoyaRenders.md) explains how each one works.

## Name the hider

Put a `Hider` request before `WorldBegin`:

```
Hider "raytrace"
```

| Name | Hider |
|---|---|
| `"hidden"` | The reyes hider. It is the default, so a scene with no `Hider` request uses it |
| `"raytrace"` | The ray-trace hider |

moya reads no parameters of `Hider`. Any other name is logged as a warning, and the hider stays
as it was.

## What each hider draws

| | Reyes (`"hidden"`) | Ray trace (`"raytrace"`) |
|---|---|---|
| `Polygon`, `PointsPolygons` | Drawn | Drawn |
| `Curves` | Drawn as ribbons that face the camera | Drawn as ribbons that face each ray |
| `Sphere` | Not drawn. The log says so once | Drawn exactly, at any size |
| Translucent surfaces (`Opacity` below 1) | Composited front to back | Composited front to back |
| Shadows, reflections and refraction | Traced, when a shader asks for them | Traced |
| A moving object's shadow | Sharp: shadows are cast at the shutter's open time | Blurred with the object |
| `ShadingRate`, `--grid`, `--bucket` | Used | Not used |
| `PixelVariance` | Not used | Adds samples where a pixel is noisy |

Both hiders read the same scene and the same camera, run the same shaders, and filter their
samples the same way.

## Which to choose

- **Choose the ray-trace hider** for a scene with spheres, or for a scene where most surfaces
  reflect or refract. It is also the only hider that adapts its sample count to noise, through
  `PixelVariance` ([Quality.md](Quality.md)).
- **Choose the reyes hider** for a scene of polygons and curves with a displacement shader. It
  dices each surface into a grid as fine as the shading rate asks for. The ray-trace hider
  dices a displaced polygon into a fixed grid of 64 by 64, so it loses detail finer than that.

A scene moved from one hider to the other can change its noise, because only the ray-trace
hider adapts its sample count.

## Shadows under the reyes hider

The reyes hider casts no rays of its own to find what the camera sees. A shadow, a reflection or
a refraction still comes from a ray, which a shader casts with `transmission()` or `trace()`.
Every built-in light that has a direction calls `transmission()`, so it casts shadows under
either hider. [Lights.md](Lights.md#shadows) has more on shadows.
