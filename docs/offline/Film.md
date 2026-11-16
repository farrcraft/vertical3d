# The film

This page is for anyone changing how moya places camera samples and adds them to a
picture. The host side is in `moya/libmoya/film/`, in the namespace `v3d::moya::film`. The
kernel side is in `moya/libmoya/kernel/film/`, in `v3d::moya::kernel::film`.

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

`film::Filter` builds one of RI's five filters on a device, from the weight
`film::Filter::weight()` gives and a width along each axis. RIB names the filter as
`rib::Filter`. `view()` gives the `kernel::film::PixelFilter` that kernel code
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
  blue, then its alpha.
  The channels are those the film was made with, each a name, 1 to 4 components and a combine,
  in the order given. `offset(name)` is where a channel's first float sits.
- **Each pixel holds weighted sums, and one sum of weights.** `kernel::film::Pixels::add()` adds
  a sample's radiance, converted to ACEScg through the observer, and adds its weight. It adds one
  to the pixel's count of samples.
- **The alpha is the sample's coverage**, added with the sample's weight. A sample covers its
  pixel when its camera ray met geometry or a light's shape, and does not when the ray left the
  scene, so a dome light the camera sees leaves its pixels clear. A camera ray that passes a
  cut-out surface counts only the surface it kept. `add()` takes the coverage, and one unless it
  is given.
  `addChannel()` adds the same sample's value to a channel, with the same weight, and does not
  add the weight again.
- **A channel's combine says how its samples make its pixel.** `Pixels::keep()` adds a sample by
  it, as `kernel::film::Combine` names them:
  - `Mean` adds the value times the sample's weight, as `addChannel()` does, and is the default.
  - `Sum` adds the value alone.
  - `Closest` keeps the sample of the least key, the depth along its camera ray. It holds the key
    in one more float after its components. Of samples at the same depth it keeps the first
    taken, which is the one of the lowest index, since a pixel takes its samples in index order.
  - `Least` and `Most` keep each component's least and greatest.

  A new pixel holds zero, but a least channel starts at infinity, a most channel at minus
  infinity, and a closest channel's key at NaN, which any key replaces.
- **A pixel's samples are added in index order**, by the one work item that owns the pixel. Its
  sums are then the same under any thread count.
- **The sums are floats**, as a GPU adds them.
- **A film with no pixels, a channel with no name or a repeated one, and a channel of more than
  4 components** throw `std::invalid_argument`. `offset()` of a name the film lacks throws
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
- **A snapshot is resolved.** The beauty's sums and each mean channel's are divided by the
  pixel's weight. The other combines are kept as they are, and a closest channel keeps its key.
  A pixel no sample reached is zero in every float. `passes` says how many passes it holds.
  `at(x, y, offset)` reads a float, at 0 to 3 for the beauty and its alpha or at a channel's
  offset. `error(x, y)` reads a pixel's standard error.
- **A snapshot describes itself.** It holds the film's channels and their offsets, so
  `offset(name)` finds a channel without the film, and a writer needs only the snapshot.

## Cryptomatte and deep lists

A render keeps each pixel's Cryptomatte coverage and deep lists on the host, beside the film,
and a snapshot holds both. [Compositing.md](Compositing.md) describes them.

## Checkpoints

A checkpoint is the film as its device holds it, written so that a stopped render can continue
and give the same picture, byte for byte, as one that never stopped. A resolved snapshot cannot
do that, since multiplying a pixel back by its weight does not give its sums exactly.

- **`state()` gives the film unresolved**: every float each pixel holds, which pixels have
  stopped, and the count of passes, as a `film::State`. `restore(state)` puts them back, and the
  next pass continues from them. A state of another resolution, other channels, or too few
  floats or flags throws `std::invalid_argument`.
- **`output::writeCheckpoint(path, state, fingerprint)` writes a state as an EXR file** of one part,
  of floats, ZIP compressed, so any EXR reader opens it. The beauty's weighted sums are `R`, `G`,
  `B` and `A`, so a reader shows the picture scaled by each pixel's weight. A channel is named as
  the picture's writer names it, and a closest channel's key is `moya.key.` followed by its
  name. The floats after the channels are `moya.half.R`, `moya.half.G`,
  `moya.half.B`, `moya.half.weight`, `moya.sampleCount`, `moya.moment.Y2`, `moya.moment.Y`,
  `moya.moment.weight2` and `moya.weight`. Each pixel's stop flag is `moya.stopped`, of unsigned
  integers. The header's `moya/passes` holds the count of passes, and `moya/fingerprint` the
  fingerprint as sixteen hexadecimal digits.
- **A render of Cryptomatte layers keeps their coverage in its checkpoint.** The film's floats
  are then the scanline part `moya.film`, and every pixel's coverage the deep part
  `moya.coverage`. Each entry is a sample of unsigned channels: `layer`, `id`, and `weight.high`
  and `weight.low`, the halves of the weight's 64 bits, so a resumed render sums the same
  weights. An entry of the layer `0xFFFFFFFF` holds the pixel's whole weight.
- **A render of a deep output keeps its deep lists in its checkpoint** as the deep part
  `moya.deep`. Each entry's depth is its float's bits in `depth`, then `object` and `material`,
  then the halves of the bits of its weight and of its weighted red, green and blue. An entry
  whose object is `0xFFFFFFFF` holds the pixel's whole weight.
- **A checkpoint is written whole or not at all.** It is written to the path with `.partial`
  added, then renamed over the path, so a render stopped while it writes keeps the checkpoint
  before.
- **`output::readCheckpoint(path, channels, fingerprint)` reads one back** for a film of the
  channels given. A file that is not a checkpoint, a fingerprint that differs, or a missing
  channel throws `output::Refused`.
- **The fingerprint names the scene and the film.** `integrate::fingerprint(scene, channels)` is
  a 64-bit FNV-1a hash of the scene's description, the film's channels with their combines, and
  `kSamplesPerPass`. A change to any object, parameter or channel changes it, and a resume
  against it is refused. The
  description writes a float to six digits, so a change smaller than that keeps the fingerprint.
  A file a parameter names, such as a texture, is in the description by its name alone.

Background: [ADR-0216](../adr/offline/0216-offline-a-checkpoint-is-an-exr-of-the-films-sums.md)
