# Control quality and speed

moya renders by path tracing. Each sample of a pixel follows one path of light through the
scene, and the pixel is the mean of its samples. A few samples give a grainy picture, and more
samples give a smoother one. Each setting here trades render time for less grain. Set them
before `WorldBegin`.

A render is the same on every run. moya draws each sample's random numbers from the pixel and the
sample alone, so the grain does not change from one run to the next, or with the number of
threads.

## Pixel samples

```
PixelSamples 8 8
```

- `PixelSamples x y` takes x times y samples in each pixel. moya spreads them over the pixel
  in its own pattern, so only the product counts.
- The count is rounded up to a power of two. `PixelSamples 6 6` takes 64 samples.
- The default is 2 by 2, which is grainy. 64 samples suit a test, and a few hundred a finished
  picture of soft light.
- Each axis is from 1 to 256. A count below 1 is taken as 1, and one above 256 as 256.
- The time grows with the number of samples. Doubling the samples takes twice as long, and
  reduces the grain by about a third.
- More samples smooth edges, soft shadows, depth of field, motion blur and glossy reflections
  together.

## Adaptive sampling

```
PixelSamples 32 32
PixelVariance 0.02
```

`PixelVariance` stops sampling each pixel once its grain falls below the value. `PixelSamples`
is then the most samples a pixel takes. A smooth wall stops early, and a soft shadow keeps
sampling.

- moya measures a pixel's grain from two halves of its samples. It compares their means, and
  scales the difference down for a bright pixel.
- The first check comes once a pixel holds 16 samples, and another after every 16 more.
- A pixel stops only when its four neighbours are also below the value.
- A smaller value takes more samples. 0 is the default, and samples every pixel to the most.
- The render ends early when every pixel has stopped.

## Pixel filter

```
PixelFilter "gaussian" 2 2
```

The filter sets how much each sample counts toward its pixel, by where in the pixel it falls.
The two numbers are its width and height in pixels.

| Filter | Result |
|---|---|
| `"box"` | Every sample counts equally. `"box" 1 1` keeps each pixel's samples inside it |
| `"triangle"` | Samples nearer the pixel's centre count more |
| `"gaussian"` | The default, 2 by 2. Soft, with no ringing |
| `"catmull-rom"` | Sharper than gaussian. It can ring: a thin dark or bright edge beside a sharp change |
| `"sinc"` | Sharp, and rings the most |

A filter name moya does not recognise is reported, and the filter stays as it was. A width that is
not above zero is skipped with a warning.

## Bounces

```
Option "trace" "maxdepth" [6]
```

A path bounces from surface to surface until it leaves the scene or reaches a limit.
`maxdepth` sets the most bounces in all. The default is 12.

- Fewer bounces render faster. A scene loses light where paths stop early: the inside of glass
  and the light between mirrors first.
- moya also stops a path after 4 diffuse bounces and 4 glossy ones. `maxdepth` does not change
  those limits.
- From the third bounce on, moya ends a path at random once it carries little light. A path
  that survives counts for more, so the picture stays as bright on average.
- A depth below zero is logged and not used.

## Denoising

```
Option "moya" "int denoise" [1]
```

moya can remove the grain from the picture after its last pass, through Intel's Open Image
Denoise. `--denoise` on the command line does the same.

- The denoised picture is the channel `beauty_denoised` of each EXR file, beside the grainy
  picture. Open it in a compositor or an EXR viewer.
- A PNG holds the grainy picture alone. Write an EXR file to keep the denoised one.
- moya guides the denoiser by each pixel's surface colour and normal, so edges and textures stay
  sharp.
- A denoised picture smooths fine detail that too few samples left unclear. Denoise a render
  that is nearly smooth already, such as one of 64 samples or more.

## Checkpoints

A long render can write a checkpoint after each pass of 16 samples, and continue from it after it
is stopped:

```
moya.exe --file scene.rib --checkpoint scene.ckpt.exr --checkpoint-interval 60
moya.exe --file scene.rib --checkpoint scene.ckpt.exr --resume
```

- `--checkpoint-interval` writes it at most once a minute here, and always after the last pass.
- `--resume` gives the same picture as a render that never stopped.
- A checkpoint of another scene, or of the same scene changed, is refused.
- [CommandLine.md](CommandLine.md#options) lists the rules.

## Render faster while you work

- Render smaller with `--width` and `--height`, or a part of the picture with `CropWindow`
  ([Output.md](Output.md)).
- Use `PixelSamples 4 4`, or `PixelVariance`, until the scene is right.
- Lower `maxdepth` for a scene of diffuse surfaces.
- Use a released moya, or a Release build. A Debug build is slower.
