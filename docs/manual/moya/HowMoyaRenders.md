# How moya renders

This page explains how a RIB file becomes an image, at the level that helps you set up a scene
and understand its settings. It does not describe the code.

```
scene.rib -> scene client -> scene -> path tracer -> film -> outputs -> image files
```

## Terms

- **Scene client**: the part of moya that reads each RIB request and changes the scene to match.
- **Scene**: what moya renders. It holds the camera, the options, every object, material and
  light, and the outputs, each with its parameters. moya keeps it whole until a frame ends.
- **Graphics state**: the colour, opacity, material, lights and transformation in force while
  the file is read. Each object takes the state current when it is given.
- **Primitive**: one piece of geometry, such as a polygon, a sphere, a subdivision surface or a
  set of curves.
- **Instance**: one placement of a primitive in the world, with its transformation and its
  material.
- **Material**: how a surface reflects, transmits and emits light. [Materials.md](Materials.md)
  describes them.
- **Primvar**: a value a primitive carries at each vertex, face or corner, such as `"st"` or
  `"Cs"`. A material can read it.
- **Path**: the route a ray of light takes from the camera, bouncing off surfaces, until it
  leaves the scene or stops.
- **Sample**: one path through one pixel. A sample has its own point in the pixel, a time within
  the shutter for motion blur, and a point on the lens for depth of field.
- **Pass**: a round of up to 16 samples in every pixel.
- **Film**: the picture moya adds samples to. Each pixel holds the sum of its samples and how
  much each counts.
- **Pixel filter**: the weighting that sets how much a sample counts toward its pixel.
- **Output**: a file moya writes from the film, such as a PNG or an EXR file.
- **Channel**: a picture an EXR output holds beside the beauty, such as depth or the light of one
  group.
- **Beauty**: the finished picture, with its alpha.

## From file to image

1. **The scene client reads the file, one request at a time.** The options before `WorldBegin`
   set up the picture and the camera. Each request inside the world changes the graphics state or
   adds an object to the scene.
2. **At `WorldBegin` the camera is placed** by the transformation in force. The scene of any
   earlier world is cleared.
3. **Each primitive becomes a primitive and an instance in the scene.** The instance takes the
   material the graphics state names. Changing the colour after a polygon does not change that
   polygon.
4. **At `WorldEnd` the scene is complete, and the path tracer renders it.** moya cuts each
   subdivision surface into triangles, displaces it, and builds a structure that lets a ray find
   the surface it meets quickly.
5. **The path tracer adds samples to the film, a pass at a time**, until every pixel has its
   samples or has stopped early.
6. **The outputs are written** from the film, with the background laid behind each, if the scene
   names one.

A file can hold several frames, each between `FrameBegin` and `FrameEnd`. moya renders and writes
each frame at its `WorldEnd`.

## A path

A sample starts at the camera. Its ray goes through the sample's point in the pixel, from its
point on the lens, at its time.

- **The ray meets the nearest surface.** The material there says how the surface scatters light.
- **moya picks one light and a point on it**, and casts a shadow ray toward it. If nothing blocks
  the shadow ray, the light it brings is added, weighted by the material.
- **The path bounces.** moya picks a new direction from the material, and the ray goes on from
  the surface. A path that meets a light or a glowing surface adds that light too.
- **Light found both ways is weighted** so that each way counts most where it finds light best.
  A small, bright light is found best by the shadow ray, and a large, dim one by the bounce. The
  method is called multiple importance sampling.
- **The path ends** when it leaves the scene, reaches its limit of bounces, or is ended at random
  once it carries little light. A path that leaves the scene adds what any dome light or distant
  light gives it there.

A path carries a few wavelengths of light, not red, green and blue. moya turns them into the
picture's colour as it adds the sample to the film. Colours in the scene, such as `Color`,
become spectra first.

## Grain and how it falls

Each sample is a guess at the pixel's light, and the mean of many guesses is the pixel. The
grain is how far the mean of the samples taken so far lies from the true value.

- The grain falls with the square root of the number of samples. Four times the samples halves
  it.
- A scene of small lights, glossy surfaces or light that bounces many times is grainier than one
  lit by a broad sky.
- The random numbers depend only on the pixel and the sample, so a render is the same on every
  run. An animation shows the same grain pattern in every frame where nothing moves.
- [Quality.md](Quality.md) lists the settings that trade time for grain: samples, adaptive
  sampling, bounces and denoising.

## Sampling and filtering

- Each sample lands at a point in its pixel drawn by the pixel filter, and counts toward that
  pixel alone.
- A wider filter gives a softer picture.
- A sample covers its pixel when its camera ray meets something. The pixel's alpha is the share
  of its samples that cover it.
