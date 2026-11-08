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

- **Each pixel holds the beauty and the channels.** The beauty is ACEScg's red, green and blue.
  The channels are those the film was made with, each a name and 1 to 4 floats, in the order
  given. `offset(name)` is where a channel's first float sits.
- **Each pixel holds weighted sums, and one sum of weights.** `kernel::film::Pixels::add()` adds
  a sample's radiance, converted to ACEScg through the observer, and adds its weight.
  `addChannel()` adds the same sample's value to a channel, with the same weight, and does not
  add the weight again.
- **A pixel's samples are added in index order**, by the one work item that owns the pixel. Its
  sums are then the same under any thread count.
- **The sums are floats**, as a GPU adds them.
- **A film with no pixels, a channel with no name or a repeated one, and a channel of more than
  4 floats** throw `std::invalid_argument`. `offset()` of a name the film lacks throws
  `std::out_of_range`.

## Passes and snapshots

- **Samples are added in passes.** `pass(launch)` hands the function the film's
  `kernel::film::Pixels`. The function launches kernel code over the film's resolution, which
  adds each pixel's next samples, and returns when every pixel has run.
- **`snapshot()` can be called from any thread at any time.** It waits for a pass in progress to
  end, so a snapshot holds every sample of the passes before it, and none of the next.
- **A snapshot is resolved.** Each sum is divided by its pixel's weight, and a pixel no sample
  reached is zero. `passes` says how many passes it holds. `at(x, y, offset)` reads a float, at
  0 to 2 for the beauty or at a channel's offset.
- **A snapshot describes itself.** It holds the film's channels and their offsets, so
  `offset(name)` finds a channel without the film, and a writer needs only the snapshot.
