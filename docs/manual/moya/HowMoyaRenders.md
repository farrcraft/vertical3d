# How moya renders

This page explains how a RIB file becomes an image, at the level that helps you set up a scene
and understand its settings. It does not describe the code.

## Terms

- **Primitive**: one piece of geometry, such as a polygon, a sphere or one curve.
- **Graphics state**: the colour, opacity, shaders, lights and transformation in force while the
  file is read. Each primitive takes the state current when it is given.
- **Hider**: the part of moya that works out which surface the camera sees at each point of the
  picture. moya has two, the reyes hider and the ray-trace hider.
- **Sample**: one point inside a pixel where the hider finds what is seen. A sample also has a
  time within the shutter, for motion blur, and a point on the lens, for depth of field.
- **Pixel filter**: the weighting that blends the samples near a pixel into its colour.
- **Shading point**: a point where moya runs the shaders to find a colour.
- **Micropolygon**: a tiny polygon, about a pixel across or smaller, that the reyes hider cuts a
  surface into.
- **Grid**: a regular mesh of micropolygons cut from one primitive. The reyes hider shades a
  whole grid at once.
- **Bucket**: a rectangle of pixels, 16 by 16 by default. The reyes hider sorts primitives into
  buckets.
- **Shading rate**: the area of one micropolygon, in pixels.
- **Imager**: a shader run over the finished picture.

## From file to image

1. **moya reads the file, one request at a time.** The options before `WorldBegin` set up the
   picture and the camera. Each request inside the world changes the graphics state or adds a
   primitive.
2. **At `WorldBegin` the camera is fixed.** Every camera request takes effect then, in whatever
   order the file gave them.
3. **Each primitive is stored with its graphics state.** Changing the colour after a polygon
   does not change that polygon.
4. **At `WorldEnd` the hider renders.** It finds what each sample sees and runs the shaders there.
5. **The pixel filter blends the samples into pixels.**
6. **The imager runs**, if the scene names one, over each row of pixels.
7. **moya writes the image file**, if one is named.

## The reyes hider

The reyes hider cuts every surface into micropolygons and shades them before it decides what
is visible.

- **Bucketing.** As each primitive arrives, the hider works out which part of the picture it
  covers, and files it in the bucket at that part's upper left corner. A primitive outside the
  camera's view, or outside the clipping range, is dropped here.
- **Splitting.** A primitive too large to dice is split into smaller pieces, which are bucketed
  again. The shading rate and the grid size set how large is too large.
- **Dicing.** A primitive small enough is cut into one grid of micropolygons.
- **Displacement and shading.** The displacement shader moves the grid's points. The surface
  shader then runs once over all the grid's points together.
- **Hiding.** Each micropolygon is tested against the samples it covers. A sample keeps every
  surface in front of its nearest opaque one, and composites them front to back.

Shading happens before hiding. The shaders run on surfaces that may later turn out to be hidden,
and their cost depends on the shading rate, not on the number of samples. Raising
`PixelSamples` smooths edges and blur for little extra shading. Lowering `ShadingRate` shades
more points.

The reyes hider casts no rays to find what the camera sees. When a shader asks for a shadow or a
reflection, the ray is traced through the same scene the ray-trace hider uses.

## The ray-trace hider

The ray-trace hider casts a ray from the camera through each sample. It finds the nearest surface
the ray meets and shades that point. If the surface is translucent, the ray goes on through it,
and moya composites what it passes through front to back.

- It shades only points the camera sees, once for each sample that sees them.
- It meets spheres exactly, and curves as ribbons, without cutting either into micropolygons.
- It can add samples where a pixel is noisy, through `PixelVariance`.
- It dices a displaced polygon into a fixed grid, 64 by 64, before it traces it.

## Shadows and traced rays

Both hiders answer a shader's `trace()` and `transmission()` from one store of the scene's
geometry.

- `transmission()` casts a shadow ray. Each object it passes lets through one minus its opacity.
  It does not run that object's shader.
- `trace()` casts a ray and shades what it meets. A traced ray can trace again, up to the depth
  `Option "trace" "maxdepth"` sets. Past it, the ray returns the background.

## Sampling and filtering

- moya takes `PixelSamples` samples in each pixel, each at a random place in its own cell of a
  grid over the pixel.
- The random places depend only on the pixel's position. The same scene renders to the same
  image on every run. An animation shows the same noise pattern in every frame.
- The pixel filter blends each sample into every pixel whose centre is near enough. A wider
  filter gives a softer picture.
- Each sample counts toward a pixel's coverage when it meets a surface. The `background` imager
  fills the part of each pixel the scene left uncovered.
