# Control quality and speed

Each setting here trades render time for a smoother picture. Set them before `WorldBegin`,
except `ShadingRate` and the light samples, which belong to each object.

A render is the same on every run. moya places each pixel's samples by the pixel's position,
so the noise in a picture does not change from one run to the next.

## Pixel samples

```
PixelSamples 4 4
```

- moya decides what is seen at several points in each pixel, called samples, and blends them
  into the pixel's colour. `PixelSamples x y` takes x by y samples per pixel.
- The default is 2 by 2. A scene that names none is smoothed at its edges, and takes four times
  as long as one sample per pixel.
- More samples smooth edges, depth of field, motion blur and soft shadows. The time grows with
  the number of samples.
- A count below 1 is taken as 1, and one above 256 as 256.
- Each sample is placed at random inside its own cell of an x by y grid over the pixel. With one
  sample along an axis, it is at the pixel's centre.

## Pixel filter

```
PixelFilter "gaussian" 2 2
```

The filter sets how much each sample counts toward the pixels near it. The two numbers are its
width and height in pixels. A sample counts toward every pixel whose centre is within half the
width of it.

| Filter | Result |
|---|---|
| `"box"` | Every sample inside counts equally. `"box" 1 1` with `PixelSamples 1 1` gives one sample at each pixel centre, with no smoothing |
| `"triangle"` | Samples nearer the pixel's centre count more |
| `"gaussian"` | The default, 2 by 2. Soft, with no ringing |
| `"catmull-rom"` | Sharper than gaussian. It can ring: a thin dark or bright edge beside a sharp change |
| `"sinc"` | Sharp, and rings the most |

A filter name moya does not recognise is reported, and the filter stays as it was. A width that is
not above zero is skipped with a warning.

## Adaptive samples (ray-trace hider)

```
PixelVariance 0.01
```

Under the ray-trace hider, `PixelVariance` adds samples only where a pixel is noisy. moya takes
another set of `PixelSamples` samples wherever the variance of the pixel's mean, in its worst
channel, is above the value. It stops at four times the first set. A smaller value adds samples
in more places. Zero, the default, turns it off.

The reyes hider ignores `PixelVariance`, and always takes `PixelSamples` samples.

## Shading rate (reyes)

```
ShadingRate 0.5
```

The reyes hider cuts each surface into tiny polygons, called micropolygons, and runs the shaders
once at each of their corners. `ShadingRate` is the area of one micropolygon, in pixels.

- The default is 1, about one micropolygon per pixel.
- A smaller rate shades more points. It shows finer detail in a shader's pattern or a
  displacement, and takes longer.
- A larger rate, such as 4, renders faster and blurs fine detail.
- It belongs to each object, like a colour, so one object can be shaded more finely than the
  rest.

The ray-trace hider shades the points its rays hit, and ignores `ShadingRate`.

## Grid and bucket size (reyes)

```
Option "limits" "bucketsize" [32 32] "gridsize" [64]
```

- `gridsize` is the number of micropolygons the reyes hider shades together, 256 by default.
- `bucketsize` is the size in pixels of the rectangles the reyes hider renders one at a time,
  16 by 16 by default.
- `--grid` and `--bucket` on the command line set the same sizes, and win over the scene's
  `Option "limits"`. `--bucket` sets a square bucket.
- Each size is from 1 to 65536. Any other value is reported, and the size stays as it was.
- A smaller grid splits surfaces into more, smaller pieces before it dices them.

## Trace depth

```
Option "trace" "maxdepth" [3]
```

A reflection can show another reflection, and so on. `maxdepth` sets how many times a ray can
bounce before it returns the background. The default is 2.

- A depth above 16 is taken as 16, with a warning.
- A depth that is negative or not a number is not used, with a warning.
- A fraction is rounded down.

## Area light samples

`Attribute "light" "integer nsamples"` sets how many points of an area light are sampled for
each shaded point. More samples give smoother soft shadows. [Lights.md](Lights.md#area-lights)
describes it.

## Render faster while you work

- Render smaller with `--width` and `--height`, or a part of the picture with `CropWindow`
  ([Output.md](Output.md)).
- Use `PixelSamples 1 1` until the scene is right.
- Use a Release build. A Debug build is slower.
