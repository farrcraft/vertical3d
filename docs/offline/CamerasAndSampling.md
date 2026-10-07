# Cameras and sampling

Where the image's pixels are, how each pixel is sampled and filtered, and the camera effects
that sampling makes possible: depth of field, motion blur and adaptive sampling.

## Framebuffers and raster space

- **`offline::FrameBuffer` is a stack of float planes**, each the size of the image.
  `image(channels)` takes the leading planes as the picture and ignores the rest. The plane
  count is not the channel count.
- **moya's planes are red, green, blue, depth and coverage**, named by
  `moya::FrameBuffer::Plane`, under either hider.
- **Coverage is the filtered fraction of a pixel's samples that hit something.** An imager
  reads it as `alpha`. It separates a pixel nothing was drawn into from a black one. It is
  exactly zero or one only at one sample per pixel under a one-pixel box filter.
- **A value outside [0, 1] saturates** when the planes become an image. A value that is not a
  number becomes 0.
- **Raster y runs downward from the upper left.** This is RI's convention and `image::Image`'s
  row order.
- **Raster space is `raster * screen` applied to a camera space point**: the projection first,
  then the scale into pixels. A matrix applies to the point on its right. Reversing either the
  y direction or the order writes a correct render upside down, or in eye units.
- **A depth is raster space z under either hider**: the projection, then the raster matrix. A
  ray hit is carried back into camera space and projected the same way.

## Sampling and the film

A pixel's colour is a filtered set of samples. Three classes divide the work:

- `offline::Sampling` holds what `PixelSamples`, `PixelFilter`, `PixelVariance`, `Shutter` and
  `DepthOfField` set. Every field starts at the RI default.
- `offline::Sampler` gives a pixel's samples: a raster position, a time and a lens point.
- `offline::Film` filters samples into pixels and writes them into a renderer's planes.

Both hiders render through them.

- **The RI defaults are two by two samples per pixel under a gaussian filter two pixels wide.**
  A scene that names neither is antialiased, and four times slower than one sample per pixel.
- **Samples are stratified and jittered**, over a grid the size of `PixelSamples`.
- **A pixel's samples are seeded by its column and row**, through a `type::Random`. A frame is
  the same on every run, every standard library, and any order of pixels or buckets.
- **An axis with one stratum is sampled at the pixel centre**, not jittered. One sample under a
  one-pixel box therefore reproduces a render taken at pixel centres exactly.
- **A `PixelSamples` rate below one still takes one sample**, and a rate above 256 takes 256
  along that axis.
- **A sample is filtered into every pixel whose centre lies within half the filter's width**
  on each axis, as it arrives, since the width is the filter's whole extent. The film holds a
  weighted sum per pixel, not the samples.
- **The filters use RI's formulas**, cut off at the width the scene gives. Catmull-rom peaks at
  two and has a negative lobe, so a pixel's weights can sum to nearly zero. The film writes
  black there instead of dividing.
- **A filtered colour is premultiplied by opacity**, as `Ci` is.
- **A miss carries a colour into the film.** Under the ray hider it is
  `trace::Scene::background()`, which is black unless something sets it. Coverage counts only
  hits.
- **A depth is not filtered.** It is the nearest hit among the samples inside the pixel itself,
  because a blend of two surfaces' depths lies on neither surface. A pixel no sample hit keeps
  the value its depth plane held.
- **A reference image that pins hiding or shading names `PixelSamples 1 1` and
  `PixelFilter "box" 1 1`**, in its test code and in its `.rib`. `reference-sampled.png` and
  `raytrace-sampled.png` pin the defaults.

Background: [ADR-0076](../adr/0076-offline-seeded-samples-resolved-by-one-shared-film.md)

## Cameras and projections

- **The defaults are moya's**: a 320 by 240 image, pixel aspect 1, an orthographic projection,
  a 90 degree field of view for a perspective one, and clipping from `1e-10` to `1e38`.
- **A picture side is from 1 to 65536 pixels**, `largestResolution` in
  [Sampling.h](../../api/render/offline/Sampling.h), wherever it enters the renderer: `Format`,
  `RiFormat` or the command line. A larger side is refused, and so is one that is not a number,
  is infinite in either direction, or is between 0 and 1. The size already set is kept. A
  fraction is dropped.
- **A `Format` side of zero or less is the default for that side**, 320 for the width and 240
  for the height, as RI reads it. The other side keeps the size `Format` named. A pixel aspect
  that is not a positive finite number is square pixels.
- **The frame aspect follows `Format` and the screen window follows the frame aspect**, unless
  the scene names either. A scene can set `FrameAspectRatio` or `ScreenWindow` without the other
  reverting it. The default screen window for an image wider than tall is `[-a, a]` by
  `[-1, 1]`, where `a` is the frame aspect.
- **The camera options take effect at `WorldBegin`, in any order.** A `Format`,
  `FrameAspectRatio`, `ScreenWindow` or `Clipping` named after `Projection` still shapes the
  screen transform, which `prepareWorld()` builds again from the options as they stand.
