# The film

This page is for anyone changing how moya's new core places camera samples and adds them to a
picture. The host side is in `moya/libmoya/film/`, in the namespace `v3d::moya::film`. The
kernel side is in `moya/libmoya/kernel/film/`, in `v3d::moya::kernel::film`. The reyes and ray
hiders use `render::offline::film` instead, as [Cameras and sampling](CamerasAndSampling.md)
describes.

## A sample lands in its own pixel

A camera sample is drawn for one pixel, and is added to that pixel alone.

- **The pixel filter places it.** `kernel::film::PixelFilter::sample(u)` takes two random
  numbers and gives an offset from the pixel's centre, drawn with a density that follows the
  filter, and a weight.
- **The weight is the filter's value over that density.** A pixel's weighted mean of its own
  samples is then the filtered picture, without adding any sample to a neighbour.
- **A filter with negative lobes gives samples of either sign.** Catmull-Rom and sinc do. The
  weight's magnitude is the same for every sample of a filter, and its sign is the filter's sign
  where the sample landed.

Background: [ADR-0199](../adr/offline/0199-offline-a-sample-lands-in-the-one-pixel-it-was-drawn-for.md)

## The pixel filters

`film::Filter` builds one of RI's five filters on a device, from `render::offline::film::filter`
and a width along each axis. `view()` gives the `kernel::film::PixelFilter` that kernel code
reads.

- **A box** draws its offset uniformly over the width.
- **A triangle** draws each axis from the tent's inverse.
- **Catmull-Rom, Gaussian and sinc** are tables. Each cell holds the filter's value at the cell's
  centre. A table has `Filter::kPerPixel` cells per pixel of width along each axis, 32, and at
  most `Filter::kMostCells`, 1024. A cell is drawn in proportion to its absolute value, through
  the cumulative distributions of the rows and of each row's cells, and a position within it
  uniformly.
- **A width that `filterWidth()` refuses**, and a filter that is zero over its whole width,
  throw `std::invalid_argument`.

## The film's pixels

`film::Film` holds a picture's pixels in a device's memory. The device outlives the film.

- **Each pixel holds the beauty, the channels, the half beauty, a count of samples, the beauty's
  moments and a weight.** The moments give the pixel's standard error, as
  [the next section](#a-pixels-noise) describes, and the half beauty decides when the pixel
  stops, as [Stopping a pixel](#stopping-a-pixel) does. The beauty is ACEScg's red, green and
  blue.
  The channels are those the film was made with, each a name and 1 to 4 floats, in the order
  given. `offset(name)` is where a channel's first float sits.
- **Each pixel holds weighted sums, and one sum of weights.** `kernel::film::Pixels::add()` adds
  a sample's radiance, converted to ACEScg through the observer, and adds its weight. It adds one
  to the pixel's count of samples.
  `addChannel()` adds the same sample's value to a channel, with the same weight, and does not
  add the weight again.
- **A pixel's samples are added in index order**, by the one work item that owns the pixel. Its
  sums are then the same under any thread count.
- **The sums are floats**, as a GPU adds them.
- **A film with no pixels, a channel with no name or a repeated one, and a channel of more than
  4 floats** throw `std::invalid_argument`. `offset()` of a name the film lacks throws
  `std::out_of_range`.

## A pixel's noise

Each pixel holds three sums beside its weight, which give the standard error of its beauty's
luminance. Each is a sum over the pixel's samples of the squared weight: times the squared
luminance, times the luminance, and alone.

- **The error is a weighted mean's.** The variance is each squared weight times its squared
  distance from the mean, summed, and divided by the squared sum of the weights. It is then
  scaled by n / (n − 1), where n is the squared sum of the weights over the sum of their
  squares. For samples of equal weight, that is the samples' variance over their count.
- **A pixel whose samples weigh less than two of equal weight** has an infinite error, since one
  sample shows no spread.
- **The error treats the samples as independent.** ZSobol's samples are stratified, so their
  mean converges faster than the error says, and a comparison through it errs towards passing.
- **The sums are floats**, and the snapshot computes the error from them in doubles.

## Stopping a pixel

Adaptive sampling stops each pixel once its noise falls below a threshold.
[Integrator.md](Integrator.md#adaptive-sampling) says when a render checks.

- **The half beauty holds half of each pass's samples.** A pass's samples are a run of a power of
  two, and the first half of a run is stratified as the whole run is. `add()` given `half` adds
  the sample to the half beauty too, as weighted sums of red, green and blue and a sum of their
  weights.
- **A pixel's noise is `Film::noise()`**, as Cycles measures it. It is the distance of the half
  beauty's mean from the beauty's, summed over red, green and blue. That is divided by the square
  root of the beauty's sum of red, green and blue, held to zero or more. 0.0001 is added to the
  root so that a black pixel's noise is finite. A pixel whose beauty or half beauty
  holds no weight has an infinite noise.
- **`converge(threshold)` stops a pixel with its four neighbours.** A pixel stops when its noise
  and the noise of each neighbour inside the picture are below the threshold, all as the last
  pass left them. A pixel whose two halves agree by chance beside noise then keeps sampling. It
  returns how many pixels still sample.
- **A pixel that has stopped stays stopped**, and `Pixels::done()` says so to kernel code, which
  adds it no samples. The flags are a buffer of their own, one for each pixel, which
  `converge()` uploads.
- **A snapshot holds each pixel's count of samples**, as `samples` and `count(x, y)`.

Background: [ADR-0215](../adr/offline/0215-offline-a-pixels-noise-is-the-difference-of-two-halves.md)

`film/Compare.h` measures a snapshot against an expected value or against another snapshot, in
standard errors. [Testing.md](../contributing/Testing.md#moya-references) describes how a test
uses it.

## Passes and snapshots

- **Samples are added in passes.** `pass(launch)` hands the function the film's
  `kernel::film::Pixels`. The function launches kernel code over the film's resolution, which
  adds each pixel's next samples, and returns when every pixel has run.
- **`snapshot()` can be called from any thread at any time.** It waits for a pass in progress to
  end, so a snapshot holds every sample of the passes before it, and none of the next.
- **A snapshot is resolved.** Each sum is divided by its pixel's weight, and a pixel no sample
  reached is zero. `passes` says how many passes it holds. `at(x, y, offset)` reads a float, at
  0 to 2 for the beauty or at a channel's offset. `error(x, y)` reads a pixel's standard error.
- **A snapshot describes itself.** It holds the film's channels and their offsets, so
  `offset(name)` finds a channel without the film, and a writer needs only the snapshot.
