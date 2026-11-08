# The sampler

This page is for anyone changing how moya's new core draws the random numbers a path uses. The
code is kernel code, in `moya/libmoya/kernel/sample/` and the namespace
`v3d::moya::kernel::sample`. The old film's sampler, `offline::film::Sampler`, serves the reyes
and ray hiders and is described in [Cameras and sampling](CamerasAndSampling.md).

## A sample holds no state

`ZSobol` is the sampler. A sample is a function of five things, and of nothing else:

- **the pixel**, as a `glm::uvec2`;
- **the sample's index** within the pixel, from zero;
- **the dimension**, which numbers the draws one path makes;
- **the seed**, given when the sampler is made;
- **the frame**, given when the sampler is made.

The sampler holds the samples per pixel, the resolution's size in digits, and the seed and frame
mixed into one value. Nothing changes as samples are drawn. Any thread draws any sample in any
order and gets the same value, so a picture is the same under any thread count.

- **`get1D(pixel, index, dimension)`** gives one value in [0, 1).
- **`get2D(pixel, index, dimension)`** gives two values in [0, 1), stratified together.
- **Each draw takes a dimension of its own**, whether it gives one value or two. Two draws at
  the same dimension give the same values.
- **The samples per pixel are a power of two.** A count that is not is rounded up, and
  `samplesPerPixel()` gives the count used.
- **Every frame takes its own samples.** The frame is part of the scrambling's seed, so two
  frames draw different values for the same pixel, index and dimension.

Background: [ADR-0194](../adr/offline/0194-offline-stateless-samples-one-picture-per-device.md)

## How a sample is drawn

`ZSobol` is Ahmed and Wonka's sampler, as pbrt-v4's `ZSobolSampler` builds it.

1. **The pixel's Morton index**, with the sample's index below it, is one index into one
   two-dimensional Sobol sequence. Pixels along the Morton curve take consecutive runs of the
   sequence, so neighbouring pixels draw from disjoint parts of it.
2. **The index's base-4 digits are reordered**, from the highest. Each digit takes one of the 24
   orders of four digits, chosen by a hash of the digits above it and the dimension. When the
   samples per pixel are an odd power of two, the lowest digit is one bit, flipped or kept by
   the same kind of hash. The digits above a pixel's samples are its Morton index, so each pixel
   still takes an aligned run of the sequence.
3. **The reordered index gives a Sobol point** in the sequence's first two dimensions. `get1D`
   takes the first.
4. **The point is Owen scrambled**, each value by its own seed, from a hash of the dimension,
   the seed and the frame. The scrambling is Laine and Karras's hash with Vegdahl's improvement:
   each binary digit is flipped or kept by a hash of the digits before it.

An aligned run of 2^n points of the sequence is a (0, n, 2)-net: every grid of 2^n equal cells
over the unit square holds one point. Reordering the digits and scrambling keep that, so a
pixel's samples are stratified in every such grid, in every dimension.

## The generator matrices

Every dimension draws from the Sobol sequence's first two dimensions, whose generator matrices
are the same in every construction of the sequence. `SobolMatrices::make()` computes them,
so there is no table of direction numbers.

- **The first is the van der Corput sequence's.** Bit c of the index gives binary digit c of the
  value.
- **The second is Pascal's triangle mod 2.** Bit c of the index gives digit r when the binomial
  coefficient C(c, r) is odd. By Lucas's theorem, it is odd when r's bits are a subset of c's.

A value has 32 binary digits, and the matrices read 64 bits of the index. Index bits from 32 on
reach only the second dimension's leading digits.

`sobol(index, dimension)` gives the unscrambled value in index order. The sequence is often
listed in Gray-code order instead, which differs from the fourth value on.