- **The world to camera transformation applies as it stands.** `RenderContext::prepareWorld()`
  saves the current transformation as the `"camera"` coordinate system. By the RI standard,
  that transformation is the world to camera one. A transpose or an inverse of it is correct
  only when it is a pure rotation.
- **Both hiders project through the same coordinate systems**, `"camera"`, `"screen"` and
  `"raster"`. Any camera one hider accepts, the other accepts: an off-centre `ScreenWindow`, a
  matrix that scales, and a matrix that reverses handedness. RI's camera basis for a general
  look-at reverses handedness.
- **The ray hider finds a primary ray by inverting the camera to raster transformation.** It
  solves for camera x and y at a chosen depth, instead of inverting the whole matrix, because
  RI's default clipping range ruins the depth terms of a full inverse. Rays start on the near
  plane.
- **The frustum cull uses `type::geometry::Frustum` with `Depth::MinusOneToOne`**, the depth
  range moya's projection produces.

## Depth of field

- **`DepthOfField fstop focallength focaldistance` sets the lens.**
  `Sampling::lensRadius()` is `focalLength / (2 * fstop)`, in camera space units. An infinite
  fstop, an fstop of zero or a focal length of zero is a pinhole.
- **Only a perspective camera blurs.** An orthographic camera has no lens to move and ignores
  `DepthOfField`.
- **The lens is at the eye**, for both hiders.
- **The ray hider moves a ray's origin across the lens** and aims it at the point the pinhole ray
  would reach on the plane of focus.
- **The reyes hider moves the micropolygon instead.** A sample's lens point `L` shifts an eye
  space point by `L * radius * (1 - z / focalDistance)` in x and y. Depth is unchanged, so the
  perspective divide is unchanged, and a corner's raster position is linear in `L`. Each
  micropolygon is therefore projected three times, at the lens centre and one unit along each
  lens axis, and every sample's corners are a sum of those. Its bound is grown to cover the
  lens's four extremes, so every sample it can reach is tested.

## Motion blur

- **Only a transformation moves.** `offline::MovingTransform` is the current transformation at
  a motion block's two ends. Inside a block, each transform request applies to its own copy of
  the transformation the block started with, the first to the open end and the last to the
  close end. A block naming more than two times keeps its first and last. Outside a block, a
  request applies to both ends, so what follows a block moves with it.
- **Between the ends, translation and scale are interpolated linearly and rotation by a
  quaternion.** That is exact for a rigid motion with uniform scale. A shear, or a non-uniform
  scale under a rotation, is approximate. When an end flattens the primitive, as a motion that
  grows it from nothing does, the two ends are blended as matrices instead, which grows it in a
  straight line out of its flat end.
- **A sample's time** lies between the `Shutter` open and close times. The default shutter is
  `0 0`, which means no blur.
- **The ray hider carries each ray into the pose at its sample's time.** Its shadow and traced
  rays see that same time.
- **A moving primitive is stored at its reference end**, `MovingTransform::reference()`: the
  open end, unless that end has no inverse, as when a motion grows a primitive from nothing; then
  the close end. Both hiders move it from there to a sample's time by `at(time)` times the
  reference's inverse. A moving primitive flat at both ends has no reference to move from,
  and moya logs it once and leaves it out.
- **The reyes hider caches the motion to each sample's time** over the region a moving grid
  sweeps, up to `RenderContext::motionCache()` entries, a million by default. A grid that
  sweeps more works out the motion for each micropolygon instead, and draws the same picture.
- **The reyes hider places a moving micropolygon per sample.** A primitive carries its moving
  object to eye transformation. The hider moves the eye space corners from the reference end to
  the sample's time. The micropolygon's bound is the union of where it is at eight slices of the
  shutter, grown by the furthest a corner moves in one slice. A sample is rejected by its own
  slice's bound before anything is placed.
- **A moving primitive is culled by its bound at both ends**, and measured for splitting at its
  reference end only, because a split shrinks a primitive but not the distance it travels.
- **A moving occluder's shadow is sharp under the reyes hider**, because a grid's shadow rays
  are cast at shutter open.
- **A pixel's seed is its position**, so two frames of an animation share their sample
  patterns. A moving scene shows this as fixed-pattern noise. The fix is to fold the frame
  number into the seed, in `offline::Sampler` alone.

## Adaptive sampling

- **Only the ray hider adapts its sample count.** With `PixelVariance` above zero, it takes
  another seeded set of samples wherever the variance of a pixel's mean, in its worst channel,
  is above the bound. It stops at four times the first set.
- **`RenderContext::samplesTaken()` returns how many samples a pixel took.** It returns zero
  under the reyes hider.
- **The reyes hider takes the count `PixelSamples` names.** It samples a whole bucket at once,
  and adapting per pixel would split a grid's hiding in two.
- **A scene moved between hiders can therefore change its noise.** The hider is named in the
  file, so the change is never silent.
