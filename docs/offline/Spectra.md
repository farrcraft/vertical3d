# Spectra

This page is for anyone changing how moya's new core carries light and turns it into colour.
The code is kernel code, in `moya/libmoya/kernel/spectrum/` and the namespace
`v3d::moya::kernel::spectrum`.

## A path carries four wavelengths

- **`SampledSpectrum`** holds a spectral quantity at a path's wavelengths: radiance, a
  reflectance or a throughput. It has one value for each of `kSamples` wavelengths, which is 4,
  and its arithmetic is per wavelength. `safeDivide` gives zero where its denominator is zero.
- **`SampledWavelengths`** holds the wavelengths, in nanometres, and the density each was drawn
  with. A spectrum's values are in the same order as its wavelengths.
- **The visible range is 360 nm to 830 nm**, the range of the CIE's observer.

## How wavelengths are drawn

`SampledWavelengths::visible(u)` draws a path's wavelengths from one random number.

- **The density follows the observer's luminance.** `visiblePdf` is proportional to
  1 / cosh²(0.0072 (λ − 538)) over the visible range, and zero outside it. Its scale makes it
  integrate to one. The wavelengths that affect a picture most are drawn most often.
- **`sampleVisible(u)` inverts the density's integral**, so u in [0, 1] gives a wavelength from
  360 nm to 830 nm, drawn with `visiblePdf`.
- **The first wavelength is the hero.** It is `sampleVisible(u)`. The others are
  `sampleVisible` at u + 1/4, u + 2/4 and u + 3/4, each wrapped into [0, 1), so the four lie
  evenly through the distribution.

An estimate divides each wavelength's contribution by its density. `density()` gives the four
densities as a spectrum.

## Colour from a spectrum

- **`observer(λ)`** gives the CIE 1931 2° colour-matching functions x̄, ȳ and z̄, interpolated
  linearly between the CIE's 1 nm rows. It is zero outside 360 nm to 830 nm, and for a NaN.
- **`toXyz(spectrum, wavelengths)`** is one sample's estimate of a spectrum's XYZ. It sums
  x̄ȳz̄ times each value over its density, then divides by the number of wavelengths and by
  `kYIntegral`, the sum of ȳ at 1 nm steps. A constant spectrum of one then has a Y of one. A
  wavelength drawn with no density adds nothing. The mean of many samples converges to the
  spectrum's XYZ.
- **`d65(λ)`** is CIE standard illuminant D65, interpolated between the CIE's 1 nm values from
  300 nm to 830 nm, and zero outside them.
- **`illuminantA(λ)`** is CIE standard illuminant A, from the formula that defines it: a
  Planckian radiator at 2848 K, with the second radiation constant 1.435 × 10⁷ nm K, normalised to
  100 at 560 nm.

Both illuminants also take a `SampledWavelengths` and give a `SampledSpectrum`.

## The CIE's tables

The observer and D65 are measured data. They are the CIE's own tables, in `CieObserver.h` and
`CieD65.h`, at 1 nm steps. Those two files hold the data and nothing else, because the CIE
licenses its tables under CC BY-SA 4.0 and the rest of the tree is MIT. Each file states the
licence and the CIE's citation. `CIE-NOTICE.txt` beside them is the notice a release shows, and
[notices.ts](../contributing/Dependencies.md#third-party-notices) puts it in the notices file.

The values are the CIE's, written as float literals. To replace a table, take the CSV from the
CIE's data table page and write it in the same form, with its citation.
